// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR, CODE WRITTEN BY OTHER STUDENTS, OR CODE DERIVED FROM AN AI TOOL- Max Roberts


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <dirent.h>
#include <time.h>
#include <ar.h>

struct meta {
    char name[16];
    int mode;
    int size;
    time_t mtime;
};

void write_ar_mag(int fd) {
    if (write(fd, ARMAG, SARMAG) != SARMAG) {
        perror("write ARMAG");
        exit(1);
    }
}

int fill_ar_hdr(char *filename, struct ar_hdr *hdr) {
    struct stat st;
    char buf[18];

    
    if (stat(filename, &st) < 0) { perror(filename); return -1; }

    // Define our mem space
    memset(hdr, ' ', sizeof(struct ar_hdr));

    // ar_name
    snprintf(buf, sizeof(buf), "%s/", filename);
    memcpy(hdr->ar_name, buf, strlen(buf));

    //ar_date
    snprintf(buf, sizeof(buf), "%ld", st.st_mtime);
    memcpy(hdr->ar_date, buf, strlen(buf));
    
    //ar_uid
    snprintf(buf, sizeof(buf), "%d", (int)st.st_uid);
    memcpy(hdr->ar_uid, buf, strlen(buf));

    //ar_gid
    snprintf(buf, sizeof(buf), "%d", (int)st.st_gid);
    memcpy(hdr->ar_gid, buf, strlen(buf));

    //ar_mode
    snprintf(buf, sizeof(buf), "%o", (int)st.st_mode);
    memcpy(hdr->ar_mode, buf, strlen(buf));

    // size
    snprintf(buf, sizeof(buf), "%ld", (long)st.st_size);
    memcpy(hdr->ar_size, buf, strlen(buf));

    // arfmag
    memcpy(hdr->ar_fmag, ARFMAG, 2);

    return 0;
}

// Stole this write pattern from the internet
void writeFileData(int dstfd, int srcfd, int size) {
    char *buf;
    int remaining = size;
    int n;

    buf = malloc(size > 0 ? size : 1);
    if (!buf) { perror("malloc"); exit(1); }

    while (remaining > 0) {
        n = read(srcfd, buf, remaining);
        if (n <= 0) { perror("read"); free(buf); exit(1); }
        if (write(dstfd, buf, n) != n) { perror("write"); free(buf); exit(1); }
        remaining -= n;
    }

    if (size % 2 != 0) {
        char pad = '\n';
        write(dstfd, &pad, 1);
    }

    free(buf);
}

int fill_meta(struct ar_hdr hdr, struct meta *meta) {
    char buf[18];
    // ar_name to name
    memcpy(meta->name, hdr.ar_name, sizeof(hdr.ar_name));

    // strip trailing spaces
    for (int i = sizeof(hdr.ar_name) - 1; i >= 0; i--) {
        if (meta->name[i] == ' ') {
            meta->name[i] = '\0';
        } else {
            break;
        }
    }

    // Trailing slash
    int len = strlen(meta->name);
    if (len > 0 && meta->name[len - 1] == '/') {
        meta->name[len - 1] = '\0';
    }


    // ar_mode to mode
    memset(buf, 0, sizeof(buf));
    memcpy(buf, hdr.ar_mode, sizeof(hdr.ar_mode));
    meta->mode = (int)strtol(buf, NULL, 8);

    // ar_size to size
    memset(buf, 0, sizeof(buf));
    memcpy(buf, hdr.ar_size, sizeof(hdr.ar_size));
    meta->size = atoi(buf); // hm

    // date to mtime
    memset(buf, 0, sizeof(buf));
    memcpy(buf, hdr.ar_date, sizeof(hdr.ar_date));
    meta->mtime = (time_t)atol(buf);

    return 0;
}

int checkArchiveMagic(int fd) {
    char magic[SARMAG];
    if (read(fd, magic, SARMAG) != SARMAG) {
        fprintf(stderr, "myar: not a valid archive, too short\n");
        return -1;
    }
    if (memcmp(magic, ARMAG, SARMAG) != 0) {
        fprintf(stderr, "myar: not a valid archive (wrong magic)\n");
        return -1;
    }
    return 0;
}



int quickAppend(char *archive, char **files, int nfiles) {
    // open archive or create it
    int archivefd = open(archive, O_WRONLY | O_CREAT, 0666);

    if (archivefd < 0) { perror(archive); exit(1); }

    struct stat ast;
    fstat(archivefd, &ast);
    // Handle archive creation
    if (ast.st_size == 0) {
        write_ar_mag(archivefd);
    } else {
        lseek(archivefd, 0, SEEK_END);
    }

    // append all files
    for (int i = 0; i < nfiles; i++) {
        int filefd = open(files[i], O_RDONLY);
        if (filefd < 0) { 
            perror(files[i]); 
            continue; 
        }
        
        struct ar_hdr hdr;
        if (fill_ar_hdr(files[i], &hdr) < 0) { 
            close(filefd); 
            continue; 
        }

        // write header to archive
        if (write(archivefd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
            close(filefd);
            exit(1);
        }

        // stat the file to get size
        struct stat file_stat;
        stat(files[i], &file_stat);

        writeFileData(archivefd, filefd, (int)file_stat.st_size);
        close(filefd);
    }

    close(archivefd);
    return 0;
}


int quickAppendOld(char *archive, int days) {

    // Identify files to append
    time_t now = time(NULL);
    time_t cutoff = (time_t)days * 86400;

    // Open current directory
    DIR *dir = opendir(".");

    // Read all files
    struct dirent *dp;
    while ((dp = readdir (dir)) != NULL) {
        if (strcmp(dp->d_name, archive) == 0) continue;
        // stat file
        struct stat fst;
        if (stat(dp->d_name, &fst) < 0) { perror(dp->d_name); continue; }
        if ((now - fst.st_mtime) < cutoff) continue;
        if (!S_ISREG(fst.st_mode)) continue; 
        
        char *file = dp->d_name;
        quickAppend(archive, &file, 1); 
    }

    closedir(dir);
    return 0;
}


int extractFiles(char *archive, char **files, int nfiles, int restore) {
    int archivefd = open(archive, O_RDONLY);
    if (archivefd < 0) { perror(archive); exit(1); }

    // Verify this is an archive
    if (checkArchiveMagic(archivefd) < 0) {
        close(archivefd);
        exit(1);
    }

    struct ar_hdr hdr;
    struct meta m;

    // Loop
    while (read(archivefd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
        // Decide whether to extract based on header
        fill_meta(hdr, &m);
        int extract = 0;

        // compare names
        for (int i = 0; i < nfiles && !extract; i++) {
            if (strcmp(files[i], m.name) == 0) {
                extract = 1;
                break;
            }
        }
        
        // if correct file found:
        if (extract) {
            int outfd = open(m.name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outfd < 0) { perror(m.name); exit(1); }
            writeFileData(outfd, archivefd, m.size);
            close(outfd);
            if (restore) {
                // restore original mtime & permissions

                chmod(m.name, m.mode);

                // modify time with utimbuf
                struct utimbuf utim;
                utim.modtime = m.mtime;
                utim.actime = m.mtime;
                utime(m.name, &utim);
            }
        } else {
            // Skip file contents
            lseek(archivefd, m.size + (m.size % 2), SEEK_CUR);
        }
    }
    close(archivefd);
    return 0;
}


int getList(char *archive, int verbose) {
    // List file names
    int archivefd = open(archive, O_RDONLY);
    if (archivefd < 0) { perror(archive); exit(1); }

    if (checkArchiveMagic(archivefd) < 0) { close(archivefd); exit(1); }

    struct ar_hdr hdr;
    while (read(archivefd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
        
        if (verbose) {
            // Man Page says this includes:
            // file permissions, decimal user, group ID's
            // file size in bytes, file modification time,
            // file name.

            // File Permissions
            char mbuf[9] = {0};
            memcpy(mbuf, hdr.ar_mode, sizeof(hdr.ar_mode));
            int mode = (int)strtol(mbuf, NULL, 8);

            printf("%c", (mode & 0400) ? 'r' : '-');
            printf("%c", (mode & 0200) ? 'w' : '-');
            printf("%c", (mode & 0100) ? 'x' : '-');
            printf("%c", (mode & 0040) ? 'r' : '-');
            printf("%c", (mode & 0020) ? 'w' : '-');
            printf("%c", (mode & 0010) ? 'x' : '-');
            printf("%c", (mode & 0004) ? 'r' : '-');
            printf("%c", (mode & 0002) ? 'w' : '-');
            printf("%c ", (mode & 0001) ? 'x' : '-');

            // Decimal user
            printf("%.*s ", (int)sizeof(hdr.ar_uid), hdr.ar_uid);

            // Group IDs
            printf("%.*s ", (int)sizeof(hdr.ar_gid), hdr.ar_gid);

            // file size in bytes 
            printf("%.*s ", (int)sizeof(hdr.ar_size), hdr.ar_size);

            // file modification time
            printf("%.*s ", (int)sizeof(hdr.ar_date), hdr.ar_date);

            printf("%.*s\n", (int)sizeof(hdr.ar_name), hdr.ar_name);
        } else {
            struct meta m;
            fill_meta(hdr, &m);
            printf("%s\n", m.name);
        }
        int skip = atoi(hdr.ar_size);
        skip += skip % 2;
        lseek(archivefd, skip, SEEK_CUR);
    }

    close(archivefd);
    return 0;
}

// Rebuild new archive excluding the specified files
int delete(char *archive, char **files, int nfiles) {
    int archivefd = open(archive, O_RDONLY);
    if (archivefd < 0) { perror(archive); exit(1); }

    if (checkArchiveMagic(archivefd) < 0) { close(archivefd); exit(1); }

    if (unlink(archive) < 0) { perror("unlink"); exit(1); }

    int newfd = open(archive, O_WRONLY | O_CREAT, 0666);
    if (newfd < 0) { perror(archive); exit(1); }

    write_ar_mag(newfd);

    struct ar_hdr hdr;
    struct meta m;
    int deletions[nfiles];
    memset(deletions,0,sizeof(deletions));

    while (read(archivefd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
        fill_meta(hdr, &m);


        // Mark files for deletion
        int shouldDelete = 0;
        for (int i = 0; i < nfiles; i++) {
            // Ensure we only delete first match
            if (!deletions[i] && strcmp(files[i], m.name) == 0) {
                shouldDelete = 1;
                deletions[i] = 1;
                break;
            }
        }

        

        if (!shouldDelete) {
            write(newfd, &hdr, sizeof(hdr));
            int padSize = m.size + (m.size % 2);
            char *buf = malloc(padSize > 0 ? padSize : 1); 
            int n = read(archivefd, buf, padSize);
            write(newfd, buf, n);
            free(buf);
        } else {
            // skip
            lseek(archivefd, m.size + (m.size % 2), SEEK_CUR);
        }
    }
    close(archivefd);
    close(newfd);
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: myar [qxotvdA:] archive-file [file1 .....]");
        return 1;
    }

    char *options = argv[1];
    char *archive = argv[2];
    // 4th element should always be file names array
    char **files = argv + 3;

    // since first 3 args are only non-file inputs, should just be argc-3
    int nfiles = argc - 3;

    // Parse Arguments
    // Creating bools because order could affect behavior sometimes
    int q = 0;
    int x = 0;
    int o = 0;
    int t = 0;
    int v = 0;
    int d = 0;
    int A = 0;

    int days = 0;


    for (int i = 0; options[i] != '\0'; i++) {
        switch (options[i]) {
            case 'q': q = 1; break;
            case 'x': x = 1; break;
            case 'o': o = 1; break;
            case 't': t = 1; break;
            case 'd': d = 1; break;
            case 'v': v = 1; break;
            case 'A':
                A = 1;
                if (argc < 3) {
                    fprintf(stderr, "myar: -A requires N and archive\n");
                    exit(1);
                }
                days = atoi(&options[i+1]);
                i = strlen(options);
                break;
            default:
                fprintf(stderr, "myar: unknown option '%c'\n", options[i]);
                exit(1);
        }
    }

    if (A) {
        quickAppendOld(archive, days);
    } else if (q) {
        quickAppend(archive, files, nfiles);
    } else if (x) {
        extractFiles(archive, files, nfiles, o);
    } else if (t) {
        getList(archive, v);
    } else if (d) {
        delete(archive, files, nfiles);
    } else {
        fprintf(stderr, "myar: no operation specified\n");
        exit(1);
    }

    return 0;
}
