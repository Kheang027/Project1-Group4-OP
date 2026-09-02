#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MIN_THREADS 2
#define MAX_THREADS 10
#define QUEUE_SIZE 20
#define CHUNK_SIZE 4096

/* A queue item keeps both the data and its position in the file. */
typedef struct {
    char data[CHUNK_SIZE];
    size_t length;
    off_t offset;
} Chunk;

typedef struct {
    Chunk queue[QUEUE_SIZE];
    int front;
    int rear;
    int count;

    int source_fd;
    int destination_fd;
    off_t file_size;
    off_t next_offset;
    int active_readers;
    int failed;

    pthread_mutex_t queue_mutex;
    pthread_mutex_t offset_mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Shared;

typedef struct {
    Shared *shared;
    int number;
} ThreadArgs;

/* Record a thread error and wake any threads waiting on the queue. */
static void report_failure(Shared *shared, const char *operation, int error)
{
    fprintf(stderr, "%s failed: %s\n", operation, strerror(error));

    if (pthread_mutex_lock(&shared->queue_mutex) == 0) {
        shared->failed = 1;
        pthread_cond_broadcast(&shared->not_empty);
        pthread_cond_broadcast(&shared->not_full);
        pthread_mutex_unlock(&shared->queue_mutex);
    }
}

/* pread() may return fewer bytes than requested, so continue if necessary. */
static ssize_t read_chunk(int fd, char *data, size_t length, off_t offset)
{
    size_t total = 0;

    while (total < length) {
        ssize_t result = pread(fd, data + total, length - total,
                               offset + (off_t)total);
        if (result > 0) {
            total += (size_t)result;
        } else if (result == 0) {
            break;
        } else if (errno != EINTR) {
            return -1;
        }
    }
    return (ssize_t)total;
}

/* pwrite() may also be partial, so write until the whole chunk is copied. */
static int write_chunk(int fd, const Chunk *chunk)
{
    size_t total = 0;

    while (total < chunk->length) {
        ssize_t result = pwrite(fd, chunk->data + total,
                                chunk->length - total,
                                chunk->offset + (off_t)total);
        if (result > 0) {
            total += (size_t)result;
        } else if (result < 0 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

static void reader_finished(Shared *shared)
{
    pthread_mutex_lock(&shared->queue_mutex);
    shared->active_readers--;
    if (shared->active_readers == 0)
        pthread_cond_broadcast(&shared->not_empty);
    pthread_mutex_unlock(&shared->queue_mutex);
}

static void *reader(void *argument)
{
    ThreadArgs *args = argument;
    Shared *shared = args->shared;

    for (;;) {
        off_t offset;
        size_t wanted;
        Chunk item;
        int rc = pthread_mutex_lock(&shared->offset_mutex);

        if (rc != 0) {
            report_failure(shared, "pthread_mutex_lock", rc);
            break;
        }

        offset = shared->next_offset;
        if (offset >= shared->file_size) {
            pthread_mutex_unlock(&shared->offset_mutex);
            break;
        }

        wanted = CHUNK_SIZE;
        if (shared->file_size - offset < (off_t)wanted)
            wanted = (size_t)(shared->file_size - offset);
        shared->next_offset += (off_t)wanted;
        pthread_mutex_unlock(&shared->offset_mutex);

        item.offset = offset;
        ssize_t bytes = read_chunk(shared->source_fd, item.data,
                                   wanted, offset);
        if (bytes < 0) {
            report_failure(shared, "pread", errno);
            break;
        }
        item.length = (size_t)bytes;

        rc = pthread_mutex_lock(&shared->queue_mutex);
        if (rc != 0) {
            fprintf(stderr, "Reader %d could not lock queue: %s\n",
                    args->number, strerror(rc));
            break;
        }

        while (shared->count == QUEUE_SIZE && !shared->failed) {
            rc = pthread_cond_wait(&shared->not_full,
                                   &shared->queue_mutex);
            if (rc != 0) {
                shared->failed = 1;
                break;
            }
        }

        if (shared->failed) {
            pthread_mutex_unlock(&shared->queue_mutex);
            break;
        }

        shared->queue[shared->rear] = item;
        shared->rear = (shared->rear + 1) % QUEUE_SIZE;
        shared->count++;
        pthread_cond_signal(&shared->not_empty);
        pthread_mutex_unlock(&shared->queue_mutex);
    }

    reader_finished(shared);
    return NULL;
}

static void *writer(void *argument)
{
    ThreadArgs *args = argument;
    Shared *shared = args->shared;

    for (;;) {
        Chunk item;
        int rc = pthread_mutex_lock(&shared->queue_mutex);

        if (rc != 0) {
            fprintf(stderr, "Writer %d could not lock queue: %s\n",
                    args->number, strerror(rc));
            break;
        }

        while (shared->count == 0 && shared->active_readers > 0 &&
               !shared->failed) {
            rc = pthread_cond_wait(&shared->not_empty,
                                   &shared->queue_mutex);
            if (rc != 0) {
                shared->failed = 1;
                break;
            }
        }

        if (shared->failed ||
            (shared->count == 0 && shared->active_readers == 0)) {
            pthread_mutex_unlock(&shared->queue_mutex);
            break;
        }

        item = shared->queue[shared->front];
        shared->front = (shared->front + 1) % QUEUE_SIZE;
        shared->count--;
        pthread_cond_signal(&shared->not_full);
        pthread_mutex_unlock(&shared->queue_mutex);

        if (write_chunk(shared->destination_fd, &item) != 0) {
            report_failure(shared, "pwrite", errno);
            break;
        }
    }
    return NULL;
}

static int parse_thread_count(const char *text, int *count)
{
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);

    if (errno != 0 || *text == '\0' || *end != '\0' ||
        value < MIN_THREADS || value > MAX_THREADS)
        return -1;

    *count = (int)value;
    return 0;
}

int main(int argc, char *argv[])
{
    int thread_count;
    int status = EXIT_FAILURE;
    struct stat source_info;
    Shared shared;
    pthread_t readers[MAX_THREADS];
    pthread_t writers[MAX_THREADS];
    ThreadArgs reader_args[MAX_THREADS];
    ThreadArgs writer_args[MAX_THREADS];
    int reader_count = 0;
    int writer_count = 0;

    if (argc != 4 || parse_thread_count(argv[1], &thread_count) != 0) {
        fprintf(stderr, "Usage: %s n source destination (2 <= n <= 10)\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    if (stat(argv[2], &source_info) != 0) {
        perror("stat source");
        return EXIT_FAILURE;
    }

    memset(&shared, 0, sizeof(shared));
    shared.file_size = source_info.st_size;
    shared.active_readers = thread_count;
    shared.source_fd = open(argv[2], O_RDONLY);
    if (shared.source_fd == -1) {
        perror("open source");
        return EXIT_FAILURE;
    }

    shared.destination_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (shared.destination_fd == -1) {
        perror("open destination");
        close(shared.source_fd);
        return EXIT_FAILURE;
    }

    if (ftruncate(shared.destination_fd, shared.file_size) != 0) {
        perror("ftruncate destination");
        goto close_files;
    }

    int qm = pthread_mutex_init(&shared.queue_mutex, NULL);
    int om = qm == 0 ? pthread_mutex_init(&shared.offset_mutex, NULL) : -1;
    int ne = om == 0 ? pthread_cond_init(&shared.not_empty, NULL) : -1;
    int nf = ne == 0 ? pthread_cond_init(&shared.not_full, NULL) : -1;
    if (qm != 0 || om != 0 || ne != 0 || nf != 0) {
        fprintf(stderr, "Could not initialise pthread resources\n");
        if (ne == 0) pthread_cond_destroy(&shared.not_empty);
        if (om == 0) pthread_mutex_destroy(&shared.offset_mutex);
        if (qm == 0) pthread_mutex_destroy(&shared.queue_mutex);
        goto close_files;
    }

    /* Start writers first; they sleep until readers add queue items. */
    for (int i = 0; i < thread_count; i++) {
        writer_args[i] = (ThreadArgs){&shared, i + 1};
        int rc = pthread_create(&writers[i], NULL, writer, &writer_args[i]);
        if (rc != 0) {
            fprintf(stderr, "Could not create writer %d: %s\n",
                    i + 1, strerror(rc));
            break;
        }
        writer_count++;
    }

    if (writer_count > 0) {
        for (int i = 0; i < thread_count; i++) {
            reader_args[i] = (ThreadArgs){&shared, i + 1};
            int rc = pthread_create(&readers[i], NULL, reader,
                                    &reader_args[i]);
            if (rc != 0) {
                fprintf(stderr, "Could not create reader %d: %s\n",
                        i + 1, strerror(rc));
                break;
            }
            reader_count++;
        }
    }

    pthread_mutex_lock(&shared.queue_mutex);
    shared.active_readers -= thread_count - reader_count;
    if (shared.active_readers == 0)
        pthread_cond_broadcast(&shared.not_empty);
    pthread_mutex_unlock(&shared.queue_mutex);

    for (int i = 0; i < reader_count; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < writer_count; i++)
        pthread_join(writers[i], NULL);

    if (!shared.failed && reader_count == thread_count &&
        writer_count == thread_count)
        status = EXIT_SUCCESS;

    pthread_cond_destroy(&shared.not_full);
    pthread_cond_destroy(&shared.not_empty);
    pthread_mutex_destroy(&shared.offset_mutex);
    pthread_mutex_destroy(&shared.queue_mutex);

close_files:
    if (close(shared.source_fd) != 0) {
        perror("close source");
        status = EXIT_FAILURE;
    }
    if (close(shared.destination_fd) != 0) {
        perror("close destination");
        status = EXIT_FAILURE;
    }
    return status;
}
