#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MESSAGE_SIZE 80

/** Used for the like list which is held by each message **/
typedef struct like_node {
    int timestamp;
    int server_index;
    char * username;
    struct like_node * next;
} like_node;

/** Used for the message list which is held by each chatroom **/
typedef struct message_node {
    int timestamp;
    int server_index;
    char message[MAX_MESSAGE_SIZE];
    char * chatroom_name;
    struct like_node * like_head;
    struct message_node * next;
} message_node;

/** Used for the chatroom list which is held by each server **/
typedef struct chatroom_node {
    char * chatroom_name;
    struct message_node * mess_head;
    struct chatroom_node * next;
} chatroom_node;

typedef struct lamport_timestamp {
    int server_index;
    int timestamp;
} lamport_timestamp;



/** Method declarations **/
int add_chatroom(char * new_name);
int add_message(char * new_mess, char * room_name, lamport_timestamp ts);
int add_like(char * user, message_node * mess, lamport_timestamp ts);

struct chatroom_node * chatroom_head;

int main() {
    return 0;
}

int add_chatroom(char * new_name) {
    //create chatroom to add
    struct chatroom_node * to_add = malloc(sizeof(chatroom_node));

    if (to_add == NULL)
        return 0;

    to_add->chatroom_name = new_name;

    //if head is null, set head to node to_add
    if (chatroom_head == NULL) {
        chatroom_head = to_add;
        return 1;
    }

<<<<<<< HEAD
    chatroom_node * curr = chatroom_head;

    /*
     * check for duplicates
    while (curr == NULL) {
        if (strcmp(curr->chatroom_name, new_name))
            return 0;
        curr = curr->next;
    }
    curr = chatroom_head;
     */

    //else find next null, add to end, return if duplicate found
=======
    //else find next null, add to end
    chatroom_node * curr = chatroom_head;
>>>>>>> 7170ad370f70aa7be51a41255239b11abc281a42
    while (curr->next != NULL) {
        if (strcmp(curr->chatroom_name, new_name) == 0) {
            return 0;
        }
        curr = curr->next;
    }

    //TODO fix hack to find duplicate
    if (strcmp(curr->chatroom_name, new_name) == 0) {
        return 0;
    }

    curr->next = to_add;

    return 1;
}

int add_message(char * new_mess, char * room_name, lamport_timestamp ts) {

    struct chatroom_node * curr_room = chatroom_head;
    struct message_node * curr_mess;
    int lamport = (10 * ts.timestamp) + (ts.server_index);

    /*make chatroom guaranteed to exist*/
    add_chatroom(room_name);

    /*look for correct chatroom*/
    while (strcmp(curr_room->chatroom_name, room_name) == 0) {
        curr_room = curr_room->next;
    }

    /*create message to add*/
    message_node * to_add = malloc(sizeof(message_node));
    to_add->server_index = ts.server_index;
    to_add->timestamp = ts.timestamp;
    to_add->chatroom_name = room_name;

    /*iterate through messages in room*/
    curr_mess = curr_room->mess_head;

    /*if head doesn't exist add, return*/
    if (curr_mess == NULL) {
        curr_room->mess_head = to_add;
        return 1;
    }

    int curr_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);

    if (lamport < curr_lamport) {
        to_add->next = curr_room->mess_head;
        curr_room->mess_head = to_add;
        return 1;
    }

    curr_lamport = (10 * curr_mess->next->timestamp) + (curr_mess->next->server_index);

    while (lamport > curr_lamport && curr_mess->next != NULL) {
        curr_mess = curr_mess->next;
        curr_lamport = (10 * curr_mess->next->timestamp) + (curr_mess->next->server_index);
    }

    if (curr_mess->next == NULL) {
        curr_mess->next = to_add;
        return 1;
    }
    to_add->next = curr_mess->next;
    curr_mess->next = to_add;
    return 1;

}

int add_like(char * user, message_node * mess, lamport_timestamp ts) {
    like_node * curr_like = mess->like_head;
    int lamport = (10 * ts.timestamp) + (ts.server_index);
    
    like_node * to_add = malloc(sizeof(like_node));
    to_add->server_index = ts.server_index;
    to_add->timestamp = ts.timestamp;
    strcpy(to_add->username, user);

    if (curr_like == NULL) {
        mess->like_head = to_add;
        return 1;
    }

    /*check for duplicates*/
    while (curr_like != NULL) {
        if (strcmp(curr_like->username, user)) {
            return 0;
        }
        curr_like = curr_like->next;
    }
    curr_like = mess->like_head;

    /*insert by lamport*/
    int curr_lamport = (10 * curr_like->timestamp) + (curr_like->server_index);

    if (lamport < curr_lamport) {
        to_add->next = mess->like_head;
        mess->like_head = to_add;
        return 1;
    }

    curr_lamport = (10 * curr_like->next->timestamp) + (curr_like->next->server_index);

    while (lamport > curr_lamport && curr_like->next != NULL) {
        curr_like = curr_like->next;
        curr_lamport = (10 * curr_like->next->timestamp) + (curr_like->next->server_index);
    }

    if (curr_like->next == NULL) {
        curr_like->next = to_add;
        return 1;
    }
    to_add->next = curr_like->next;
    curr_like->next = to_add;
    return 1;
}
