typedef struct update_struct{

  int type;
  char[80] message;
  struct lamport_timestamp update_lamport;
  struct lamport_timestamp liked_message_lamport;

} update;
