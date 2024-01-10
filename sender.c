#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>

#define IPC_KEY_PROJ_ID 1
#define SHMEM_SIZE 100

#define COM_FLAG_RECEIVER_READY 1
#define COM_FLAG_DATA_READY 2
#define COM_FLAG_DATA_READ 4
#define COM_FLAG_EOF 8

/** @brief Runs file sender module
  * @param filepath Path to the file to be sent
  * @param shm_ptr Pointer to a shared memory segment
  * @return 0 on success, -1 if file failed to open */
int run_file_sender(char filepath[], char *shm_ptr);

/* ========= Program entry point =========  */
int main(int argc, char *argv[])
{
    key_t ipc_key = 0;
    char *shm_ptr = NULL;
    int shm_id = 0;

    /* Argument check */
    if (argc != 2) {
        printf("Usage: %s <file>\n", (argc ? argv[0] : "./sender"));
        return 1;
    }

    /* Check access to the file */
    if (access(argv[1], R_OK) == -1) {
        fprintf(stderr, "Failed to access file: %s\n", argv[1]);
        return 2;
    }

    /* IPC key generation from the file */
    ipc_key = ftok(argv[1], IPC_KEY_PROJ_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "Failed to generate IPC key (errno: %d)\n", errno);
        return 3;
    }

    /* Create shared memory segment */
    shm_id = shmget(ipc_key, SHMEM_SIZE, 0444 | IPC_CREAT);
    if (shm_id == -1) {
        fprintf(stderr, "Failed to create shared memory segment (errno: %d)\n", errno);
        return 4;
    }

    /* Attach the shared memory segment */
    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (char *) -1) {
        fprintf(stderr, "Failed to attach shared memory segment (errno: %d)\n", errno);
        return 5;
    }

    /* Print IPC key and wait for a receiver */
    printf("IPC key: %d\n", ipc_key);
    printf("Waiting for a receiver...\n");
    while (shm_ptr[0] != COM_FLAG_RECEIVER_READY) {
        sleep(1);
    }

    /* ===========================
     * Run file sender module
     * =========================== */
    if (run_file_sender(argv[1], shm_ptr) == -1) {
        fprintf(stderr, "Failed to open file: %s (errno: %d)\n", argv[1], errno);
        return 2;
    }

    /* Detach shared memory segment */
    if (shmdt(shm_ptr) == -1) {
        fprintf(stderr, "Failed to detach shared memory segment (errno: %d)\n", errno);
        return 6;
    }

    /* Remove shared memory segment */
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "Failed to remove shared memory (errno: %d)\n", errno);
        return 7;
    }

    return EXIT_SUCCESS;
}

int run_file_sender(char filepath[], char *shm_ptr)
{
    char *com_flags = shm_ptr; /* com_flags[1] */
    char *shm_buf = shm_ptr + 1; /* shm_buf[SHMEM_SIZE-1] */
    size_t read_bytes = 0;
    FILE *fp = NULL;

    /* Open file given in the argument in read mode */
    fp = fopen(filepath, "r");
    if (fp == NULL)
        return -1;

    /* Zero initialize communication flags */
    *com_flags = 0;

    while (1) {
        /* Send file contents to the receiver */
        read_bytes = fread(shm_buf, 1, SHMEM_SIZE - 1, fp);
        shm_buf[read_bytes] = '\0';
        *com_flags |= COM_FLAG_DATA_READY;
        *com_flags &= ~COM_FLAG_DATA_READ;

        /* If file has been emptied, set EOF flag */
        if (read_bytes < SHMEM_SIZE - 1) {
            *com_flags |= COM_FLAG_EOF;
            break;
        }

        /* Wait for the receiver to read the data */
        while (!(*com_flags & COM_FLAG_DATA_READ)) {
            sleep(1);
        }
    }

    /* Wait for the receiver before detaching shared memory segment */
    while (*com_flags & COM_FLAG_RECEIVER_READY) {
        sleep(1);
    }

    /* Clean up */
    fclose(fp);
    return 0;
}
