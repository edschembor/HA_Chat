#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_list.c"

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
	char author[20];
	int num_likes;
} message_node;

/** Used for the chatroom list which is held by each server **/
typedef struct chatroom_node {
    char chatroom_name[80];
	struct user_node * user_list_head; //The linked list of users currently in the chatroom
    struct message_node * mess_head;
    struct chatroom_node * next;
} chatroom_node;


/** Method declarations **/
int add_chatroom(char * new_name);
message_node* add_message(char * new_mess, char * room_name, lamport_timestamp ts);
message_node* like(char * user, lamport_timestamp ts, lamport_timestamp mess_ts, char * room);

int remove_chatroom(char * name);
int remove_message(char * room_name, lamport_timestamp ts);
message_node* unlike(char * user, lamport_timestamp mess_ts, char * room);

struct chatroom_node * chatroom_head;

int add_chatroom(char * new_name) {
    //create chatroom to add
    struct chatroom_node * to_add = malloc(sizeof(chatroom_node));

    if (to_add == NULL)
        return 0;

	strcpy(to_add->chatroom_name, new_name);

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

message_node* add_message(char * new_mess, char * room_name, lamport_timestamp ts) {
    /*make chatroom guaranteed to exist*/
    add_chatroom(room_name);

    struct chatroom_node * curr_room = chatroom_head;
    struct message_node * curr_mess;
    struct message_node * prev_mess;
    int lamport = (10 * ts.timestamp) + (ts.server_index);
	int curr_lamport;

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

    curr_mess = curr_room->mess_head;

    /*if head doesn't exist add, return*/
    if (curr_mess == NULL) {
        curr_room->mess_head = to_add;
        return to_add;
    }

    if (curr_mess->next == NULL) {
        if (lamport <= curr_lamport) {
            to_add->next = curr_room->mess_head;
            curr_room->mess_head = to_add;
        } else {
            curr_room->mess_head->next = to_add;
        }
        return to_add;
    }
	
	while (curr_mess->next != NULL) {
        curr_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);

		if (lamport < curr_lamport) {
			to_add->next = curr_mess->next->next;
			curr_mess->next = to_add;
			break;
		}
		curr_mess = curr_mess->next;
	}

	curr_mess->next = to_add;

    return to_add;

}

message_node* like(char * user, lamport_timestamp ts, lamport_timestamp mess_ts, char * room) {
    /*if chatroom list doesn't exist, return 0*/
    if (chatroom_head == NULL) {
        return NULL;
    }

    chatroom_node * curr = chatroom_head;
    message_node * curr_mess;
    int mess_lamport = (mess_ts.timestamp * 10) + (mess_ts.server_index);
    int curr_mess_lamport;
    like_node * curr_like;
    like_node * prev_like;
    int lamport;

    /*find chatroom*/
    while (strcmp(curr->chatroom_name, room) != 0) {
        curr = curr->next;
        /*if chatroom doesn't exist, return 0*/
        if (curr == NULL) {
            return NULL;
        }
    }

    /*if message list for chatroom doesn't exist, return 0*/
    if (curr->mess_head == NULL) {
        return NULL;
    }

    curr_mess = curr->mess_head;
    curr_mess_lamport = (10 * curr_mess->timestamp) + (curr_mess->server_index);
    /*find correct message*/
    while (mess_lamport != curr_mess_lamport) {
        curr_mess = curr_mess->next;
        if (curr_mess == NULL) { //if message doesn't exist, return
            return NULL;
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
		curr_mess->num_likes++;
        return curr_mess;
    }

    /*check for duplicates*/
    while (curr_like != NULL) {
        if (strcmp(curr_like->username, user) == 0) {
            return NULL;
        }
        curr_like = curr_like->next;
    }
    curr_like = curr_mess->like_head;

    int curr_lamport = (10 * curr_like->timestamp) + (curr_like->server_index);

    if (curr_like->next == NULL) {
        if (lamport < curr_lamport) {
            to_add->next = curr_mess->like_head;
            curr_mess->like_head = to_add;
        } else {
            curr_mess->like_head->next = to_add;
        }
		curr_mess->num_likes++;
        return curr_mess;
    }

    while (lamport > curr_lamport) {
        prev_like = curr_like;
        curr_like = curr_like->next;
        if (curr_like == NULL)
            break;
        curr_lamport = (10 * curr_like->timestamp) + (curr_like->server_index);
    }

    if (curr_like == NULL) {
        prev_like->next = to_add;
		curr_mess->num_likes++;
        return curr_mess;
    }
    to_add->next = curr_like;
    prev_like->next = to_add;
	curr_mess->num_likes++;
    return curr_mess;
}

int remove_chatroom(char * name) {
    chatroom_node * head = chatroom_head;
    chatroom_node * to_remove;
    /**empty list**/
    if (head == NULL) {
        return 0;
    }

    /**check first element**/
    if (strcmp(head->chatroom_name, name) == 0) {
        chatroom_head = head->next;
        head->next = NULL;
        free(head);
        return 1;
    }

    while (head->next != NULL) {
        if (strcmp(head->next->chatroom_name, name) == 0) {
            to_remove = head->next;
            head->next = head->next->next;
            to_remove->next = NULL;
            free(to_remove);
            return 1;
        }
        head = head->next;
    }
    return 0;
}

int remove_message(char * room_name, lamport_timestamp ts) {
    chatroom_node * head = chatroom_head;
    message_node * curr_mess;
    message_node * to_remove;

    while (head != NULL) {
        if (strcmp(head->chatroom_name, room_name) == 0) {
            curr_mess = head->mess_head;
            break;
        }
        head = head->next;
    }

    if (head == NULL || curr_mess == NULL) {
        return 0;
    }

    //if (curr_mess->timestamp == ts.timestamp && curr_mess->server_index == ts.server_index) {
    if ((curr_mess->timestamp * 10) + curr_mess->server_index == (ts.timestamp * 10) + ts.server_index) {
        head->mess_head = head->mess_head->next;
        curr_mess->next = NULL;
        free(curr_mess);
    }

    while (curr_mess->next != NULL) {

        if ((curr_mess->next->timestamp * 10) + curr_mess->next->server_index == (ts.timestamp * 10) + ts.server_index) {
            to_remove = curr_mess->next;
            curr_mess->next = curr_mess->next->next;
            to_remove->next = NULL;
            free(to_remove);
            return 1;
        }
        curr_mess = curr_mess->next;
    }
    return 0;
}

message_node* unlike(char * user, lamport_timestamp mess_ts, char * room) {
    chatroom_node * head = chatroom_head;
    message_node * curr_mess;
    like_node * curr_like;
    like_node * to_remove;

    if (head == NULL) {
        return NULL;
    }

    while (head != NULL) {
        if (strcmp(head->chatroom_name, room) == 0) {
            curr_mess = head->mess_head;
            break;
        }
        head = head->next;
    }

    if (head == NULL || curr_mess == NULL) {
        return NULL;
    }

    while (curr_mess != NULL) {
        if (((curr_mess->timestamp * 10) + curr_mess->server_index) == ((mess_ts.timestamp * 10) + mess_ts.server_index)) {
            curr_like = curr_mess->like_head;
            break;
        }
        curr_mess = curr_mess->next;
    }

    if (curr_mess == NULL || curr_like == NULL) {
        return NULL;
    }

    if (strcmp(user, curr_like->username) == 0) {
        curr_mess->like_head = curr_mess->like_head->next;
        curr_like->next = NULL;
        free(curr_like);
		curr_mess->num_likes--;
        return curr_mess;
    }

    while (curr_like->next != NULL) {
        if(strcmp(curr_like->next->username, user) == 0) {
            to_remove = curr_like->next;
            curr_like->next = curr_like->next->next;
            to_remove->next = NULL;
            free(to_remove);
			curr_mess->num_likes--;
            return curr_mess;
        }
        curr_like = curr_like->next;
    }

    return NULL;
}
