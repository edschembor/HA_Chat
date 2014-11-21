typedef struct update_struct{

  int type;
  char message[80];
  lamport_timestamp update_lamport;
  lamport_timestamp liked_message_lamport;

} update;
