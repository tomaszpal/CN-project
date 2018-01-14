#ifndef _AN_CONNECTION_HANDLING_
#define _AN_CONNECTION_HANDLING_

/* Defines break time between polls by slave threads.           */
#define POLL_TIME_PERIOD    10

/* Handles communication with client.                           */
void client_support(int id);

/* Handles communication with slave.                            */
void slave_support(int id);

/* Handles communication with client and slave.                 */
void* handle_connection(void* arg);

#endif //_AN_CONNECTION_HANDLING_
