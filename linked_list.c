#include <stdio.h>
#include <stdlib.h>

#define MAX_MESSAGE_SIZE 80

typedef struct like_node {
    int lamport_counter;
    int service_index;
    char * username;
    struct like_node * next;
} like_node;

typedef struct message_node {
    int timestamp;
    int server_index;
    char message[MAX_MESSAGE_SIZE];
    char * chatroom_name;
    struct like_node * like_head;
    struct message_node * next;
} message_node;

typedef struct chatroom_node {
    char * chatroom_name;
    struct message_node * mess_head;
    struct chatroom_node * next;
} chatroom_node;

typedef struct lamport_timestamp {
    int server_index;
    int timestamp;
} lamport_timestamp;

int add_chatroom(char * new_name);
int add_message(char * new_mess, char * room_name, lamport_timestamp ts);
int add_like(message_node * mess, like_node * new_like, lamport_timestamp ts);

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

    chatroom_node * curr = chatroom_head;
    //else find next null, add to end
    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = to_add;

    return 1;
}

/*FIXME IS A MESS*/
int add_message(char * new_mess, char * room_name, lamport_timestamp ts) {

    struct chatroom_node * curr_room = chatroom_head;
    struct chatroom_node * prev_room;
    struct message_node * curr_mess;

    /*look for correct chatroom*/
    while (curr_room != NULL && strcmp(curr_room->chatroom_name, room_name) == 0) {
        prev_room = curr_room;
        curr_room = curr_room->next;
    }

    if (curr == NULL) {
        add_chatroom(room_name);
    }

    /*create message to add*/
    message_node * to_add = malloc(sizeof(message_node));
    to_add->server_index = ts.server_index;
    to_add->timestamp = ts.timestamp;
    to_add->chatroom_name = room_name;

    curr_mess = curr_room->mess_head;
    if (curr_mess == NULL) {
        prev_room->next = to_add;
        return 1;
    }

    while (curr_mess->next != NULL) {
        curr_mess = curr_mess->next;
    }

    curr_mess->next = to_add;
    return 1;
}

int add_like(message_node * mess, like_node * new_like, lamport_timestamp ts) {
    return 0;
}
