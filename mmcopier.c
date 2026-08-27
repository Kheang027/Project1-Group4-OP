#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

typedef struct thread_args {
    char *src_file;
    char *dst_file;
} thread_args_t;

void *copy_file(void *thread_args);

/**
 * @brief Creates threads to copy files from a source directory to a destination directory.
 *
 * The program validates the command-line arguments, opens the source directory,
 * creates the destination directory if it does not exist, and creates up to
 * n (2 <= n <= 10) threads to copy files concurrently. Each thread is responsible
 * for copying one file. The function waits for all created threads to finish
 * before releasing allocated resources and terminating.
 *
 * If a memory allocation, directory operation, or pthread operation fails,
 * an error is printed to stderr and the program terminates with an error status.
 *
 * @return 0 if all files are copied successfully; 1 if an error occurs.
 */
int main(int argc, char **argv) {
    // Parse args
    if (argc != 4) {
        fprintf(stderr, "Error: Wrong number of args\n");
        return 1;
    }
    int n = atoi(argv[1]);
    if (n < 2 || n > 10) {
        fprintf(stderr, "Error: n has to be an integer between 2 and 10\n");
        return 1;
    }
    char *src_dir = argv[2];
    char *dst_dir = argv[3];
    
    // Copy files from src_dir to dst_dir with n threads for n files
    // Case 1: src_dir == dst_dir
    if (strcmp(src_dir, dst_dir) == 0) {
        fprintf(stderr, "Error: Source and destination directories are the same\n");
        return 1;
    }

    // Case 2: src_dir != dst_dir
    // Get src_dir ptr
    DIR *src_dirp = opendir(src_dir);
    if (src_dirp == NULL) {
        fprintf(stderr, "Error: Source directory does not exist\n");
        return 1;
    }

    // Check if dst_dir exists and create dir if not
    DIR *dst_dirp = opendir(dst_dir);
    if (dst_dirp == NULL) {
        if (mkdir(dst_dir, 0777) == -1) {
            fprintf(stderr, "Error making destination directory\n");
            return 1;
        }
    } else {
        closedir(dst_dirp);
    }

    // Copy files from src_dir to dst_dir with 1 thread per file
    struct dirent *entry = NULL;
    int i = 0;
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * n);
    if (threads == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    while ((entry = readdir(src_dirp)) != NULL && i < n) {
        // Skip entries that are hidden files, and . and ..
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Create thread for each file
        thread_args_t *thread_args = (thread_args_t *)malloc(sizeof(thread_args_t));
        if (thread_args == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }
        // Setup args for each thread
        // Allocate enough memory for: src_dir + '/' + filename + '\0'
        thread_args->src_file = (char *)malloc(strlen(src_dir) + 1 + strlen(entry->d_name) + 1);
        if (thread_args->src_file == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }
        strcpy(thread_args->src_file, src_dir);
        strcat(thread_args->src_file, "/");
        strcat(thread_args->src_file, entry->d_name);
        thread_args->dst_file = (char *)malloc(strlen(dst_dir) + 1 + strlen(entry->d_name) + 1);
        if (thread_args->dst_file == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }
        strcpy(thread_args->dst_file, dst_dir);
        strcat(thread_args->dst_file, "/");
        strcat(thread_args->dst_file, entry->d_name);

        // Create thread
        if (pthread_create(&threads[i], NULL, copy_file, (void *)thread_args) != 0) {
            fprintf(stderr, "Error: Failed to create thread\n");
            return 1;
        }
        i++;
    }
    closedir(src_dirp);

    // Wait for threads to finish
    for (int j = 0; j < i; j++) {
        int result;
        if ((result = pthread_join(threads[j], NULL)) != 0) {
            fprintf(stderr, "Error: pthread_join failed to execute\n");
        }
    }
    free(threads);

    return 0;
}

/**
 * @brief Copies a source file to a destination file.
 *
 * The source file is read in BUFFER_SIZE-sized chunks and each chunk
 * is written to the destination file. Once copying is complete, both
 * files and the dynamically allocated thread arguments are released.
 *
 * If a pthread operation or file operation fails, an error is printed
 * to stderr and the entire program terminates with EXIT_FAILURE.
 *
 * @param thread_args Pointer to a thread_args_t containing the source
 *                    and destination file paths.
 *
 * @return NULL when the thread terminates successfully.
 */
void *copy_file(void *thread_args) {
    thread_args_t *args = (thread_args_t *)thread_args;

    // Copy src_file to dst_file
    FILE *src_file = fopen(args->src_file, "r");
    if (src_file == NULL) {
        fprintf(stderr, "Error: Failed to open file: %s\n", args->src_file);
        exit(EXIT_FAILURE);
    }

    FILE *dst_file = fopen(args->dst_file, "w");
    if (dst_file == NULL) {
        fprintf(stderr, "Error: Failed to open file: %s\n", args->dst_file);
        exit(EXIT_FAILURE);
    }
    
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read) {
            fprintf(stderr, "Error: Failed to write to file: %s\n", args->dst_file);
            exit(EXIT_FAILURE);
        }
    }
    if (ferror(src_file)) {
        fprintf(stderr, "Error: Failed to read file: %s\n", args->src_file);
        exit(EXIT_FAILURE);
    }

    // Close files
    fclose(src_file);
    fclose(dst_file);

    // Free args
    free(args->src_file);
    free(args->dst_file);
    free(args);

    // Close thread
    pthread_exit(NULL);
}