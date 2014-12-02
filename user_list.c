/** The user structure - this allows us to create a linked list of users
 *  which are currently in a chatroom**/

typedef struct user_node {
	char*  user;
	struct user_node * next;
	int    connected_server;
} user_node;

/** Adds a user to a user linked list **/
void add_user(user_node *head, user_node *toAdd)
{
	user_node *tmp = head;
	while(tmp->next != NULL)
	{
		tmp = tmp->next;
	}
	tmp->next = toAdd;
}

/** Removes a user from the linked list **/
/** THIS IS WRONG **/
/*int remove_user(user_node *head, user_node *toRemove)
{
	user_node *tmp = head;
	while(tmp->next != NULL)
	{
		if(tmp->user == toRemove->user)
		{
			tmp->next = tmp->next->next;
			//delete memory??
			return 1;
		}
	}
	return 0;
}*/

/** Remove a user from the linked list based on the server it is
 *  currently connected to **/
int remove_user(user_node *head, int connected_server_index)
{
	user_node *tmp = head->next;

	if(head->next == NULL) {
		return 0;
	}

	while(tmp->next != NULL) {
		if(tmp->next->connected_server == connected_server_index) {
			tmp->next = tmp->next->next;
		}
	}

	return 1;
}
