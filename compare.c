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
    if (prev = NULL){ //if there's nothing b4 current->word
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
