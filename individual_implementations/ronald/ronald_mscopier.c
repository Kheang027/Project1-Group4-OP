#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define QUEUE_SIZE 20
#define BUFFER_SIZE 4096

typedef struct buffer_t {
    char buffer[QUEUE_SIZE][BUFFER_SIZE];
    size_t bytes_read[QUEUE_SIZE];
    int in;
    int out;
    int full;
} buffer_t;

typedef struct read_from_buffer_args_t {
    FILE *dst_fp;
    buffer_t *buffer;
    bool *file_read;
    pthread_mutex_t *mutex;
    pthread_cond_t *not_full;
    pthread_cond_t *not_empty;
} read_from_buffer_args_t;

typedef struct write_to_buffer_args_t {
    FILE *src_fp;
    buffer_t *buffer;
    bool *file_read;
    pthread_mutex_t *mutex;
    pthread_cond_t *not_full;
    pthread_cond_t *not_empty;
} write_to_buffer_args_t;

void *read_from_buffer(void *thread_args);
void *write_to_buffer(void *thread_args);
bool file_read_and_buffer_empty(pthread_mutex_t *mutex, bool *file_read, int *full);

int main(int argc, char **argv) {
    // Parse args
    if (argc != 4) {
        printf("Error: Wrong number of args\n");
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 2 || n > 10) {
        printf("Error: n has to be an integer between 2 and 10\n");
        return 1;
    }
    char *src_file = argv[2];
    char *dst_file = argv[3];

    // Create n threads and src_file to dst_file
    // Create shared resources
    buffer_t buffer = {0};
    FILE *src_fp = fopen(src_file, "r");
    if (src_fp == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", src_file);
        return 1;
    }
    FILE *dst_fp = fopen(dst_file, "w");
    if (dst_fp == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", dst_file);
        return 1;
    }
    bool file_read = false;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_t not_full;
    pthread_cond_init(&not_full, NULL);
    pthread_cond_t not_empty;
    pthread_cond_init(&not_empty, NULL);
    read_from_buffer_args_t read_args = {
        .dst_fp = dst_fp, 
        .buffer = &buffer,
        .file_read = &file_read,
        .mutex = &mutex,
        .not_full = &not_full,
        .not_empty = &not_empty
    };
    write_to_buffer_args_t write_args = {
        .src_fp = src_fp, 
        .buffer = &buffer,
        .file_read = &file_read,
        .mutex = &mutex,
        .not_full = &not_full,
        .not_empty = &not_empty
    };

    // Create n read and write threads
    pthread_t read_threads[n];
    pthread_t write_threads[n];
    for (int i = 0; i < n; i++) {
        if (pthread_create(&read_threads[i], NULL, read_from_buffer, (void *)&read_args) != 0) {
            fprintf(stderr, "Error: Failed to create thread\n");
            return 1;
        }
        if (pthread_create(&write_threads[i], NULL, write_to_buffer, (void *)&write_args) != 0) {
            fprintf(stderr, "Error: Failed to create thread\n");
            return 1;
        }
    }
    // Wait for threads to execute
    for (int i = 0; i < n; i++) {
        if (pthread_join(read_threads[i], NULL) != 0) {
            fprintf(stderr, "Error: Failed to join thread\n");
            return 1;
        }
        if (pthread_join(write_threads[i], NULL) != 0) {
            fprintf(stderr, "Error: Failed to join thread\n");
            return 1;
        }
    }

    // Free resources
    fclose(src_fp);
    fclose(dst_fp);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}

void *read_from_buffer(void *thread_args) {
    read_from_buffer_args_t *args = (read_from_buffer_args_t *)thread_args;

    while (true) {
        // Acquire mutex and check if buffer is not empty
        if (pthread_mutex_lock(args->mutex) != 0) {
            fprintf(stderr, "Error: Failed to lock mutex\n");
            exit(EXIT_FAILURE);
        }
        if (args->buffer->full == 0 && !*args->file_read) {
            if (pthread_cond_wait(args->not_empty, args->mutex) != 0) {
                fprintf(stderr, "Error: Failed to wait on condition variable\n");
                exit(EXIT_FAILURE);
            }
        }
        // Check if file has been read and buffer is empty
        if (args->buffer->full == 0 && *args->file_read) {
            if (pthread_mutex_unlock(args->mutex) != 0) {
                fprintf(stderr, "Error: Failed to unlock mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        // Read from buffer, write to file, and update state
        if (fwrite(args->buffer->buffer[args->buffer->out], 1, args->buffer->bytes_read[args->buffer->out], args->dst_fp) != args->buffer->bytes_read[args->buffer->out]) {
            fprintf(stderr, "Error: Failed to write to destination file\n");
            exit(EXIT_FAILURE);
        }
        args->buffer->out = (args->buffer->out + 1) % QUEUE_SIZE;
        args->buffer->full--;
        // Release mutex and signal cv
        if (pthread_cond_signal(args->not_full) != 0) {
            fprintf(stderr, "Error: Failed to signal condition variable\n");
            exit(EXIT_FAILURE);
        }
        if (pthread_mutex_unlock(args->mutex) != 0) {
            fprintf(stderr, "Error: Failed to unlock mutex\n");
            exit(EXIT_FAILURE);
        }
    }

    pthread_exit(NULL);
}

void *write_to_buffer(void *thread_args) {
    write_to_buffer_args_t *args = (write_to_buffer_args_t *)thread_args;

    while (true) {
        // Acquire mutex and check cv
        if (pthread_mutex_lock(args->mutex) != 0) {
            fprintf(stderr, "Error: Failed to lock mutex\n");
            exit(EXIT_FAILURE);
        }
        if (args->buffer->full == QUEUE_SIZE) {
            if (pthread_cond_wait(args->not_full, args->mutex) != 0) {
                fprintf(stderr, "Error: Failed to wait on condition variable\n");
                exit(EXIT_FAILURE);
            }
        }
        // Check if file is read
        if (*args->file_read) {
            if (pthread_mutex_unlock(args->mutex) != 0) {
                fprintf(stderr, "Error: Failed to unlock mutex\n");
                exit(EXIT_FAILURE);
            }
            pthread_exit(NULL);
        }
        // Write to buffer and update state
        size_t bytes_read = fread(args->buffer->buffer[args->buffer->in], 1, BUFFER_SIZE, args->src_fp);
        if (bytes_read) {
            args->buffer->bytes_read[args->buffer->in] = bytes_read;
            args->buffer->in = (args->buffer->in + 1) % QUEUE_SIZE;
            args->buffer->full++;
        } else if (feof(args->src_fp)) {
            break;
        } else {
            fprintf(stderr, "Error: Failed to read source file\n");
            exit(EXIT_FAILURE);
        }
        // Release mutex and signal cv
        if (pthread_cond_signal(args->not_empty) != 0) {
            fprintf(stderr, "Error: Failed to signal condition variable\n");
            exit(EXIT_FAILURE);
        }
        if (pthread_mutex_unlock(args->mutex) != 0) {
            fprintf(stderr, "Error: Failed to unlock mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    *args->file_read = true;
    // Wake up all sleeping producers and consumers to exit
    if (pthread_cond_broadcast(args->not_empty) != 0) {
        fprintf(stderr, "Error: Failed to broadcast condition variable\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_broadcast(args->not_full) != 0) {
        fprintf(stderr, "Error: Failed to broadcast condition variable\n");
        exit(EXIT_FAILURE);
    } 
    if (pthread_mutex_unlock(args->mutex) != 0) {
        fprintf(stderr, "Error: Failed to unlock mutex\n");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
}