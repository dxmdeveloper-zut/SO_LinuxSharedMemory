#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef IPC_KEY_PATHNAME
#define IPC_KEY_PATHNAME "key.txt"
#endif
#define IPC_KEY_PROJ_ID 1


int main(int argc, char *argv[])
{
    key_t ipc_key;

    ipc_key = ftok(IPC_KEY_PATHNAME, IPC_KEY_PROJ_ID);
    if (ipc_key == -1){
        fprintf(stderr, "Failed to generate IPC key\n");
        return 1;
    }

    return EXIT_SUCCESS;
}
