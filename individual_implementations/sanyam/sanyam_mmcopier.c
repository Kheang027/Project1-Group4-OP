#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MIN_FILES 2
#define MAX_FILES 10
#define BUFFER_SIZE 4096

typedef struct {
    const char *source_dir;
    const char *destination_dir;
    int file_number;
} ThreadArgs;

static void *copy_file(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char source_path[1024];
    char destination_path[1024];
    int source_fd = -1;
    int destination_fd = -1;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    int written = snprintf(source_path, sizeof(source_path), "%s/source%d.txt",
                           args->source_dir, args->file_number);
    if (written < 0 || (size_t)written >= sizeof(source_path)) {
        fprintf(stderr, "Source path is too long for source%d.txt\n",
                args->file_number);
        return NULL;
    }

    written = snprintf(destination_path, sizeof(destination_path), "%s/source%d.txt",
                       args->destination_dir, args->file_number);
    if (written < 0 || (size_t)written >= sizeof(destination_path)) {
        fprintf(stderr, "Destination path is too long for source%d.txt\n",
                args->file_number);
        return NULL;
    }

    source_fd = open(source_path, O_RDONLY);
    if (source_fd == -1) {
        fprintf(stderr, "Could not open %s: %s\n", source_path, strerror(errno));
        return NULL;
    }

    destination_fd = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_fd == -1) {
        fprintf(stderr, "Could not open %s: %s\n",
                destination_path, strerror(errno));
        close(source_fd);
        return NULL;
    }

    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;

        while (total_written < bytes_read) {
            ssize_t bytes_written = write(destination_fd,
                                          buffer + total_written,
                                          (size_t)(bytes_read - total_written));
            if (bytes_written == -1) {
                fprintf(stderr, "Could not write %s: %s\n",
                        destination_path, strerror(errno));
                close(source_fd);
                close(destination_fd);
                return NULL;
            }
            total_written += bytes_written;
        }
    }

    if (bytes_read == -1) {
        fprintf(stderr, "Could not read %s: %s\n", source_path, strerror(errno));
    }

    if (close(source_fd) == -1) {
        fprintf(stderr, "Could not close %s: %s\n", source_path, strerror(errno));
    }
    if (close(destination_fd) == -1) {
        fprintf(stderr, "Could not close %s: %s\n",
                destination_path, strerror(errno));
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int n;
    pthread_t threads[MAX_FILES];
    ThreadArgs args[MAX_FILES];
    int created_threads = 0;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s n source_dir destination_dir\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    long parsed_n = strtol(argv[1], &endptr, 10);
    if (*argv[1] == '\0' || *endptr != '\0' ||
        parsed_n < MIN_FILES || parsed_n > MAX_FILES) {
        fprintf(stderr, "n must be an integer between %d and %d\n",
                MIN_FILES, MAX_FILES);
        return EXIT_FAILURE;
    }
    n = (int)parsed_n;

    if (access(argv[2], R_OK | X_OK) == -1) {
        fprintf(stderr, "Cannot access source directory %s: %s\n",
                argv[2], strerror(errno));
        return EXIT_FAILURE;
    }

    if (access(argv[3], W_OK | X_OK) == -1) {
        fprintf(stderr, "Cannot access destination directory %s: %s\n",
                argv[3], strerror(errno));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; ++i) {
        args[i].source_dir = argv[2];
        args[i].destination_dir = argv[3];
        args[i].file_number = i + 1;

        int result = pthread_create(&threads[i], NULL, copy_file, &args[i]);
        if (result != 0) {
            fprintf(stderr, "Could not create thread %d: %s\n",
                    i + 1, strerror(result));
            break;
        }
        ++created_threads;
    }

    for (int i = 0; i < created_threads; ++i) {
        int result = pthread_join(threads[i], NULL);
        if (result != 0) {
            fprintf(stderr, "Could not join thread %d: %s\n",
                    i + 1, strerror(result));
            return EXIT_FAILURE;
        }
    }

    if (created_threads != n) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}