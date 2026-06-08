#define _DEFAULT_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/*IMPLEMENT SORT AND PADDING*/
#define MAX_ENTRIES 1024
#define MAX_NAME_LEN 256

// int strcmp(const char *str1, const char *str2);
int comp_names(const void* a, const void *b){
    return strcmp((const char *) a, (const char *) b);
}

//helper function, check mode type
void mode_string(mode_t mode, char *str) {
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '-';

    //permission string; user, group, other 
    str[1] = (mode & S_IRUSR) ? 'r' : '-'; 
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';

}

void print_long(const char *dir, const char *name, int max_user_len, int max_grp_len, int max_size_len, int max_link_len) {
    char fullpath[4096];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) < 0) {     //memory address of st = &st
        perror(name);
        return;
    }
    char modes[11];
    mode_string(st.st_mode, modes);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    const char *user = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";

    char time_buf[64];

    struct tm *tm = localtime(&st.st_mtim.tv_sec);
    strftime(time_buf, sizeof(time_buf), "%b %e %H:%M", tm);

    printf("%s %*lu %-*s %-*s %*ld %s %s\n", 
        modes, 
        max_link_len, (unsigned long) st.st_nlink,
        max_user_len, user, 
        max_grp_len, group,
        max_size_len, (long) st.st_size,
        time_buf, 
        name
    );
}


int show_all = 0;
long long_format = 0;

int main(int argc, char *argv[]) { //argv = array of strings
    int opt;
    int max_user_len = 0;
    int max_grp_len = 0;
    int max_size_len = 0;
    int max_link_len = 0;
    // char fullpath[4096];

    while ((opt = getopt(argc, argv, "al")) != -1){
        switch (opt) {
            case 'a':
                show_all = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "usage: %s [-al] [path]\n", argv[0]);
                return 1;
        }
    }


    const char *path = (optind < argc) ? argv[optind] : ".";    //opendir() opens a directory stream corresponding to directory name
    
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return 1;
    }
    
    //an array of pointers to structs; holds file names and metadata
    char names[MAX_ENTRIES][MAX_NAME_LEN];
    int count = 0;


    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){
        if (!show_all && entry->d_name[0] == '.') continue;
        if (count >= MAX_ENTRIES) break;

        // char *strncpy(char *dest, const char *src, size_t n);
        strncpy(names[count], entry->d_name, MAX_NAME_LEN);
        names[count][MAX_NAME_LEN - 1] = '\0';
        count++;
    }
    closedir(dir);

    //sort names
    qsort(names, count, MAX_NAME_LEN, comp_names);

    //only if long format
    if (long_format) {
        char inspect_path[4096];

        for (int i = 0; i < count; i++) {
            //base path and names[i] into inspect_path
            snprintf(inspect_path, sizeof(inspect_path), "%s/%s", path, names[i]);
           
            //lstat on inspect_path
            struct stat st;
            if (lstat(inspect_path, &st) < 0) continue;

            //getpwuid(st.st_uid) and getgrgid(st.st_gid)
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_uid);
            const char *user = pw ? pw->pw_name : "?";
            const char *group = gr ? gr->gr_name : "?";

            //calculate string length of user and group name
            int u_len = strlen(user);
            if (u_len > max_user_len) {
                max_user_len = u_len;
            }
            
            int gr_len = strlen(group);
            if (gr_len > max_grp_len) {
                max_grp_len = gr_len;
            }

            //calculate digit length of st.st_size and st.st_nlink 
            int sz_len = snprintf(NULL, 0, "%ld", (long)st.st_size);
            if (sz_len > max_size_len) {
                max_size_len = sz_len;
            }

            int lnk_len = snprintf(NULL, 0, "%lu", (unsigned long)st.st_nlink);
            if (lnk_len > max_link_len) {
                max_link_len = lnk_len;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        if (long_format) {
            //calculated max trackers into print_long
            print_long(path, names[i], max_user_len, max_grp_len, max_size_len, max_link_len);
        }
        else {
            //standard print
            printf("%s\n", names[i]);
        }

    }

    return 0;
}