/** The user structure - this allows us to create a linked list of users
 *  which are currently in a chatroom**/

typedef struct user_node {
	char  *user;
	struct user_node * next;
	int    connected_server;
} user_node;

/** Adds a user to a user linked list **/
void add_user(user_node *head, user_node *to_add)
{
	user_node *tmp = head;
	while(tmp->next != NULL)
	{
		tmp = tmp->next;
	}
	tmp->next = to_add;
	to_add->next = NULL;
	printf("\nAdded: %s\n", to_add->user);
}

/** Removes a user from the linked list **/
int remove_user(user_node *head, user_node *to_remove)
{
	user_node *tmp = head->next;

	if(head->user != NULL) {
		if(strcmp(head->user, to_remove->user) == 0) {
			head = head->next;
			return 0;
		}
	}

	if(head->next == NULL) {
		return 0;
	}

	while(tmp->next != NULL) {
		if(strcmp(tmp->next->user, to_remove->user) == 0) {
			tmp->next = tmp->next->next;
			break;
		}
		tmp = tmp->next;
	}

	return 0;
}

/** Remove a user from the linked list based on the server it is
 *  currently connected to **/
int remove_user_partition(user_node *head, int connected_server_index)
{
	user_node *tmp = head->next;

	if(head->next == NULL) {
		return 0;
	}

	while(tmp->next != NULL) {
		printf("\nHERE: %s\n", tmp->user);
		if(tmp->next->connected_server == connected_server_index) {
			tmp->next = tmp->next->next;
			break;
		}
		tmp = tmp->next;
	}

	return 1;
}
