#ifndef _SLAVE_TOOLS_
#define _SLAVE_TOOLS_

/* Defines clean types for slave:
 * - soft - clean folder inside,
 * - hard - remove folder and its content.                        */
typedef enum CleanType {
    clean_soft,
    clean_hard
} CleanType;

/* Cleans the work directory.                                   */
int clean(CleanType type);

/* Executes a python script.                                    */
int do_work(RData_File* result, fileType type, const char* data, unsigned long size);

/* Handshake with a server to give info about being slave.      */
int handshake(int s_socket, const char key[KEY_LENGTH + 1]);

/* Checks if slave has python2 and python3 interpreter
 * and creates work directory.                                  */
int setup();

#endif //_SLAVE_TOOLS_
