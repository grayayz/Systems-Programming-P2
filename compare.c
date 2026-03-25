#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUFFIX ".txt"

//need a linked list of mappings between words and counts (later frequencies)/iterate in lexigocraphic order
typedef struct wfd_node{
    char *word;
    int count;
    double frequency;
    struct wfd_node *next;
} wfd_node;

//word storage (dynamically allocated):file's path, total word count, and its WFD
typedef struct{
    int total_word_count;
    char *file_path;
    wfd_node *node;
} file_wfd;


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
    new_node->word = strdup(current->word);
    new_node->count = 1;
    new_node->frequency = 0.0;
    new_node->next = current;
    if (prev == NULL){ //if there's nothing b4 current->word
        new_node = head; //new_node becomes the new head
    }
    prev->next = new_node;
    return head;
}


void frequency(wfd_node *head, int total_words){
    //frequency = ((# of times the word appears)/(total # of words))
    wfd_node *prev = NULL;
    wfd_node *current = head;

    while (current != NULL){
        current->frequency = ((current->count)/((double)total_words));
        prev = current;
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

