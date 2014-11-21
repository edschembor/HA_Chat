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

/** Removes a user to a user linked list **/
int remove_user(user_node *head, user_node *toRemove)
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
}
