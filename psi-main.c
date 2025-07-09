#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

//main file for interpreting PSI LISP

typedef enum PType{
    BOOL=0,
    NUM=1,
    STR=2,
    SYM=3,
    LIST=4,
    CELL=5,
    FUNC=6,
    ERR=7,
    START=8
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

//make a new pval meant to serve as the start of a list
pval* pval_new(){
    pval* out = malloc(sizeof(pval));
    assert(out);
    out->type = PType.START;
    out->value = NULL;
    out->next = NULL;
    out->is_end = true;
    return out;
}

// pval* pval_new (PType ty) {
//     pval* out = malloc(sizeof(pval));
//     assert(out);
//     out->type = ty;
//     out->value = NULL;
//     out->next = NULL;
//     out->is_end = true;
//     return out;
// }

//constructor is tricky because we need to allocate a void*
//we don't know how big the thing needs to be
//we do get given a type though, could just malloc depending on if statement
//and we don't have to handle strings yet
//oh shit right, strings would need a length parameter
//So? Make more constructors.
//Want to keep things constrained such that there's still one pval struct though
//Wait, do I need a length param? I mean I do if the pval is allocating memory for 
//strings on its own. But if it assumes it gets passed allocated pointers from outside,
//I don't. The spec said strings are supposed to be immutable, so I don't need to worry
//about extensions. And as long as I keep to the convention of always passing allocated
//memory, it won't cause any bugs.
//Leaving a struct's memory management to external code just feels wrong.
//Again, I can just make a constructor for each type. It would make the code more 
//legible too. I wouldn't have to pass PType anymore because I could just bake the type
//into the constructor, so overall parameter count would decrease.
pval* pval_string_cons(pval* tl, char* str, int len){
    pval* out = malloc(sizeof(pval));
    assert(out);
    char* val = malloc(len);
    strlcpy(val, str, len);

    out->type = STR;
    out->value = val;
    out->next = tl;
    out->is_end = false;
    return out;
}

pval* pval_cons(void* val, pval* tl, PType ty) {
    pval* out = malloc(sizeof(pval));
    assert(out);
    out->type = ty;
    out->value = val;
    out->next = tl;
    out->is_end = false;
    return out;
}

//deconstruct pval
void free_pval(pval* p){
    free(p->value);
    if(p->next){
        free_pval(p->next);
    }
    free(p);
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
pval* pval_add(pval* parent, void* value, PType ty){
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

//add pval to end of existing list
pval* pval_add(pval* parent, void* value, PType ty){
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
        const bool *b = (bool)p->value;
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
        int num = (int)p->value;
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
        int len = strlen((*char)p->value) + 1);
        char* str = malloc(len);
        snprintf(str, len, "%s", (*char)p->value);
        return str;
    } else if(p->type == SYM){
        //symbol is just some set of characters that's been defined to equal something
        //symbols can be part of functions or cells 
        //symbol name should already be a printable string
        int len = strlen((*char)p->value) + 1);
        char* str = malloc(len);
        snprintf(str, len, "%s", (*char)p->value);
        return str;
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
        //this paren counter thing works fine for this code

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

bool is_string_int(char* input){
    char* endptr = NULL;
    int number = (int)strtol(input, &endptr, 10);

    //there was no number in the string
    if(input == endptr){
        return false;
    } 
    //a number was read but not all characters in the string were numerical
    else if (errno == 0 && input && *endptr != 0){ 
        return false;
    }
    //everything went fine, the string is a number
    else{
        return true;
    }
}

//parse string input
//assuming input is one line of valid lisp, throw an error if it isn't
//read input into a list of pvals
//start param so we have something to append pvals to
int parse_input(char* input, pval* start){
    //ok so we want to peel off every pair of parens into its own list
    //like, we see a paren, that's a sign to start a new list for everything in that paren
    //but the string starts with parens, what do we do with that
    //well, we open a list right? that's the thing we needed to do anyway
    //if TOS = 0, skip ahead to matching close paren

    //this block checks for unbalanced parens
    //should probably be broken out into a pre-check since it only needs to be run once per line
    int paren_count = 0; //count of open parens
    int i = 0;
    while(input[i] != '\0'){
        if(input[i] == '(') paren_count++;
        else if (input[i] == ')') paren_count--;
    }
    if(paren_count != 0) return 0;
    
    //get whitespace-delimited string tokens
    char *token, *string, *tofree;

    //allocate enough memory to hold the input string, copy the input string into that memory,
    //then set tofree and string to the returned pointer
    tofree = string = strdup(input);
    //make sure allocation worked
    assert(string != NULL);

    //strsep replaces first instance of delimiter character in target string with '\0'
    //and puts the pointer to the character after that in the variable of the first
    //parameter while returning a pointer to the original value of the first parameter.
    //Call in loop such that 'token' always holds the current delimited string segment.
    //Stores NULL in first argument if end of string is reached, so the call after that
    //will return NULL and break the loop.
    while((token = strsep(&string, " ")) != NULL){
        //replace with checks for if the string is a value or paren or whatever
        if(token[0] == '('){
            //string is getting chopped up with null terminators, so to copy a section of the raw input string we need to look at the unmodified input
            //string. Except we're working with pointers here, not indices. We can get back to a character count index with pointer arithmetic.
            //Then cut out the matching part of the input string and feed it to a new call of parse_input.
            //Then have &string skipped ahead to the token after the close paren so it keeps going without issue.
            int segment_start = token - tofree + 1; //gets index to character after '(', can be used to find it in the input string
            int readPos = 0; //increment this integer as we read to preserve inputIndex
            //shouldn't we convert to input indices after finding the increments of the two bounding parentheses instead of before?
            paren_count = 1; //reset paren_count from earlier
            while(paren_count > 0){
                readPos++;
                if(token[readPos] == ')'){
                    paren_count--;
                } else if(program[readPos] == '('){
                    paren_count++;
                }
            }
            int segment_end = token[readPos] - tofree;//gets the index of the closing ')' with reference to the overall input string
            //readPos now equals the index of the matching ')'
            //so now inputIndex to readPos completely bounds the string enclosed by the parens and we can cut it out of input
            int trim_size = segment_end - segment_start + 1; //add 1 to make room for \0
            char* trimmed_input = malloc(trim_size); //allocate memory 
            strlcpy(trimmed_input, input[segment_start], trim_size); //copy the characters within the parens into the allocated string

            //open a list pval and attach it to the last pval
            pval* temp = pval_new();
            pval* parsed_list = parse_input(trimmed_input, temp);
            pval* ls = pval_add(start, parsed_list, PType.LIST);
            free(trimmed_input); //once parse_input returns, there's no further need for trimmed_input
            //read forward to matching close paren, then take that string and call the parser on it

            //set string pointer equal to the end of the segment so that strsep is called from that point
            string = token[segment_end]; 
        }
        //figure out token type
        //start with checking if it's an int
        else if(is_string_int(token)){
            
        }
        //basic tokening loop seems good, now to start breaking out other types
    }

    free(tofree);


    if(s->base[s->top] == 0){ 
        //paren_count will not hit 0 until the matching close paren is read
        while(paren_count > 0){
            read_pos++;
            if(program[read_pos] == ')'){
                paren_count--;
            } else if(program[read_pos] == '('){
                paren_count++;
            }
        }


        return read_pos;
    } else { 
        //otherwise, return the read position unmodified
        return read_pos;
    }
}


        //main difference is here
        //Instead of skipping ahead for a while loop or if statement, I want to make a 
        //new list. Once I see a new paren, I should make a new list pval. That list
        //should contain every other pval found until the matching close paren. After
        //hitting the close paren, the list closes and we go back to putting new pvals
        //on the next property.

        //One good way to do this would be to "read forward" on hitting a paren until
        //hitting the matching close paren, then recursively call parse on that.
        //We would have to make sure that parse could also take a pval as a starter 
        //that it could add on to, or alternatively have parse return a pval that can
        //then be appended to the value property of a list pval. The second one sounds
        //easier.

        //after parsing, have list of pvals
        //execute every list
        //collapse list into return value
        //return read position, allowing main loop to read program from there

        //Thinking more about parsing, LISP syntax doesn't have a space between
        //parens and containing tokens, it tends to look like (+ 2 (* 3 4)).
        //This means parsing needs to either know to separate out parens beyond normal
        //space delimiting or it needs to peel off the first paren layer.
        //Second case meaning that the parser would get (+ 2 (* 3 4)) and immediately 
        //turn it into + 2 (* 3 4).
        //This also means that in space-by-space parsing we will never see a '(' by itself,
        //it's always going to be concatenated with something else.
        //Solution: check every token to see if it starts with a paren.
        //If it does, prune the paren from the token and start a plist.
        //Wait, is that a good idea? Parsing method should be built for recursion. We
        //don't want to risk making a plist twice in response to one paren.
        //I was thinking the parser when seeing a paren, would make a new plist and then
        //feed the contained string into another parser call. 
        //Biggest issue is that if seeing an open paren triggers the recursion, we can't have 
        //the parser input start with a paren.
        //The parser needs to strip out the parens before calling itself on the remaining input.
        //The parser could technically start by recursing on itself and opening a plist such that
        //every list of pvals starts with a plist. That's not wrong exactly, but it feels vaguely
        //dumb. It feels like it makes more sense to have an outer first step trim the input string.
        //Wait, what if it's not dumb actually? Ensuring the output always starts with a specific 
        //pval type could be useful.

        //Parser behavior on reading a token would then be
        //1) check if first character in token is an open paren
        //1a) if it is, call parser on string inside parens (not including the parens that contain it)
        //2) figure out what type of token it should be 
        //3) make a pval out of the token and stick it on the last pval
        //3a) wait, how do we know that the last pval was? What if the last pval was a list?
        //4) read next token and repeat until end of string

        //Parser calls shouldn't necessarily return anything since they're allocating objects onto a list.

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