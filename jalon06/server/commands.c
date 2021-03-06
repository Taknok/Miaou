#include <time.h> //juste dans le .c
#include <unistd.h>
#include <libgen.h> //pour le basename

#include "commands.h"


int check_commande_arg(char * buffer, char * commande){
    if ((strlen(buffer) -1)==strlen(commande)) {
        return TRUE;
    }
    return FALSE;
}

/*
Utilisée dans server pour gerer les cas de crash client donc a exporter
*/
int commande_quit(char * commande, int retour_client, Client * liste_clients, int i, int * compteur, fd_set * readfds){
    if (strcmp(commande, COMMAND_QUIT) == 0 || retour_client == 0) { //si deco
        printf("Deconnection du client\n");
        close(liste_clients[i].lst_sock);
        FD_CLR(liste_clients[i].lst_sock, readfds); //enlève le client de l'ecoute
        *compteur = remove_client(liste_clients, i, *compteur); //met notre client a zero
        return 1; //on a bien fait un /q
    } else {
        return 0; //on n'a pas fait un /q
    }
}

/*
Pas besoin de l'exporter dans un .h car elle est interne a commande
*/
int commande_nick(char * commande, char ** copy_buffer, Client * liste_clients, int i, char * buffer, int change_name,int compteur){
    char * argument;
    int k;

    if (check_commande_arg(buffer, commande)) { //éviter le seg fault si juste /nick
        snprintf(buffer, BUFFER_SIZE, "Entrez un pseudo valide avec %s pseudo!\n", COMMAND_NICK);
        return 0;
    }

    if (strcmp(commande, COMMAND_NICK) == 0) {
        argument = strsep(copy_buffer, " ");

        if (!change_name) {
            liste_clients[i].pseudo = (char *)malloc(strlen(argument) * sizeof(char));
        }

        // vérifie que le pseudo n'est pas déja utilisé
        for (k = 0; k < compteur; k++) {  // On commence à i car  le client 2 peut mettre son nom avant le client 1. si on commence à 0 => core dump
            if(strcmp(argument,liste_clients[k].pseudo) == 0) {
                snprintf(buffer, BUFFER_SIZE, "Le pseudo est deja utilisé, veuillez en utiliser un autre !\n");
                return 0;
            }
        }

        strcpy(liste_clients[i].pseudo, argument);

        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Vous avez été enregistré comme : %s\n", liste_clients[i].pseudo);

        liste_clients[i].registered = 1;

        return 1; //on a bien fait la commande nick

    } else if (!change_name) {
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Veuillez vous enregistrer avec la commande %s [pseudo]\n", COMMAND_NICK);
        return 0; //on n'a pas fait la commande nick
    }
}


void commande_who(Client * liste_clients, int compteur, Message * message){
    int i = 0;
    memset(message->buffer, 0, BUFFER_SIZE);
    for (i = 0; i < compteur; i++) {
        strcat(message->buffer, "\t- \'");
        strcat(message->buffer, liste_clients[i].pseudo);
        strcat(message->buffer, "\'\n");
    }
    message->destination = no_one;
}


void display_time(time_t connection_date, char * date){
    struct tm temps = *localtime(&connection_date);
    strftime(date, SIZE_DATE, "le %x à %X", &temps);
    puts(date);
}


void commande_whois(char * commande,Client * liste_clients, int compteur, char * buffer, char ** copy_buffer){

    if (check_commande_arg(buffer, commande)) {  // Pour éviter le core dump si on a rien mis après la commande (juste /whois)
        snprintf(buffer, BUFFER_SIZE, "Entrez la commande %s [pseudo] \n",COMMAND_WHOIS);
        return;
    }

    char * argument = strsep(copy_buffer, " ");
    int indice = get_indice_user(liste_clients, argument, compteur);

    if (indice != -1) { //si on a trouvé

        char date[SIZE_DATE];
        display_time(liste_clients[indice].connection_date, date);

        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "%s est connecté depuis %s avec l'ip %s et le port %i\n"
        , argument, date, liste_clients[indice].ip, liste_clients[indice].port);

    } else {
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Le client %s est inconnu(e)\n", argument);

    }
}


void commande_help(Message * message){
    memset(message->buffer, 0, BUFFER_SIZE);
    snprintf(message->buffer, BUFFER_SIZE, "\nCommandes diponibles :\n"
    "\t- %s \t\t\t> ajout ou change de pseudo\n"
    "\t- %s \t\t\t> ferme le chat\n"
    "\t- %s \t\t\t> affiche les utilisateurs en ligne\n"
    "\t- %s [pseudo] \t> affiche les informations relatives au client\n"
    "\t- %s \t\t\t> envoyer un message a tout le monde\n"
    "\t- %s [pseudo] \t\t> envoyer un message a un client précis\n"
    "\t- %s  \t\t\t> Rejoindre un salon de discussion\n"
    "\t- %s  \t\t\t> Quitter votre salon de discussion\n\n"
    "\t- %s [pseudo] \"file/2/send\"\t> Envoyer un fichier a un utilisateur\n\n",
    COMMAND_NICK, COMMAND_QUIT, COMMAND_WHO, COMMAND_WHOIS,  COMMAND_ALL, COMMAND_WHISP, COMMAND_JOIN, COMMAND_QUIT_CHANNEL, COMMAND_SEND);

    message->destination = no_one;
}

int check_client_exist(Client * liste_clients,  int compteur, char * pseudo){
    int i = 0;
    int exist = FALSE;
    for (i = 0; i < compteur; ++i) {
        if (strcmp(liste_clients[i].pseudo, pseudo) == 0) { //si on trouve le destinataire
            exist = TRUE;
            break;
        }
    }
    return exist;
}

void commande_whisp(char * commande, char ** copy_buffer, Message * message, Client * liste_clients, int compteur, char * sender_name){
    if (check_commande_arg(message->buffer, commande)) {  // Pour éviter le core dump si on a rien mis après la commande (juste /whois)
        snprintf(message->buffer, BUFFER_SIZE, "Entrez la commande %s [pseudo] blabla \n",COMMAND_WHISP);
        return;
    }

    char * argument = strsep(copy_buffer, " ");
    memset(message->buffer, 0, BUFFER_SIZE);

    int exist = check_client_exist(liste_clients, compteur, argument);

    if (exist) {
        message->destination = user;
        strcpy(message->dest_name, argument);

        snprintf(message->buffer, BUFFER_SIZE, "%s[%s] : %s\n%s", TEXT_COLOR_MAGENTA,sender_name, *copy_buffer, TEXT_COLOR_RESET); //supprime la commande et l'argument du message a transmettre
    } else {
        message->destination = no_one; //on renvoie un message d'erreur
        snprintf(message->buffer, BUFFER_SIZE, "/!\\ utilisateur non trouvé !\n"); //supprime la commande et l'argument du message a transmettre
    }
}


void commande_join_channel(char * commande, char ** copy_buffer, Client * liste_clients, int i, int compteur, Message * message){

    if (check_commande_arg(message->buffer, commande)) {  // Pour éviter le core dump si on a rien mis après la commande (juste /join)
            snprintf(message->buffer, BUFFER_SIZE, "Entrez la commande %s [nom du salon] \n", COMMAND_JOIN);
            return;
        }

    if (liste_clients[i].channel != NULL) { //si on es deja dans une channel
        message->destination = no_one;
        snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_RED "Veuillez quitter le salon en 1er !\n" TEXT_COLOR_RESET);
        return;
    }

    char * argument = strsep(copy_buffer, " "); //argument contient le nom de la channel
    Channel * channel;

    if (channel_exist(liste_clients, argument, compteur)){
        channel = channel_find(argument, liste_clients, compteur);
        liste_clients[i].channel = channel;
    } else {
        // snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_YELLOW TEXT_COLOR_BOLD "/!\\" TEXT_COLOR_UNBOLD TEXT_COLOR_MAGENTA " Cette channel n'existe pas !\n" TEXT_COLOR_RESET);
        channel = channel_create(argument);
        liste_clients[i].channel = channel;
    }
    channel_add_subscriber(channel);
    message->destination = no_one;
    snprintf(message->buffer, BUFFER_SIZE, "Vous avez été déplacé dans le channel %s\n", argument);
}

int commande_quit_channel(Client * liste_clients, int i, Message * message){
    if (liste_clients[i].channel == NULL) { //si on n'est pas dans une channel
        message->destination = no_one;
        snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_RED "Veuillez rejoindre un salon en 1er !\n" TEXT_COLOR_RESET);
        return;
    }

    channel_rem_subscriber(liste_clients[i].channel);

    liste_clients[i].channel = NULL;

    message->destination = no_one;
    snprintf(message->buffer, BUFFER_SIZE, "Sortie du salon\n");
}

void commande_send_file(char * commande, char ** copy_buffer, Client * liste_clients, int i, int compteur, Message * message){
    if (check_commande_arg(message->buffer, commande)){
        snprintf(message->buffer, BUFFER_SIZE, "Entrez la commande %s [user] \"ficher/a/envoyer\" \n", COMMAND_SEND);
        return;
    }



    char * user_dest_name = strsep(copy_buffer, " ");
    char * file_location = strsep(copy_buffer, " ");

    int exist = check_client_exist(liste_clients, compteur, user_dest_name);

    if (!exist){
        message->destination = no_one;
        snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_RED "Le client est inconnu\n" TEXT_COLOR_RESET);
        return;
    }

    int k;
    for(k = 0; k < compteur; k++){
        if (strcmp(user_dest_name, liste_clients[k].pseudo) == 0){
            break; //on a trouvé l'indice
        }
    }
    liste_clients[i].etat_transfert = TRUE; //le client i (qui init le send) veut envoyer un fichier
    liste_clients[k].etat_transfert = -1; //en attente

    Transfert * transfert = malloc(sizeof(*transfert));
    transfert->sock_sender = liste_clients[i].lst_sock;
    transfert->sock_recv = liste_clients[k].lst_sock;
    strcpy(transfert->file, file_location);

    liste_clients[i].transfert = transfert;
    liste_clients[k].transfert = transfert;

    char out[BUFFER_SIZE];
    replace_str(basename(file_location), "\"", "", out);
    snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_CYAN "%s souhaite vous envoyer le fichier %s. Acceptez vous ? [Y,n]\n" TEXT_COLOR_RESET, liste_clients[i].pseudo, out); //basename renvoie le nom du fichier
    message->destination = user;
    message->dest_name = liste_clients[k].pseudo;
}

void destroy_transfert(Client * liste_clients, int i, int compteur, Message * message){
    if(liste_clients[i].etat_transfert == 3){ //refusé
        snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_RED "Transfert refusé\n" TEXT_COLOR_RESET);
    } else if (liste_clients[i].etat_transfert == 2){ //accepté
        snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_GREEN "Transfert accepté\n" TEXT_COLOR_RESET);
    }

    int k; //cherche le client qui voulais envoyer
    for (k = 0; k < compteur; ++k){
        if (liste_clients[k].lst_sock == (liste_clients[i].transfert)->sock_sender){
            break;
        }
    }
    message->destination = user;
    message->dest_name = liste_clients[k].pseudo;

    liste_clients[i].etat_transfert = 0;
    liste_clients[k].etat_transfert = 0;
    free(liste_clients[k].transfert);
    liste_clients[i].transfert = NULL;
    liste_clients[k].transfert = NULL;
    printf("Destruction du transfert\n");
}

void init_transfert(Client * liste_clients, int i, int compteur, Message * message){
    Transfert * tmp = liste_clients[i].transfert;

    printf("Envoie de la notif transfert a l'emetteur\n"); //on envoie l'ip du client qui recois pour pourvoir se connecter
    while(send(tmp->sock_sender, liste_clients[i].ip, 2 * BUFFER_SIZE, 0) == -1) { //astuce on envoie 2 fois plus de donnée que normalement, a la recep on sera que l'on est en transfert
        perror("erreur envoie\n");
    }


    char file_name[BUFFER_SIZE];
    replace_str(tmp->file, "\"", "", file_name);
    printf("Envoie de la notif transfert au recepteur\n");
    while(send(tmp->sock_recv, basename(file_name), 2 * BUFFER_SIZE, 0) == -1) {
        perror("erreur envoie\n");
    }
    destroy_transfert(liste_clients, i, compteur, message);
}

//Controleur des commandes
int do_commande(Message * message, int retour_client, Client * liste_clients, int i, int * compteur, fd_set * readfds){
    int salon;
    char * commande = message->buffer;
    char local_copy_buffer[BUFFER_SIZE];
    char * copy_buffer = local_copy_buffer;

    strcpy(copy_buffer, message->buffer);
    copy_buffer[strlen(copy_buffer) - 1] = '\0'; //-1 pour eviter le \n

    commande = strsep(&copy_buffer, " "); //recupere la commande

    if (commande_quit(commande, retour_client, liste_clients, i, compteur, readfds)) { //si deco
        return 0; //on ne veut pas rentrer en le send
    }

    switch (liste_clients[i].registered) {
    case 0:
        if (commande_nick(commande, &copy_buffer, liste_clients, i, message->buffer, 0,*compteur)) {
            printf("Le client %i a bien été enregistré comme %s\n", liste_clients[i].lst_sock, liste_clients[i].pseudo );
        }
        message->destination = no_one; //repond au client

        return 1; //on rentre dans le send
        break;

    case 1: //si on est enregistre

        /*TRANSFERT */
        if (liste_clients[i].etat_transfert == 1){
            return 0; //si on envoie on ne veut pas pouvoir faire d'autre commande
        }

        if (liste_clients[i].etat_transfert == -1){ //si on est en attente alors il faut choisir si on recoit ou pas
            if (strcmp(message->buffer, "Y\n") == 0){
                liste_clients[i].etat_transfert = 2; //on recoit le fichier
                init_transfert(liste_clients, i, *compteur, message);
            } else if (strcmp(message->buffer, "n\n") == 0){
                liste_clients[i].etat_transfert = 3; //on ne veut pas
                destroy_transfert(liste_clients, i, *compteur, message);
            } else {
                snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_RED "Veuillez répondre par Y ou n\n" TEXT_COLOR_RESET);
                message->destination = no_one;
            }
            return 1;//on rentre dans le send dans tous les cas
        }
        /* FIN TRANSFERT */

        if (strcmp(commande, COMMAND_NICK) == 0) {
            if (commande_nick(commande, &copy_buffer, liste_clients, i, message->buffer, 1,*compteur)) {
                printf("Le client %i a bien été enregistré comme %s\n", liste_clients[i].lst_sock, liste_clients[i].pseudo );
            }
        } else if (strcmp(commande, COMMAND_WHO) == 0) {
            commande_who(liste_clients, *compteur, message);
        } else if (strcmp(commande, COMMAND_WHOIS) == 0) {
            commande_whois(commande,liste_clients, *compteur, message->buffer, &copy_buffer);
            message->destination = no_one;
        } else if (strcmp(commande, COMMAND_HELP) == 0) {
            commande_help(message);


        } else if (strcmp(commande, COMMAND_ALL) == 0) { //message to all
            snprintf(message->buffer, BUFFER_SIZE, "[%s] : %s\n", (message->sender).pseudo, copy_buffer); //supprime la commande du message a transmettre
            message->destination = everyone;
            memset(message->dest_name, 0, BUFFER_SIZE);
        } else if (strcmp(commande, COMMAND_WHISP) == 0) {
            commande_whisp(commande, &copy_buffer, message, liste_clients, *compteur, liste_clients[i].pseudo);

        } else if (strcmp(commande, COMMAND_QUIT_CHANNEL) == 0){
            commande_quit_channel(liste_clients, i, message);
        } else if (strcmp(commande, COMMAND_JOIN) == 0){
            commande_join_channel(commande,&copy_buffer,liste_clients, i, *compteur, message);


        } else if (strcmp(commande, COMMAND_SEND) == 0){
            commande_send_file(commande, &copy_buffer, liste_clients, i, *compteur, message);


        } else { //aucune commande
            if ((message->sender).channel == NULL) { //si on n'est PAS dans une channel
                //il faut au moins une commande pour envoyer un message donc on renvoie une erreur
                memset(message->buffer, 0, BUFFER_SIZE);
                snprintf(message->buffer, BUFFER_SIZE,  TEXT_COLOR_BOLD TEXT_COLOR_YELLOW "/!\\ " TEXT_COLOR_UNBOLD TEXT_COLOR_RED "Erreur commande.\nFaite %s pour obtenir l'aide\n" TEXT_COLOR_RESET, COMMAND_HELP);
            } else {
                char sauv[BUFFER_SIZE];
                strcpy(sauv, message->buffer);
                snprintf(message->buffer, BUFFER_SIZE, TEXT_COLOR_GREEN TEXT_COLOR_BOLD "[%s]" TEXT_COLOR_UNBOLD " : %s" TEXT_COLOR_RESET, (message->sender).pseudo, sauv);
                message->destination = channel;
                message->dest_name = ((message->sender).channel)->name;
            }
        }

        return 1; //si aucune commande, peutetre que c'est juste un message donc on rentre dans send
        break;

    default:
        perror("liste_clients.registered different de 0 ou 1 !");
        return 0;
        break;
    }
}
