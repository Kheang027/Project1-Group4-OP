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

int main(int argc, char **argv) {
    // Get args
    if (argc != 4) {
        printf("Error: Wrong number of args\n");
        return 1;
    }
    int n = atoi(argv[1]);
    if (n < 2 || n > 10) {
        printf("Error: n has to be greater than or equal to 2 or lesser than or equal to 10\n");
        return 1;
    }
    char *src_dir = argv[2];
    char *dst_dir = argv[3];
    
    // Case 1: src_dir == dst_dir
    if (strcmp(src_dir, dst_dir) == 0) {
        printf("Error: Source and destination directories are the same\n");
        return 1;
    }

    // Case 2: src_dir != dst_dir
    // Get src_dir
    DIR *src_dirp = opendir(src_dir);
    if (src_dirp == NULL) {
        printf("Error: %s does not exist\n", src_dir);
        return 1;
    }

    // Get or create dst_dir
    DIR *dst_dirp = opendir(dst_dir);
    if (dst_dirp == NULL) {
        if (mkdir(dst_dir, 0777) == -1) {
            printf("Error making destination directory\n");
            return 1;
        }
    } else {
        closedir(dst_dirp);
    }

    // Copy files from src_dir to dst_dir with a thread per file
    struct dirent *entry = NULL;
    int i = 0;
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * n);
    while ((entry = readdir(src_dirp)) != NULL && i < n) {
        // Skip hidden files
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Create thread for each file
        thread_args_t *thread_args = (thread_args_t *)malloc(sizeof(thread_args_t));
        thread_args->src_file = (char *)malloc(strlen(src_dir) + 1 + strlen(entry->d_name) + 1);
        strcpy(thread_args->src_file, src_dir);
        strcat(thread_args->src_file, "/");
        strcat(thread_args->src_file, entry->d_name);
        thread_args->dst_file = (char *)malloc(strlen(dst_dir) + 1 + strlen(entry->d_name) + 1);
        strcpy(thread_args->dst_file, dst_dir);
        strcat(thread_args->dst_file, "/");
        strcat(thread_args->dst_file, entry->d_name);

        pthread_create(&threads[i], NULL, copy_file, (void *)thread_args);
        i++;
    }
    closedir(src_dirp);

    // Wait for threads to finish
    for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
    }
    free(threads);

    return 0;
}

void *copy_file(void *thread_args) {
    thread_args_t *args = (thread_args_t *)thread_args;

    // Copy files
    FILE *src_file = fopen(args->src_file, "r");
    FILE *dst_file = fopen(args->dst_file, "w");
    char buffer[BUFFER_SIZE];
    int bytes_read;
    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dst_file);
    }
    fclose(src_file);
    fclose(dst_file);

    // Free args
    free(args->src_file);
    free(args->dst_file);
    free(args);

    // Close thread
    pthread_exit(NULL);
}