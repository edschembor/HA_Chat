typedef struct update_struct{

  int type;
  char message[80];
  char *user;
  lamport_timestamp lamport;
  lamport_timestamp liked_message_lamp;
  int vector[5];

} update;
