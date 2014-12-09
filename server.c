/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Server Program
 *
 */

#include "data_structure.c"
#include "sp.h"
#include <stdio.h>
#include <stdlib.h>
#include "update.h"
#include "update_array.c"

#define MAX_NAME 80
#define SERVER_GROUP_NAME "servers"
#define SPREAD_NAME "10030"
#define NUM_SERVERS 5
#define MAX_MEMBERS 100
#define MAX_MESSLEN 1400

/** Method Declarations **/
static void Handle_messages();
void Send_Merge_Updates();
int Is_Max(int[]);
int Min_Val(int[]);
void Send_All_Messages(char *, char *);
//void Send_Recent_TwentyFive(char *);
void Clear_Updates();
void Compare_Lamport();
void Send_Server_View();
char* chatroom_to_local(char[]);
char* chatroom_sans_index(char chatroom[]);

/** Global Variables **/
static int       machine_index;
int              group_status[NUM_SERVERS];
int              prev_group_status[NUM_SERVERS];

static char      Spread_name[MAX_NAME] = SPREAD_NAME;
static char      Private_group[MAX_NAME];
static char      server_group[MAX_NAME] = SERVER_GROUP_NAME;
static char      User[MAX_NAME] = "1";
char             current_group[NUM_SERVERS][MAX_GROUP_NAME];
int              joined_default = 0;

static mailbox   Mbox;
int              ret;
int              num_groups;
char             chatroom[MAX_NAME];
char             default_group[MAX_NAME];
char             individual_group[MAX_NAME];

int              entropy_matrix[NUM_SERVERS][NUM_SERVERS];
int              entropy_received = 0;
int              waiting_on;
int              burst_size = 100;
int              received = 0;
int              merging = 0;

int              lamport_counter;
char*            local;

user_node        *to_change;

update_array            updates[NUM_SERVERS]; //Update struct for each server
//Note: The main data structure run in data_structure.c

int main(int argc, char *argv[])
{
	/** Check usage and set command line variables **/
	if(argc != 2) {
		printf("\nUsage: server <server_index>\n");
		exit(0);
	}
	machine_index = atoi(argv[1]);

	/** Set up timeout for Spread connection **/
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;

	/** Set up the server name **/
	strcpy(individual_group, "server");
	char *machine_index_str;
	char *spread_username;

	sprintf(machine_index_str, "%d", machine_index);
	strcat(individual_group, machine_index_str);

	/** Connect to the Spread client **/
	ret = SP_connect_timeout( Spread_name, machine_index_str, 0, 1, &Mbox, Private_group,
		test_timeout);
	if(ret != ACCEPT_SESSION)
	{
		SP_error(ret);
		exit(0);
	}

	/** Join the server group **/
	ret = SP_join(Mbox, server_group);
	if(ret < 0) SP_error(ret);

	/** Join the default client group **/
	strcpy(default_group, "default");
	strcat(default_group, machine_index_str);
	ret = SP_join(Mbox, default_group);
	if(ret < 0) SP_error(ret);

	/** Join the servers individual group **/
	ret = SP_join(Mbox, individual_group);
	if(ret < 0) SP_error(ret);

	/** Initialize the udpate arrays **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		updates[i].size = INITIAL_SIZE;
		updates[i].array = malloc(sizeof(update)*INITIAL_SIZE);
	}
	local = malloc(sizeof(char[80]));

	/* Process and send messages using the Spread Event System */
	E_init();

	E_attach_fd(Mbox, READ_FD, Handle_messages, 0, NULL, HIGH_PRIORITY);

	//E_attach_fd(0, READ_FD, -----, 0, NULL, LOW_PRIORITY);

	E_handle_events();
}

static void Handle_messages()
{

	/** Initialize locals **/
	char          target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	update        received_update;
	int           endian_mismatch;
	int           service_type;
	char          sender[MAX_GROUP_NAME];
	int16         mess_type;
	char          mess[MAX_MESSLEN];
	message_node  *changed_message;
	char          *update_between_servers;
	
    changed_message = malloc(sizeof(message_node));
	update_between_servers = malloc(sizeof(update));
	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups,
		target_groups, &mess_type, &endian_mismatch, MAX_MESSLEN, mess);

	if(ret < 0) {
		return;
	}
	
	if(Is_regular_mess( service_type )) {
		
		//Cast the message to an update
		received_update = *((update *) mess);
		update_between_servers = mess;

		int test = 0;
		if(strcmp(received_update.user, "") == 0) {
			return;
		}

		/** Check if it is an unlike **/
		if(received_update.type == -1) {
			
			//Stamp the message
			received_update.lamport.timestamp = ++lamport_counter;
			received_update.lamport.server_index = machine_index;
			
			//Perform the unlike update on linked list
			changed_message = unlike(received_update.user, received_update.liked_message_lamp, 
				chatroom_sans_index(received_update.chatroom));

			if(changed_message == NULL) {
				return;
			}

			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers if the update was from a client
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					update_between_servers);
			}

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1, MAX_MESSLEN, 
				(char *) changed_message);	
		}

		/** Check if it is a like **/
		else if(received_update.type == 1) {
			
			//Stamp the message
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				received_update.lamport.timestamp = ++lamport_counter;
				received_update.lamport.server_index = machine_index;
			}

			//Perform the like update
			changed_message = like(received_update.user, received_update.lamport, 
			received_update.liked_message_lamp, chatroom_sans_index(received_update.chatroom));
            
            if (changed_message == NULL) {
                return;
            }

			//changed_message->timestamp = received_update.lamport.timestamp;
			//changed_message->server_index = received_update.lamport.server_index;

			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector	
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1, MAX_MESSLEN,
				(char *) changed_message);
		}

		/** Check if it is a chat message **/
		else if(received_update.type == 0) {

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				received_update.lamport.timestamp = ++lamport_counter;
				received_update.lamport.server_index = machine_index;
			}
			
                //Perform the new message update
			changed_message = add_message(received_update.message, chatroom_sans_index(received_update.chatroom), 
				received_update.lamport);

			strcpy(changed_message->author, received_update.user);	
			changed_message->timestamp = received_update.lamport.timestamp;
			changed_message->server_index = received_update.lamport.server_index;
	
			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);
			
			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1, MAX_MESSLEN,
				(char *) changed_message);
		}

		/** Check if it is an entropy vector for merging **/
		else if(received_update.type == 2) {
	
			printf("\nGOT A VECTOR\n");

			//Update your matrix with the received message
			int origin = received_update.lamport.server_index;
			for(int i = 0; i < NUM_SERVERS; i++) {
				entropy_matrix[origin][i] = received_update.vector[i];
			}

			//Need: Deal with mid partition - ie) look for membership change
			
			//If all have been received, send out updates if you are the max
			if(++entropy_received == waiting_on) {
				printf("\nABOUT TO MERGE\n");
				Send_Merge_Updates();
			}
			
			//Update your local vector
			for(int i = 0; i < NUM_SERVERS; i++) {
				entropy_matrix[machine_index-1][i] = entropy_matrix[i][i];
			}
			//Clear_Updates();
		}

		/** Check if its a complete history request **/
		else if(received_update.type == 3) {
			//Send the complete history to the client which requested it
			printf("\nREQUEST FOR HISTORY\n");
			Send_All_Messages(received_update.chatroom, sender);
		}

		/** Check if its a chatroom join message **/
		else if(received_update.type == 5) {
			ret = SP_join(Mbox, received_update.chatroom);

			/** Update the chatroom's users **/
			//Get the chatroom node for the update's chatroom
			chatroom_node *tmp;
			
			if(chatroom_head == NULL) {
				add_chatroom(chatroom_sans_index(received_update.chatroom));
			}

			tmp = chatroom_head;

			while(tmp->next != NULL) {
				if(strcmp(tmp->chatroom_name, received_update.chatroom)) {
					break;
				}
				tmp = tmp->next;
			}
			
			//Create the to_change node
			to_change = malloc(sizeof(user_node));
			to_change->user = received_update.user;
			to_change->connected_server = machine_index;
			
			//Add it to the data structure
			add_user(tmp->user_list_head, to_change);

			/** Send update to other servers **/
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1,
					MAX_MESSLEN, update_between_servers);
			}else{
				return;
			}
			
			/** Send update to the clients - wait for membership? **/
			message_node *mess_to_send;
			mess_to_send = malloc(sizeof(message_node));
			mess_to_send->timestamp = -1;
			strcpy(mess_to_send->author, received_update.user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1,
				MAX_MESSLEN, (char *) mess_to_send);

			/** Send the new client who is in its chatroom **/
			chatroom_node *tmp_room = chatroom_head;
			//Get the chatroom node
			while(tmp_room->next != NULL) {
				if(strcmp(tmp_room->chatroom_name, received_update.chatroom) == 0) {
					break;
				}
				tmp_room = tmp_room->next;
			}
			//Send the user updates in the server's data structure to the new client
			user_node *tmp_user = tmp_room->user_list_head->next;
			strcpy(mess_to_send->author, tmp_user->user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, sender, 1, MAX_MESSLEN, 
				(char *) mess_to_send);
			while(tmp_user->next != NULL) {
				strcpy(mess_to_send->author, tmp_user->user);
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, sender, 1, MAX_MESSLEN, 
					(char *) mess_to_send);
				tmp_user = tmp_user->next;
			}
		}

		/** Check if its a chatroom leave message **/
		else if(received_update.type == 6) {

			/** Update the chatroom's users **/
			//Get the chatroom node for the update's chatroom
			chatroom_node *tmp;
			
			if(chatroom_head == NULL) {
				add_chatroom(chatroom_sans_index(received_update.chatroom));
			}

			tmp = chatroom_head;

			while(tmp->next != NULL) {
				if(strcmp(tmp->chatroom_name, received_update.chatroom)) {
					break;
				}
				tmp = tmp->next;
			}
			
			//Create the to_change node
			to_change->user = received_update.user;
			to_change->connected_server = machine_index;
			
			//Remove it from the data structure
			remove_user(tmp->user_list_head, to_change);
			
			/** Send update to other servers **/
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1,
					MAX_MESSLEN, update_between_servers);
			}
			
			/** Send update to clients **/
			message_node *mess_to_send;
			mess_to_send = malloc(sizeof(message_node));
			mess_to_send->timestamp = -2;
			strcpy(mess_to_send->author, received_update.user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1,
				MAX_MESSLEN, (char *) mess_to_send);
		}

		/** Deal with a "server view" request **/
		else if(received_update.type == 7) {
			changed_message->timestamp = -4;
			for(int i = 1; i < NUM_SERVERS+1; i++) {
				changed_message->message[i] = group_status[i];
			}
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, chatroom_to_local(received_update.chatroom), 1, MAX_MESSLEN, (char *) changed_message);
		}
			
	}else if( Is_membership_mess( service_type )) {

		/** Check if it was an update from the server group **/
		if(strcmp(sender, SERVER_GROUP_NAME) == 0) {

			/** Maintain the servers in your parition **/
			int server_index;
			int merge_case = 0;
			for(int i = 0; i < NUM_SERVERS; i++) {
				prev_group_status[i] = group_status[i];
				group_status[i] = 0;
			}
			for(int i = 0; i < num_groups; i++) {
				server_index = atoi(&target_groups[i][1]);
				group_status[server_index] = 1;
				if(!prev_group_status[server_index]) {
					merge_case = 1;
				}
			}

			/** Set the new view of chat servers in the current server's newtwork component**/
			for(int i = 0; i < NUM_SERVERS; i++) {
				for(int j = 0; j < MAX_GROUP_NAME; j++) { 
					current_group[i][j] = target_groups[i][j];
				}
			}

			//If it was an addition, merge
			if(merge_case) {
				printf("\n-------IN MERGE CASE---------\n");
				waiting_on = num_groups-1;
				/** Flow control merging = 1; **/
				//Send your anti-entropy vector as an update
				update *entropy_update = malloc(sizeof(update));
				entropy_update->type = 2;
				for(int i = 0; i < NUM_SERVERS; i++) {
					entropy_update->vector[i] = entropy_matrix[machine_index-1][i];
				}
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1, MAX_MESSLEN, 
					(char *) entropy_update);
				entropy_received = 0;
			}

		/** The update came from a chatroom group **/
		}else{
			/** Send the new client the recent 25 messages **/
			if(Is_caused_join_mess(service_type)) {
				
				/** Send the new client the recent 25 messages **/
				if(strcmp(sender, default_group) != 0) {
                    printf("\nI got here\n");
					Send_Recent_TwentyFive(sender);
				}
			}
		}
	}
}

void Send_Merge_Updates()
{


	#if 0
	/** TODO: Should it be max in group? **/
	/** Send necessary updates **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(Is_Max(entropy_matrix[i])) {
	 		for(int j = 0; j < NUM_SERVERS; j++) {
	 			//Send out the ones you have
				int current = updates[i].start;
				int s = updates[i].size;
				while(updates[i].array[current%s].lamport.timestamp < entropy_matrix[i][j]) {
					//Send the update to the machines which need it
					//TODO - Need to re-think this method

					//send(updates[i][current%s]; -> Use private group
					current++;
				}
	 		}
	 	}
	}
	#endif


	/** Current state: Everyone sends everything to everyone **/
	printf("\nMERGING!!!!!\n");
	update *update_to_send = malloc(sizeof(update));
	for(int i = 0; i < NUM_SERVERS; i++) {
		int s = updates[i].size;
		for(int j = 0; j < s; j++) {
			//Set update_to_send
			update_to_send = &updates[i].array[j];

			//Send the update
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1,
				MAX_MESSLEN, (char *) update_to_send);
			printf("\nSENT A MERGE UPDATE\n");
		}
	}

	/** Flow control **/
	/*if(merging == 1 && received >= burst_size) {
		TODO: Send shit
	}
	if(sent them all) {
		merging = 0;
	}*/
}


int Is_Max(int vector[])
{
	/** Checks if the server for the given anti-entropy vector is the server
	 *  with the most updates in its current partition **/
	int server_val = vector[machine_index - 1];
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(vector[i] > server_val)
			return 0;
		if(vector[i] == server_val) {
			if(i > machine_index-1)
				return 0;
		}
	}
	return 1;
}

int Min_Val(int vector[])
{
	/** Gets the minimum value in the array - for removing updates  **/
	int min = vector[0];
	for(int i = 1; i < NUM_SERVERS; i++)
		if(vector[i] < min)
			min = vector[i];

	return min;
}

void Send_All_Messages(char *room_name, char *client_private_group)
{
	/** Sends all the messages it has from a particular chatroom to a specific client
	 * connected to the server and in that chatroom **/
	
	struct chatroom_node * curr_room = chatroom_head;
	struct message_node * curr_mess;
	struct message_node * tmp;
	struct message_node * tmp2; //goes to 25th to last
	struct message_node * tmp3; //goes to last

	/** Look for correct chatroom **/
	while(strcmp(curr_room->chatroom_name, room_name) != 0) {
		
		//Deal with case if the chatroom is not in the data structure
		if(curr_room->next == NULL) {
			return;
		}
		curr_room = curr_room->next;
	}

	/** Iterate tmp3 25 spaces ahead **/
	tmp3 = curr_room->mess_head;
	tmp2 = curr_room->mess_head;
	int end = 0;
	while(tmp3 != NULL && end < 25) {
		tmp3 = tmp3->next;
		end++;
	}

	/** Iterate tmp3 and tmp2 until tmp3 is null **/
	while(tmp3 != NULL) {
		tmp2 = tmp2->next;
		tmp3 = tmp3->next;
	}

	/** Iterate through and print out until at tmp2**/
	curr_mess = curr_room->mess_head;
	while(curr_mess->next != tmp2) {
		//Multicast the message to the client which requested the messages
		tmp = curr_mess->next;
		curr_mess->next = NULL;
		printf("\nSENDING A MESSAGE\n");
		SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, client_private_group, 1, MAX_MESSLEN,
			(char *) curr_mess);
		curr_mess = tmp;
	}

	/** Send message noting that sending is gone **/
	curr_mess->timestamp = -3;
	printf("\nSent end message\n");
	SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, client_private_group, 1, MAX_MESSLEN,
		(char *) curr_mess);
}


void Send_Recent_TwentyFive(char *room_name)
{
	/** Sends the most recent 25 messages it has from a particular chatroom to a specific
	 * client connected to the server and in that chatroom **/

	struct chatroom_node * curr_room = chatroom_head;
	struct message_node * curr_mess;
	struct message_node * curr_mess_end;

	if(curr_room == NULL) {
		return;
	}

	/** Look for the correct chatroom **/
	while(strcmp(curr_room->chatroom_name, chatroom_sans_index(room_name)) != 0) {
		if(curr_room->next == NULL) {
			return;
		}
		curr_room = curr_room->next;
	}

	curr_mess = curr_room->mess_head;
	curr_mess_end = curr_room->mess_head;

	/** Move curr_mess_end 25 spaces ahead of curr_mess **/
	int end = 0;
	while(curr_mess_end != NULL && end < 25) {
		curr_mess_end = curr_mess_end->next;
		end++;
	}

	/** Move both pointers so we have one at 25th to last and one at last **/
	while(curr_mess_end != NULL) {
		curr_mess_end = curr_mess_end->next;
		curr_mess = curr_mess->next;
	}

	/** Send the last 25 messages **/
	while(curr_mess != NULL) {
		SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, room_name, 1, MAX_MESSLEN, 
			(char *) curr_mess);
		curr_mess = curr_mess->next;
	}
}

void Clear_Updates()
{
	/** Sets updates which are no longer needed to type = -2 in the udpates array**/
	int min = 0;
	int current;
	int size;
	for(int i = 0; i < NUM_SERVERS; i++) {
		min = Min_Val(entropy_matrix[i]);
		current = updates[i].start;
		size = updates[i].size;
		while(updates[i].array[current%size].lamport.timestamp < min) {
			updates[i].array[current%size].type = -2;
			current++;
		}
	}
}

void Compare_Lamport(int lamport)
{
	/** Compares a received lamport timestamp to the server's - if the received one is higer, 
	 * it sets the server's equal to it**/
	if(lamport > lamport_counter) {
		lamport_counter = lamport;
	}
}

char* chatroom_to_local(char chatroom[])
{
	int chatroom_len = strlen(chatroom);
	int local_len = strlen(local);
	
	local = malloc(sizeof(char[80]));
	strncpy(local, chatroom, chatroom_len-1);
	char* server_index_char = malloc(sizeof(char));
	char tmp = (char)(((int)'0')+machine_index);
	*server_index_char = tmp;
	strcat(local, server_index_char);
	return local;
}

char* chatroom_sans_index(char chatroom[]) {
	int chatroom_len = strlen(chatroom);

	local = malloc(sizeof(char[80]));
	strncpy(local, chatroom, chatroom_len-1);
    return local;
}
