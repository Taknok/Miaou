#ifndef CONFIG_H_
#define CONFIG_H_

#define PORT 3310
#define ADDRESS [::1]
#define MAX_CLIENTS 21

#define BUFFER_SIZE 2048
#define SIZE_DATE 128

#define PORT_SERVEUR_CLIENT 4242
#define CHUNK_SIZE_RECEP 1024
static const char SAVE_LOCATION[] = "./download/";

//les commandes du serveur (custom maggle !)
static const char COMMAND_QUIT[] = 	"/q";
static const char COMMAND_NICK[] = 	"/n";
static const char COMMAND_WHO[] = 	"/who";
static const char COMMAND_WHOIS[] = "/whois";
static const char COMMAND_ALL[] = 	"/a";
static const char COMMAND_WHISP[] = "/w";
static const char COMMAND_HELP[] = 	"/help";
static const char COMMAND_QUIT_CHANNEL[]= "/qc";
static const char COMMAND_JOIN[] = 	"/j";
static const char COMMAND_SEND[] = 	"/s";

#endif /* CONFIG */
