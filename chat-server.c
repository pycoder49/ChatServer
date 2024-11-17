#include "http-server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


/**
 * GNEREAL STRUCTURE OF THE FILE ->
 * 
 * HTTP constant codes
 * 
 * Struct objects:
 *      Reaction
 *      Chat
 *      ChatList
 * 
 * Constructors for each struct object type
 * 
 * get_time() -- function to easily get time string
 * 
 * Chat server methods:
 *      uint8_t add_chat(char* username, char* message)
 *      uint8_t add_reaction(char* username, char* message, char* id)
 *      unit8_t reset()
 * 
 * Handler methods:
 *      Error handling: 404
 *      handle_root()
 *      handle_chat()
 *      handle_path()
 *      handle_react()
 *      handle_reset()
 *      handle_response()
 * 
 * main()
 */


/**
 * HTTP Codes and Errors
 * 
 * 404: NOT FOUND error
 * 200: OK reponse, everything is good
 */
char const HTTP_404_NOT_FOUND[] = "HTTP/1.1 404 Not found\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_200_OK[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_500_INTERNAL_SERVER[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n";


/**
 * Objects: 
 * 
 * Reaction
 * Chat
 * ChatList
 */
struct Reaction {
    char ruser[16];
    char rmessage[256];
};
typedef struct Reaction Reaction;

struct Chat {
    uint32_t id;
    char user[16];
    char message[350];
    char timestamp[20];
    uint32_t num_reactions;
    uint32_t reaction_capacity;
    struct Reaction *reactions;
};
typedef struct Chat Chat;

struct ChatList{
    int size;
    int capacity;
    struct Chat *chat;
};
typedef struct ChatList ChatList;


/**
 * Object constructors for:
 * 
 * Reaction
 * Chat
 * ChatList
 */
Reaction *new_reaction(char *username, char *message){
    Reaction *newReaction = (Reaction*)malloc(sizeof(Reaction));
    
    //check if malloc failed
    if(newReaction == NULL){
        return NULL;
    }

    strncpy(newReaction->ruser, username, 15);
    newReaction->ruser[15] = '\0';

    strncpy(newReaction->rmessage, message, 255);
    newReaction->rmessage[255] = '\0';

    return newReaction;
}

Chat *new_chat(int id, char* username, char* message, char* timestamp){
    Chat *newChat = (Chat*)malloc(sizeof(Chat));
    
    //checking if malloc failed
    if(newChat == NULL){
        return NULL;
    }

    //assigning values to the object
    newChat->id = id;
    
    strncpy(newChat->user, username, sizeof(newChat->user) - 1);
    newChat->user[15] = '\0';
    
    strncpy(newChat->message, message, sizeof(newChat->message) - 1);
    newChat->message[350] = '\0';
    
    strncpy(newChat->timestamp, timestamp, sizeof(newChat->timestamp) - 1);
    newChat->timestamp[19] = '\0';

    newChat->num_reactions = 0;
    newChat->reaction_capacity = 100;

    newChat->reactions = calloc(100, sizeof(Reaction));
    
    return newChat;
}

ChatList* new_list(){
    ChatList* list = (ChatList*)malloc(sizeof(ChatList));
    
    //checking if malloc failed
    if(list == NULL){
        return NULL;
    }

    list->chat = calloc(5, sizeof(Chat));
    
    //checking if calloc failed
    if(list->chat == NULL){
        free(list);
        return NULL;
    }

    list->size = 0;
    list->capacity = 5;

    return list;
}


/**
 * time module to get the current time
 * 
 * @return "YYY-MM-DD HH:MM"
 */
char* get_time(){
    time_t t = time(NULL);

    //converts to local time
    struct tm tm = *localtime(&t);

    // "YYYY-MM-DD HH:MM" requires 16 characters and 1 for null terminator
    static char time_str[20];
    
    //formats the time into the timr_str string
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d", 
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    //returns a string in format "20XX-MM-DD HH:MM"
    return time_str;
}


/**
 * Chat server methods:
 * 
 * uint8_t add_chat(char* username, char* message)
 * uint8_t add_reaction(char* username, char* message, char* id)
 */
ChatList* chatList = NULL;  //to check for initialization
int chat_id = 0;            //same number as 'size' but keeps the concepts separate

uint8_t add_chat(char* username, char* message){
    if(chatList == NULL){
        chatList = new_list();
    }

    Chat *newChat = new_chat(chat_id, username, message, get_time());
    //check is size has reached maximum capacity
    if(chatList->size >= chatList->capacity){
        Chat* new_array = realloc(chatList->chat, sizeof(Chat) * chatList->capacity*2);

        //reallocate the chat array to the new array
        chatList->chat = new_array;
        chatList->capacity *= 2;
    }
    
    //copy the new list into the chatList at the next index size
    //then incrememnt the size so the next chat can be copied there
    chatList->chat[chatList->size] = *newChat;
    chatList->size++;

    //update current chat_id so the next function call has a new chat_id
    chat_id++;

    //this will free the memory used by newChat
    //this is okay because newChat was copied into chatList at index size
    free(newChat);

    // returning one if successful for assert testing purposes
    return 1;
}

uint8_t add_reaction(char* username, char* message, int id){
    Reaction *newReaction = new_reaction(username, message);

    chatList->chat[id].reactions[chatList->chat[id].num_reactions] = *newReaction;

    chatList->chat[id].num_reactions++;

    free(newReaction);
    
    // returning one is successful for assert testing purposes
    return 1;
}



/**
 * Handler methods
 * 
 * Paths handled:
 *      Error handling:
 *          404
 *      /       -- root path
 *      /chat
 *      /post
 *      /react
 *      /reset
 */

/**
 * Handles 404 errors
 */
void handle_404(int client_socket, char *path){
    printf("SERVER LOG: Got request for unrecognized path: \%s\"\r\n", path);

    char response_buff[BUFFER_SIZE];
    snprintf(response_buff, BUFFER_SIZE, "Error 404:\r\nUnrecognized path \%s\"\r\n", path);
    //snprintf includes a null-terminator

    //send response back to client
    write(client_socket, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND));
    write(client_socket, response_buff, strlen(response_buff));
}


/**
 * Handles the "/" path -- root path
 */
void handle_root(int client_socket, char *path){
    write(client_socket, HTTP_200_OK, strlen(HTTP_200_OK));

    char instructions_str[] = "Different requests you can make:\n";
    char chats_str[] = "/chats                                          -- for all chats\n";
    char post_str[] = "/post?user=<username>&message=<message>         -- to post a chat\n";
    char react_str[] = "/react?id=<id>&user<username>&message=<message> -- to add a reaction\n";
    char reset_str[] = "/reset                                          -- to reset everything\n";

    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "%s%s%s%s%s",
            instructions_str, chats_str, post_str, react_str, reset_str);

    write(client_socket, message, strlen(message));
}


/**
 * Handles /chats path
 * Prints out all the chats
 * 
 * Format:
 * [#N 20XX-MM-DD HH:MM]   <username>: <message>
                    (<rusername>)  <reaction>
                    ... [more reactions] ...
    ... [more chats] ...
 */
void responds_with_chat(int client_socket, char* path){
    char message[BUFFER_SIZE];

    //code to print out the chats and reactions
    if(chatList == NULL){
        write(client_socket, "No chats available\n", strlen("No chats available\n"));
    }

    //19 spaces + 1 null terminator
    char reaction_user_space[20] = "                   ";
    for (int i = 0; i < chat_id; i++) {
        //printing out the formatter chat
        
        //finding out the max len username
        int max_user_len = 0;
        for(int j = 0; j < chat_id; j++){
            int username_len = strlen(chatList->chat[j].user);
            if(username_len > max_user_len){ 
                max_user_len = username_len; 
            }
        }

        //finding the required padding
        int padding_amount = max_user_len - strlen(chatList->chat[i].user);
        char padded_username[BUFFER_SIZE];

        //copying the spaces into the padded_username
        strncpy(padded_username, reaction_user_space, padding_amount);
        padded_username[padding_amount] = '\0';

        //appending the actual username
        strcat(padded_username, chatList->chat[i].user);

        //printing the padded username
        snprintf(message, BUFFER_SIZE, "[#%u %s] %.17s: %s\n",
                                        chatList->chat[i].id + 1,
                                        chatList->chat[i].timestamp,
                                        padded_username,
                                        chatList->chat[i].message);
        write(client_socket, message, strlen(message));


        //printing out formated reactions
        if(chatList->chat[i].num_reactions > 0){
            for(int j = 0; j < chatList->chat[i].num_reactions; j++){
                //the line that's going to be printed out
                char reaction_line[BUFFER_SIZE];

                //calculating the number of spaces needed from the left
                int username_len = strlen(chatList->chat[i].reactions[j].ruser);
                int total_spaces = 17 - username_len;

                //copying the total number of spaces into the reaction_user_space
                strncpy(reaction_line, reaction_user_space, total_spaces);
                reaction_line[total_spaces] = '\0';

                //adding the username inside () and the emohji
                snprintf(reaction_line + total_spaces, BUFFER_SIZE - total_spaces,
                        "          (%s) %s",
                        chatList->chat[i].reactions[j].ruser,
                        chatList->chat[i].reactions[j].rmessage);

                write(client_socket, reaction_line, strlen(reaction_line));
                write(client_socket, "\n", strlen("\n"));
            }
        }
    }
}

void handle_chat(int client_socket, char* path){
    write(client_socket, HTTP_200_OK, strlen(HTTP_200_OK));
    responds_with_chat(client_socket, path);
}


/**
 * Handles /post request
 * Takes the username and message from the url and calls: add_chat
 * 
 * Prints out all the chats, including the newest one
 */
void handle_post(int client_socket, char* path){
    write(client_socket, HTTP_200_OK, strlen(HTTP_200_OK));
    char server_message[BUFFER_SIZE];

    //checking if it's null
    if(chatList == NULL) {
        chatList = new_list();
    }

    //check that the new chat does not exceed 100,000 chats
    if(chatList->size >= 100000){
        snprintf(server_message, sizeof(server_message), "Cannot add more chats--limit 100,000\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }


    //points to the first instance of the string specified
    char user_string[] = "user=";
    char *username_pointer = strstr(path, user_string);

    //check if it doesn't exist -- NULL
    if(!username_pointer || 
        *(username_pointer + strlen(user_string)) == '&' || 
        *(username_pointer + strlen(user_string)) == '\0')
    {
        snprintf(server_message, sizeof(server_message), "Invalid, user can not be empty\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    } 

    //checking if username is longer than 15 bytes/characters
    username_pointer += 5;
    int i = 0;
    while(i < 16 && username_pointer[i] != '&' && username_pointer[i] != '\0'){
        i++;
    }

    //prints out error mesasge if max length reached
    if(i==16){
        snprintf(server_message, sizeof(server_message), "Username cannot be longer than 15 characters\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //copies username into username[]
    char username[16];
    int j = 0;
    for(; j < i; j++){
        username[j] = username_pointer[j];
    }
    username[j] = '\0';



    //same process, but for message this time
    char message_string[] = "message=";
    char *message_pointer = strstr(path, message_string);

    //check if it doesn't exist -- NULL
    if(!message_pointer) {
        snprintf(server_message, sizeof(server_message), "Missing message field 'messag=<message>'\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //getting the length of the message
    message_pointer += 8;
    i = 0;
    printf("%d\n", i);
    while(i < 256 && message_pointer[i] != '&' && message_pointer[i] != '\0'){
        i++;
    }

    //prints out error if max length breached
    if(i==256){
        snprintf(server_message, sizeof(server_message), "Message cannot be longer than 255 characters\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }
    
    //copies message into message[]
    char message[256];
    j = 0;
    for(; j < i; j++){
        message[j] = message_pointer[j];
    }
    message[j] = '\0';



    //adds the new chat, and the prints all chats including the new one
    add_chat(username, message);
    responds_with_chat(client_socket, path);
}


/**
 * Handles /react request
 * 
 * Given the chat id, it adds a reaction to that chat
 */
void handle_react(int client_socket, char* path){
    write(client_socket, HTTP_200_OK, strlen(HTTP_200_OK));
    char server_message[BUFFER_SIZE];

    //checking if it's null
    if(chatList == NULL){
        write(client_socket, "No chats to add reactions to", strlen("No chats to add reactions to"));
        write(client_socket, "\n", strlen("\n"));
        return;
    }


    //extracting the id
    char id_string[] = "id=";
    char* id_pointer = strstr(path, id_string);

    //check if the field doesn't exist -- NULL
    if(!id_pointer || 
        *(id_pointer + strlen(id_string)) == '&' ||
        *(id_pointer + strlen(id_string)) == '\0')
    {
        snprintf(server_message, sizeof(server_message), "Invalid, id field cannot be empty\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //get the id into a string and null terminate it
    char id[7];
    id_pointer += 3;
    int i = 0;
    for(; i < 7 && id_pointer[i] != '&' && id_pointer[i] != '\0'; i++){
        id[i] = id_pointer[i];
    }
    id[i] = '\0';


    //check if the id --chat id-- is valid or not
    int chat_id = atoi(id) - 1;
    if(chat_id < 0 || chat_id >= chatList->size){
        snprintf(server_message, sizeof(server_message), "Invalid id--chat with specified id does not exist\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //if the id is valid, we now check for the number of reactions in the given chat
    if(chatList->chat[atoi(id)-1].num_reactions >= 100){
        snprintf(server_message, sizeof(server_message), "Max number of reactions reached (100) - cannot add more\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }



    //if id and number of reactions are vlid, then we take care of separating the r-user and the r-message

    //extracting the username
    char ruser_string[] = "user=";
    char *ruser_pointer = strstr(path, ruser_string);

    //check if the field does not exist
    if(!ruser_pointer ||
        *(ruser_pointer + strlen(ruser_string)) == '&' ||
        *(ruser_pointer + strlen(ruser_string)) == '\0')
    {
        snprintf(server_message, sizeof(server_message), "Invalid, user field cannot be empty\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //checking if the username length is bigger than 15 characters/bytes
    ruser_pointer += 5;
    i = 0;
    while(i < 16 && ruser_pointer[i] != '&' && ruser_pointer[i] != '\0'){
        i++;
    }

    //prints out error message if max length reached
    if(i == 16){
        snprintf(server_message, sizeof(server_message), "Username cannot be longer than 15 characters\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //if everything passes, copy it into a char array
    char ruser[16];
    int j = 0;
    for(; j < i; j++){
        ruser[j] = ruser_pointer[j];
    }
    ruser[j] = '\0';



    //same process, but for the message this time

    //extracting the message
    char rmessage_string[] = "message=";
    char *rmessage_pointer = strstr(path, rmessage_string);

    //check if the field does not exist
    if(!rmessage_pointer) {
        snprintf(server_message, sizeof(server_message), "Invalid, missing message field 'message=<message>'\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //checking if the username length is bigger than 15 characters/bytes
    rmessage_pointer += strlen(rmessage_string);
    i = 0;
    while(i < 16 && rmessage_pointer[i] != '&' && rmessage_pointer[i] != '\0'){
        i++;
    }

    //prints out error message if max length reached
    if(i == 16){
        snprintf(server_message, sizeof(server_message), "Reaction message cannot be longer than 15 characters\n");
        write(client_socket, HTTP_500_INTERNAL_SERVER, strlen(HTTP_500_INTERNAL_SERVER));
        write(client_socket, server_message, strlen(server_message));
        return;
    }

    //if everything passes, copy it into a char array
    char rmessage[16];
    j = 0;
    for(; j < i; j++){
        rmessage[j] = rmessage_pointer[j];
    }
    rmessage[j] = '\0';



    //convert string to int before passing in the method and then prints out all the chats with the new reaction
    add_reaction(ruser, rmessage, atoi(id)-1);
    responds_with_chat(client_socket, path);
}


/**
 * Handles /reset request
 * 
 * Resets everything
 * Frees all used heap memory
 */
void handle_reset(int client_socket, char* path){
    // Check if chatList is initialized
    if (chatList != NULL) {
        // Free each chat and its reactions
        for (int i = 0; i < chatList->size; i++) {
            // Free reactions array if it exists
            if (chatList->chat[i].reactions != NULL) {
                free(chatList->chat[i].reactions);
            }
        }
        
        // Free the chat array
        free(chatList->chat);
        
        // Free the chatList structure itself
        free(chatList);
    }
    
    // Reset global variables to initial state
    chatList = NULL;
    chat_id = 0;

    write(client_socket, HTTP_200_OK, strlen(HTTP_200_OK));
}



uint8_t hex_to_byte(char c) {
	if ('0' <= c && c <= '9') return c - '0';
	if ('a' <= c && c <= 'f') return c - 'a' + 10;
	if ('A' <= c && c <= 'F') return c - 'A' + 10;
	return -1;

}

void url_decode(char *src, char *dest){
	int i = 0;
	char *dest_ptr = dest;
	while (*(src+i) != 0){
		if(*(src+i)=='%'){
			uint8_t a = hex_to_byte(*(src+i+1));
		       	uint8_t b = hex_to_byte(*(src+i+2));
			*dest_ptr=(unsigned char)((a<< 4)| b);
			dest_ptr++;
			i +=2; 
		} else {
			*dest_ptr = *(src+i);
			dest_ptr++;

		}	
		i++;
	}
	*dest_ptr = 0;
}

/**
 * Handles actions based on the path type:
 * 
 * /chats --> prints out all chats
 * /post --> posts the chat and prints out all chats
 * /react --> adds a new reaction in the given chat id
 * /reset --> removes everything and frees the memory
 * 
 */
void handle_response(char *request, int client_socket){
    char path[300];
    char path_decoded[300];

    printf("\nSERVER LOG: Got request: \%s\"\n", request);

    //parse the path out of the request line (limit buffer size, sscanf null-terminater)
    if(sscanf(request, "GET %s", path) != 1){
        printf("Invalid request line\n");
        return;
    }


    url_decode(path, path_decoded);

    /**
     * "/" show current number
     * "/increment" increment the current number and then show
     * 
     * Format for each request type:
     * 
     * /chats
     * /post?user=<username>&message=<message>
     * /react?user=<username>&message=<reaction>&id=<id>
     * /reset
     * 
     */
    if(strcmp(path_decoded, "/") == 0){
        handle_root(client_socket, path_decoded);
        return;
    }
    else if(strncmp(path_decoded, "/chats", 5) == 0){
        printf("All chats and reactions printed\n");
        handle_chat(client_socket, path_decoded);
        return;
    }
    else if(strncmp(path_decoded, "/post", 5) == 0){
        printf("/post request: Will handle post request here\n");
        handle_post(client_socket, path_decoded);
        return;
    }
    else if(strncmp(path_decoded, "/react", 6) == 0){
        printf("/react request: will add reaction to given id\n");
        handle_react(client_socket, path_decoded);
        return;
    }
    else if(strncmp(path_decoded, "/reset", 6) == 0){
        printf("/reset request: will reset the whole server\n");
        handle_reset(client_socket, path_decoded);
        return;
    }
    else{
        handle_404(client_socket, path_decoded);
    }
}


/**
 * The main function
 */
int main(int argc, char* argv[]){
    int port = 0;
    if(argc >= 2){
        port = atoi(argv[1]);
    }

    start_server(&handle_response, port);
}