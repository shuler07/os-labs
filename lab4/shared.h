#ifndef SHARED_H
#define SHARED_H

#include <stddef.h>
#include <semaphore.h>
#include <sys/types.h>


enum message_type {
    MESSAGE_PART,
    MESSAGE_LINE,
    MESSAGE_FINISH
};

enum reply_type {
    REPLY_OK,
    REPLY_INVALID,
    REPLY_ERROR
};

struct shared_data {
    sem_t request;
    sem_t response;
    pid_t parent_pid;
    enum message_type type;
    enum reply_type reply;
    size_t length;
    char buffer[4096];
    char message[256];
};

int create_shared(struct shared_data **data);
int close_shared(struct shared_data *data, int owner);
int wait_message(sem_t *semaphore, pid_t peer, int child_process);
void set_error(struct shared_data *data, const char *operation, int error);

#endif
