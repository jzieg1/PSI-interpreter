#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

//main file for interpreting PSI LISP

typedef enum PType{
    BOOL=0,
    NUM=1,
    STR=2,
    SYM=3,
    LIST=4,
    CELL=5,
    FUNC=6,
    ERR=7
} PType;

//function calls are ordered on a stack
typedef struct CallStack{
    OpCall base[65536];
    int top;
} CallStack;

//every layer of the call stack should be a function and its parameters
//but I don't know what function it would be for or what parameters it would have
//keep it simple for now, start with math operators
//every math operator returns an int and takes three int pointers
typedef struct OpCall{
    int (*op)(int, int, int);
    int *operandA;
    int *operandB;
    int *out;
} OpCall;


int add(int* operandA, int* operandB, int* out){
    *out = *operandA + *operandB;
    return 0;
}

int subtract(int* operandA, int* operandB, int* out){
    *out = *operandA - *operandB;
    return 0;
}

int multiply(int* operandA, int* operandB, int* out){
    *out = *operandA * *operandB;
    return 0;
}

int divide(int* operandA, int* operandB, int* out){
    *out = *operandA / *operandB;
    return 0;
}

typedef struct pval{
    PType type;
    void* value;
    pval* next;
    bool is_end;
} pval;

pval* pval_new (PType ty) {
    pval* out = malloc(sizeof(pval));
    assert(out);
    out->type = ty;
    out->value = NULL;
    out->next = NULL;
    out->is_end = true;
    return out;
}

int pval_head(pval* cur) {
    if (cur->is_end) {
        return -1; // error condition -- head of empty list
    } else {
        return cur->value;
    }
}

pval* pval_next(pval* cur) {
    return cur->next; // returns NULL if list is empty
}

//add pval to end of existing list
pval* pval_member(pval* parent, void* value, PType ty){
    //initialize new pval
    pval* out = malloc(sizeof(pval));
    out->type = ty;
    out->value = value;
    out->next = NULL;
    out->is_end = true;

    //point parent pval to new pval
    parent->next = out;
    parent->is_end = false;
}

pval* pval_cons(unsigned char c, pval* tl, PType ty) {
    pval* out = malloc(sizeof(pval));
    assert(out);
    out->type = ty;
    out->value = c;
    out->next = tl;
    out->is_end = false;
    return out;
}

void pval_print(pval* cur) {
    //write tostring
    //should parse to string differently for each type
    if (!cur->is_end) {
        printf("%d ", cur->value);
        plist_print(cur->next);
    } else {
        printf("%d ", cur->value);
        printf("\n");
    }
}

char* pval_tostring(pval* p){
    if(p->type == BOOL){
        const bool *b = p->value;
        char* str = malloc(2);
        if(*b){
            char* t = "#t";
            strlcpy(str, t, 2);
            return str;
        } else {
            char* f = "#f";
            strlcpy(str, f, 2);
            return str;
        }
    } else if(p->type == NUM){
        int num = p->value;
        //when called with NULL and 0 as its first two parameters, snprintf returns how long a string would have to be
        // to hold the converted int
        int length = snprintf(NULL, 0, "%d", num);
        //allocate a string with as many bytes as length + 1 for the null terminator
        char* str = malloc(length + 1);
        snprintf(str, length+1, "%d", num);
        return str;
    } else if(p->type == STR){
        //strings are already allocated pointers but we're returning a separate allocated pointer just to be sure
        //the caller doesn't mess with the actual pval data
        //allocate memory equal to the base string length + 1, casting the void* to const char* for strlen
        char* str = malloc(strlen(p->value) + 1);
        return str;
    } else if(p->type == SYM){
        
    } else if(p->type == LIST){
        
    } else if(p->type == CELL){
        
    } else if(p->type == FUNC){
        
    } else if(p->type == ERR){
        
    } else {
        //print error
    }
}

//when we see a '(', run the code inside it if TOS = 0 or skip to matching ')' 
//without running enclosed code
int open_paren(stack* s, char* program, int read_pos){
    //if TOS = 0, skip ahead to matching close paren
    if(s->base[s->top] == 0){ 
        uint16_t paren_count = 1; //count of open parens
        //paren_count will not hit 0 until the matching close paren is read
        while(paren_count > 0){
            read_pos++;
            if(program[read_pos] == ')'){
                paren_count--;
            } else if(program[read_pos] == '('){
                paren_count++;
            }
        }
        //return read position, allowing main loop to read program from there
        return read_pos;
    } else { 
        //otherwise, return the read position unmodified
        return read_pos;
    }
}

//when we see a ')', skip to matching '(' using inverted form of open_paren
int close_paren(stack* s, char* program, int read_pos){
    uint16_t paren_count = 1; //count of open parens
    //we want to work backwards to find the open paren, so decrement paren_count for '('
    //and increment for ')'
    while(paren_count > 0){
        read_pos--;
        //printf("read_pos = %d\n", read_pos);
        if(program[read_pos] == ')'){
            paren_count++;
            //printf("paren_count = %d\n", paren_count);
        } else if(program[read_pos] == '('){
            paren_count--;
            //printf("paren_count = %d\n", paren_count);
        }
    }
    //move read counter to just before open paren so that the main loop increment
    //will move the counter onto the open paren before the next execution round
    read_pos--;
    //return position
    return read_pos;
}

//parse string input
//assuming input is one line of valid lisp, throw an error if it isn't
//read input into a list of pvals
int parse_input(char* input){
    //ok so we want to peel off every pair of parens into its own list
    //like, we see a paren, that's a sign to start a new list for everything in that paren
    //but the string starts with parens, what do we do with that
    //well, we open a list right? that's the thing we needed to do anyway
    //if TOS = 0, skip ahead to matching close paren

    //get whitespace-delimited string tokens
    char *token, *string, *tofree;

    tofree = string = strdup(input);
    assert(string != NULL);

    while((token = strsep(&string, " ")) != NULL){
        //replace with checks for if the string is a value or paren or whatever
        printf("%s\n", token);
    }

    free(tofree);


    if(s->base[s->top] == 0){ 
        uint16_t paren_count = 1; //count of open parens
        //paren_count will not hit 0 until the matching close paren is read
        while(paren_count > 0){
            read_pos++;
            if(program[read_pos] == ')'){
                paren_count--;
            } else if(program[read_pos] == '('){
                paren_count++;
            }
        }
        //return read position, allowing main loop to read program from there
        return read_pos;
    } else { 
        //otherwise, return the read position unmodified
        return read_pos;
    }
}

//(* 4 (+ 5 6))
int main(int argc, char** argv){
}

/* main list structure
centerpiece of the code
Could be implemented as linked list of linked lists or array of arrays
Either way, every node is some kind of pval pointer. First pointer should be a function pointer, following pointers should 
be to parameters or things that will become parameters.
Main advantage of list is that I don't have to know how many parameters will be used in advance, I can just chain them 
together arbitrarily.
With an array, I would have to know how many pointers to allocate for ahead of time or implement resizable arrays, which is 
annoying.
Also this is LISP, we should probably use lists.
 */