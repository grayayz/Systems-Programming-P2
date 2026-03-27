#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>

#define SUFFIX ".txt"
#define BUFFER_SIZE 1024


typedef struct wfd_node{
//need a linked list of mappings between words and counts (later frequencies)/iterate in lexigocraphic order
    char *word;
    int count;
    double frequency;
    struct wfd_node *next;
} wfd_node;


typedef struct{
//word storage (dynamically allocated):file's path, total word count, and its WFD
    int total_word_count;
    char *file_path;
    wfd_node *node;
} file_wfd;


void free_wfd(wfd_node *head){
    wfd_node *current = head;
    while (current != NULL){
        wfd_node *next = current->next;
        free(current->word);
        free(current);
        current = next;
    }
}


void free_file_wfd(file_wfd *f){
    if (!f) return;
    free (f->file_path);
    free_wfd(f->node);
    free(f);
}


wfd_node *add_word(wfd_node *head, char *word){
    //insert or increment a word in the WFD list.
    wfd_node *prev = NULL;
    wfd_node *current = head;
    
    while (current != NULL){
        int cmp = strcmp(word, current->word);
        if (cmp == 0){ //same word, word already exists in file
            current->count++; //just increment count
            return head;
        }
        //needs to be prev<word<current->word
        if (cmp < 0){ //correct
            break;
        }
        prev = current;
        current = current->next;
    }
    //done handling existing word logic, need to add new word
    wfd_node *new_node = malloc(sizeof(wfd_node));
    new_node->word = strdup(word); 
    new_node->count = 1;
    new_node->frequency = 0.0;
    new_node->next = current;
    if (prev == NULL){ //if there's nothing b4 current->word
        head = new_node; //new_node becomes the new head
    } else{
        prev->next = new_node;
    }
    return head;
}


void frequency(wfd_node *head, int total_words){
    //frequency = ((# of times the word appears)/(total # of words))
    wfd_node *current = head;

    while (current != NULL){
        current->frequency = ((current->count)/((double)total_words));
        current = current->next;
    }
}


file_wfd *process_file(const char *path){
    //read a file using open/read/close, tokenize into words,
    int fd = open(path, O_RDONLY);
    if (fd == -1){
        fprintf(stderr, "Error opening file %s\n", path);
        perror("open");
        return NULL;
    }
    file_wfd *new_file = malloc(sizeof(file_wfd));
    new_file->file_path = strdup(path);
    new_file->node = NULL;           
    new_file->total_word_count = 0;
    char buf[BUFFER_SIZE];   // buffer to read into
    size_t bytes_read;
    int word_length = 0;
    char *word_buf = malloc(sizeof(char) * 1024);
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0){
        for (int i = 0; i < (int)bytes_read; i++){
            char c = buf[i];
            if ((isalpha(c)) || (isdigit(c)) || (c == '-')){ //if its a word character
                word_buf[word_length++] = c; //accumulate
            } else{
                if (word_length >0){
                    for (int j = 0; j < word_length; j++){
                        word_buf[j] = tolower(word_buf[j]);
                    }
                    word_buf[word_length] = '\0';
                    new_file->node = add_word(new_file->node, word_buf);
                    new_file->total_word_count++;
                    word_length = 0;
                }
            }
        }
    }
    if (word_length > 0) {
        for (int j = 0; j < word_length; j++)
            word_buf[j] = tolower(word_buf[j]);
        word_buf[word_length] = '\0';
        new_file->node = add_word(new_file->node, word_buf);
        new_file->total_word_count++;
    }
    frequency(new_file->node, new_file->total_word_count);
    close(fd);
    free(word_buf);
    return new_file;
} 


int has_suffix(const char *filename){
    size_t len = strlen(filename);
    size_t suffix_len = strlen(SUFFIX);
    if (len < suffix_len){
        return 0;
    }
    int cmp = strcmp((filename + (len-suffix_len)), SUFFIX);
    if (cmp == 0){
        return 1;
    }

    return 0;
}


void collect_files(const char *path, int is_explicit, file_wfd ***files, int *count, int *capacity){
    struct stat buf;
    int sf = stat(path, &buf);
    if (sf == -1) {
        perror("stat");
        return;
    }
    if (S_ISDIR(buf.st_mode)){
        //is directory
        DIR *dir = opendir(path);
        char fullpath[BUFFER_SIZE];
        if (dir == NULL){
            perror("opendir");
            return;
        }
        struct dirent *de;
        while ((de = readdir(dir))){
            if (((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))){
                continue;
            }
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, de->d_name);
            collect_files(fullpath, 0, files, count, capacity);
        }
        closedir(dir);
    } else if ((S_ISREG(buf.st_mode))){
        //file
        if (is_explicit || has_suffix(path) == 1){
            file_wfd *new_file = process_file(path);
            if (new_file == NULL){
                return;
            }
            if (*count == *capacity){
                *capacity*=2;
                *files = realloc(*files, *capacity * sizeof(file_wfd *));
            }
            (*files)[*count] = new_file;
            (*count)++;
        }
    } 
}


int main(int argc, char *argv[]){
        if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_or_dir> ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    int count = 0;
    int capacity = 8;    
    file_wfd **files = malloc(capacity * sizeof(file_wfd *));
    if (!files){
        perror("malloc");
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; i++) {
        collect_files(argv[i], 1, &files, &count, &capacity);
    }
    for (int i = 0; i < count; i++) free_file_wfd(files[i]);
    free(files);

    return EXIT_SUCCESS;    
}
