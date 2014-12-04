typedef struct update_struct{

  int type;
  char message[80];
  char user[20];
  lamport_timestamp lamport;
  lamport_timestamp liked_message_lamp;
  int vector[5];
  char chatroom[80];

} update;
