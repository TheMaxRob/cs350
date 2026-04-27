// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR, CODE WRITTEN BY OTHER STUDENTS, OR CODE DERIVED FROM AN AI TOOL- Max Roberts

// 1. Extract alphabetic words
// 2. lowercase
// 3. discard words <= short length
// 4. truncate to long length
// 5. sort alphabetically
// 6. count duplicates

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int main (int argc, char *argv[])
{
    int opt;

    int count = 1;
    int s = 0;
    int l = 2048; // generic large value I decided on

    while ((opt = getopt (argc, argv, "n:s:l:")) != -1) {
        switch (opt) {
            case 'n':
                count = atoi(optarg);
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'l':
                l = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: pipesort [-nsl] \n");
                exit(1);
        }
    }

    // Passed checks, proceed with pipesort

    // read standard input
    int parse_sort[2];
    int sort_merge[2];

    pipe(parse_sort);
    pipe(sort_merge);

    pid_t sort_pid = fork();

    if (sort_pid == 0) {
        // if this is the sorter
        dup2(parse_sort[0], STDIN_FILENO);
        dup2(sort_merge[1], STDOUT_FILENO);
        close(parse_sort[0]);
        close(parse_sort[1]);
        close(sort_merge[0]);
        close(sort_merge[1]);

        execlp("/usr/bin/sort", "sort", NULL);
        exit(1);
    }

    // Fork merger
    pid_t merge_pid = fork();
    if (merge_pid == 0) {
        // if this is the merger
        // read from sort_merge[0] write to stdout
        dup2(sort_merge[0], STDIN_FILENO);
        close(sort_merge[0]);
        close(sort_merge[1]);
        close(parse_sort[0]);
        close(parse_sort[1]);

        char buf[l+1];
        char prev[l+1];
        prev[0] = '\0';
        int count = 0;
        while (fgets(buf, sizeof(buf), stdin) != NULL) {
            // if buffer equals prev, increase count and continue
            buf[strcspn(buf, "\n")] = '\0';
            if (buf[0] == '\0') continue;  // skip blanks if we somehow get one
            if (strcmp(buf, prev) == 0) {
                count++;
                continue;
            } else {
                // print count then word
                if (count > 0) {
                    printf("%-10d%s\n", count, prev);
                }
                strcpy(prev, buf);
                count = 1;
            }
        }

        printf("%-10d%s\n", count, prev);
        exit(0);
    }

    // if this is the parser
    // read input
    // parse into words
    // lower()

    close(parse_sort[0]);
    close(sort_merge[0]);
    close(sort_merge[1]);
    
    int c;
    char buf[l+1];
    int length = 0;
    FILE *to_sort = fdopen(parse_sort[1], "w");
    while ((c = fgetc(stdin)) != EOF) {
        if (isalpha(c)) {
            if (length < l) {
                // add to buffer
                buf[length] = tolower(c);
                length++;   
            }
        } else {
            // end of word
            if (length > s) {
                buf[length] = '\0';
                fprintf(to_sort, "%s\n", buf);
            }
            length = 0;
            memset(buf, 0, l+1);
        }
    }
    // Handle final word
    if (length > s) {
        buf[length] = '\0';
        fprintf(to_sort, "%s\n", buf);
    }
    fclose(to_sort);
    waitpid(sort_pid, NULL, 0);
    waitpid(merge_pid, NULL, 0);

    exit(0);
}
