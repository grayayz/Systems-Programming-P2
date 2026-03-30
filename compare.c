#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

typedef struct{
    const char *path_a;
    const char *path_b;
    int combined_word_count;
    double jsd;
} pair_analysis;

/* how many unordered distinct file pairs exist for n files 
(n choose 2). */
size_t analysis_pair_count( size_t n ){
    if ( n < 2 )
        return 0;
    return n * ( n - 1 ) / 2;
}

/* goes through a WFD list 
asserts each word frequency is in [0, 1]. */
static void assert_wfd_frequencies_in_range( const wfd_node *node ){
    for ( ; node != NULL; node = node->next )
        assert( node->frequency >= 0.0 && node->frequency <= 1.0 );
}

/* JSD between two WFDs  
merging their sorted word lists. */
static double jsd_between_wfds( const wfd_node *a, const wfd_node *b ){
    double kld1 = 0.0;
    double kld2 = 0.0;
    const wfd_node *pa = a;
    const wfd_node *pb = b;

    while ( pa != NULL || pb != NULL ){
        int cmp;
        if ( pb == NULL )
            cmp = -1;
        else if ( pa == NULL )
            cmp = 1;
        else
            cmp = strcmp( pa->word, pb->word );

        double f1 = 0.0;
        double f2 = 0.0;

        if ( cmp < 0 ){
            f1 = pa->frequency;
            assert( f1 >= 0.0 && f1 <= 1.0 );
            pa = pa->next;
        } else if ( cmp > 0 ){
            f2 = pb->frequency;
            assert( f2 >= 0.0 && f2 <= 1.0 );
            pb = pb->next;
        } else {
            f1 = pa->frequency;
            f2 = pb->frequency;
            assert( f1 >= 0.0 && f1 <= 1.0 );
            assert( f2 >= 0.0 && f2 <= 1.0 );
            pa = pa->next;
            pb = pb->next;
        }

        double mean = 0.5 * ( f1 + f2 );
        assert( mean > 0.0 || ( f1 == 0.0 && f2 == 0.0 ) );
        if ( f1 > 0.0 )
            kld1 += f1 * log2( f1 / mean );
        if ( f2 > 0.0 )
            kld2 += f2 * log2( f2 / mean );
    }

    return sqrt( 0.5 * kld1 + 0.5 * kld2 );
}

/* qsort comparator: 
higher combined word 
count sorts first (descending). */
static int pair_analysis_sort_cmp( const void *va, const void *vb ){
    const pair_analysis *x = va;
    const pair_analysis *y = vb;
    if ( x->combined_word_count > y->combined_word_count )
        return -1;
    if ( x->combined_word_count < y->combined_word_count )
        return 1;
    return 0;
}

/* exits the program if fewer than two files were collected 
(analysis needs a pair). */
void analysis_exit_if_too_few_files( size_t n ){
    if ( n < 2 ){
        fprintf( stderr, "compare: fewer than two input files after collection\n" );
        exit( EXIT_FAILURE );
    }
}

/* fills results with JSD 
combines counts for each i<j pair
then qsorts by combined count. */
void analysis_phase( file_wfd *files, size_t n, pair_analysis *results ){
    analysis_exit_if_too_few_files( n );

    size_t k = 0;
    for ( size_t i = 0; i < n; i++ ){
        assert_wfd_frequencies_in_range( files[i].node );
        for ( size_t j = i + 1; j < n; j++ ){
            assert_wfd_frequencies_in_range( files[j].node );
            results[k].path_a = files[i].file_path;
            results[k].path_b = files[j].file_path;
            results[k].combined_word_count =
                files[i].total_word_count + files[j].total_word_count;
            results[k].jsd = jsd_between_wfds( files[i].node, files[j].node );
            k++;
        }
    }

    qsort( results, k, sizeof ( pair_analysis ), pair_analysis_sort_cmp );
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
    if (new_file->total_word_count > 0)
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
            if (de->d_name[0] == '.'){
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

    analysis_exit_if_too_few_files( (size_t)count );

    size_t n = (size_t)count;
    size_t num_pairs = analysis_pair_count( n );
    pair_analysis *results = malloc( num_pairs * sizeof ( pair_analysis ) );
    file_wfd *files_row = malloc( n * sizeof ( file_wfd ) );
    if ( !results || !files_row ){
        perror( "malloc" );
        free( results );
        free( files_row );
        for ( int i = 0; i < count; i++ )
            free_file_wfd( files[i] );
        free( files );
        return EXIT_FAILURE;
    }
    for ( size_t i = 0; i < n; i++ )
        files_row[i] = *files[i];

    analysis_phase( files_row, n, results );
    free( files_row );

    for ( size_t i = 0; i < num_pairs; i++ )
        printf( "%.5f %s %s\n", results[i].jsd, results[i].path_a, results[i].path_b );

    free( results );

    for (int i = 0; i < count; i++) free_file_wfd(files[i]);
    free(files);

    return EXIT_SUCCESS;    
}
