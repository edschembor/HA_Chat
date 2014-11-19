#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MESSAGE_SIZE 80
#define MAX_USERNAME_SIZE 20

typedef struct lamport_timestamp {
    int server_index;
    int timestamp;
} lamport_timestamp;

/** Used for the like list which is held by each message **/
typedef struct like_node {
    int timestamp;
    int server_index;
    struct lamport_timestamp mess_timestamp;
    char username[MAX_USERNAME_SIZE];
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


/** Method declarations **/
int add_chatroom(char * new_name);
int add_message(char * new_mess, char * room_name, lamport_timestamp ts);
int add_like(char * user, lamport_timestamp ts, lamport_timestamp mess_ts, char * room);
void print_chatrooms();
void print_messages(char * room);
void print_likes(char * room, lamport_timestamp ts);

struct chatroom_node * chatroom_head;

//TEST METHODS
int main() {
    lamport_timestamp ts;
    ts.server_index = 1;
    ts.timestamp = 1;
    add_message("hia", "room1", ts);
    ts.timestamp = 3;
    add_message("hib", "room1", ts);
    ts.timestamp = 2;
    add_message("hic", "room1", ts);
    ts.timestamp = 5;
    add_message("hid", "room1", ts);
    print_chatrooms();
    print_messages("room1");

    ts.server_index = 2;
    ts.timestamp = 1;
    add_message("byea", "room2", ts);
    ts.timestamp = 3;
    add_message("byeb", "room2", ts);
    ts.timestamp = 2;
    add_message("byec", "room2", ts);
    ts.timestamp = 5;
    add_message("byed", "room2", ts);
    print_chatrooms();
    print_messages("room2");


    lamport_timestamp new_ts;
    new_ts.server_index = 1;
    new_ts.timestamp = 1;
    ts.timestamp = 3;
    add_like("jan", new_ts, ts, "room2");
    new_ts.timestamp++;
    add_like("jan", new_ts, ts, "room2");
    new_ts.timestamp++;
    add_like("jan1", new_ts, ts, "room2");
    new_ts.timestamp++;
    add_like("jan2", new_ts, ts, "room2");
    new_ts.timestamp++;
    add_like("jan3", new_ts, ts, "room2");
    new_ts.timestamp++;
    add_like("jan4", new_ts, ts, "room2");
    new_ts.timestamp = 8;
    add_like("jan5", new_ts, ts, "room2");
    new_ts.timestamp = 7;
    add_like("jan6", new_ts, ts, "room2");
    print_likes("room2", ts);
    return 0;
}

void print_chatrooms() {
    chatroom_node * curr = chatroom_head;
    printf("\n[");
    while (curr != NULL) {
        printf("%s, ", curr->chatroom_name);
        curr = curr->next;
    }
    printf("]\n");
}

void print_messages(char * room) {
    chatroom_node * temp = chatroom_head;
    while (strcmp(temp->chatroom_name, room) != 0) {
        temp = temp->next;
    }
    message_node * curr = temp->mess_head;
    printf("\n[");
    while (curr != NULL) {
        printf("timestamp: %d -- message: %s, ", (curr->timestamp * 10) + (curr->server_index), curr->message);
        curr = curr->next;
    }
    printf("]\n");
}


void print_likes(char * room, lamport_timestamp ts) {
    chatroom_node * temp = chatroom_head;
    while (strcmp(temp->chatroom_name, room) != 0) {
        temp = temp->next;
    }
    message_node * curr_mess = temp->mess_head;
    int lamport = (10 * ts.timestamp) + (ts.server_index);
    int mess_lam = (10 * curr_mess->timestamp) + (curr_mess->server_index);
    while (lamport != mess_lam) {
        curr_mess = curr_mess->next;
        mess_lam = (10 * curr_mess->timestamp) + (curr_mess->server_index);
    }

    like_node * curr_like = curr_mess->like_head;
    printf("\nMessage Timestamp: %d\nLikes:\n-----------------\n", lamport);
    printf("[");
    while (curr_like != NULL) {
        printf("timestamp: %d -- user: %s, ", (curr_like->timestamp * 10) + (curr_like->server_index), curr_like->username);
        curr_like = curr_like->next;
    }
    printf("]\n");
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
    //check for duplicates
    while (curr != NULL) {
        if (strcmp(curr->chatroom_name, new_name) == 0)
            return 0;
        curr = curr->next;
    }
    curr = chatroom_head;

    //else find next null, add to end, return if duplicate found
    while (curr->next != NULL) {
        if (strcmp(curr->chatroom_name, new_name) == 0) {
            return 0;
        }
        curr = curr->next;
    }

    //TODO fix hack to find duplicate
    //if (strcmp(curr->chatroom_name, new_name) == 0) {
    //    return 0;
    //}

    curr->next = to_add;

    return 1;
}

int add_message(char * new_mess, char * room_name, lamport_timestamp ts) {
    /*make chatroom guaranteed to exist*/
    add_chatroom(room_name);

    struct chatroom_node * curr_room = chatroom_head;
    struct message_node * curr_mess;
    struct message_node * prev_mess;
    int lamport = (10 * ts.timestamp) + (ts.server_index);

    /*look for correct chatroom*/
    while (strcmp(curr_room->chatroom_name, room_name) != 0) {
        curr_room = curr_room->next;
    }

    /*create message to add*/
    message_node * to_add = malloc(sizeof(message_node));
    to_add->server_index = ts.server_index;
    to_add->timestamp = ts.timestamp;
    to_add->chatroom_name = room_name;
    strcpy(to_add->message, new_mess);

    /*iterate through messages in room*/
    curr_mess = curr_room->mess_head;

    /*if head doesn't exist add, return*/
    if (curr_mess == NULL) {
        curr_room->mess_head = to_add;
        return 1;
    }

    int curr_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);

    /*TODO necessary? if 1 element in list base case*/
    if (curr_mess->next == NULL) {
        if (lamport < curr_lamport) {
            to_add->next = curr_room->mess_head;
            curr_room->mess_head = to_add;
        } else {
            curr_room->mess_head->next = to_add;
        }
        return 1;
    }

    while (lamport > curr_lamport) {
        prev_mess = curr_mess;
        curr_mess = curr_mess->next;
        if (curr_mess == NULL) //FIXME i hate this.
            break;
        curr_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);
    }

    if (curr_mess == NULL) {
        prev_mess->next = to_add;
        return 1;
    }
    to_add->next = curr_mess;
    prev_mess->next = to_add;
    return 1;

}

int add_like(char * user, lamport_timestamp ts, lamport_timestamp mess_ts, char * room) {
    /*if chatroom list doesn't exist, return 0*/
    if (chatroom_head == NULL) {
        return 0;
    }

    chatroom_node * curr = chatroom_head;
    message_node * curr_mess;
    int mess_lamport = (mess_ts.timestamp * 10) + (mess_ts.server_index);
    int curr_mess_lamport = 0;
    like_node * curr_like;
    like_node * prev_like;
    int lamport;

    /*find chatroom*/
    while (strcmp(curr->chatroom_name, room) != 0) {
        curr = curr->next;
        /*if chatroom doesn't exist, return 0*/
        if (curr == NULL) {
            return 0;
        }
    }

    /*if message list for chatroom doesn't exist, return 0*/
    if (curr->mess_head == NULL) {
        return 0;
    }

    curr_mess = curr->mess_head;

    /*find correct message*/
    while (mess_lamport != curr_mess_lamport) {
        curr_mess = curr_mess->next;
        if (curr_mess == NULL) { //if message doesn't exist, return
            return 0;
        }
        curr_mess_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);
    }

    curr_like = curr_mess->like_head;
    lamport = (10 * ts.timestamp) + (ts.server_index);

    /*create like to add to list*/
    like_node * to_add = malloc(sizeof(like_node));
    to_add->server_index = ts.server_index;
    to_add->timestamp = ts.timestamp;
    strcpy(to_add->username, user);

    /*if head doesn't exist add, return*/
    if (curr_like == NULL) {
        curr_mess->like_head = to_add;
        return 1;
    }

    /*check for duplicates*/
    while (curr_like != NULL) {
        if (strcmp(curr_like->username, user) == 0) {
            return 0;
        }
        curr_like = curr_like->next;
    }
    curr_like = curr_mess->like_head;

    int curr_lamport = (10 * curr_like->timestamp) + (curr_like->server_index);

    /*TODO necessary? if 1 element in list base case*/
    if (curr_like->next == NULL) {
        if (lamport < curr_lamport) {
            to_add->next = curr_mess->like_head;
            curr_mess->like_head = to_add;
        } else {
            curr_mess->like_head->next = to_add;
        }
        return 1;
    }

    while (lamport > curr_lamport) {
        prev_like = curr_like;
        curr_like = curr_like->next;
        if (curr_like == NULL) //FIXME i hate this.
            break;
        curr_lamport = (10 * curr_like->timestamp) + (curr_like->server_index);
    }

    if (curr_like == NULL) {
        prev_like->next = to_add;
        return 1;
    }
    to_add->next = curr_like;
    prev_like->next = to_add;
    return 1;


}
