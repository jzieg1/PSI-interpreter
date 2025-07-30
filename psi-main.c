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

typedef struct pval{
    PType type;
    void* value;
    struct pval* next;
    bool is_end;
} pval;

pval* pval_cons(void* val, PType ty) {
    pval* out = malloc(sizeof(pval));
    if(out){
        out->type = ty;
        out->value = val;
        out->next = NULL;
        out->is_end = false;
    }
    return out;
}

//makes a pval of ambiguous type
//used for capturing output from PSI func calls
//all PSI func calls return a pval, but it's unknown to the execution function what type of pval should be returned
//so this makes one without a set type or value and lets the caller determine those on its own
pval* pval_new_ambig(){
    pval* out = malloc(sizeof(pval));
    out->next = NULL;
    out->is_end = false;
}

//deconstruct pval
void free_pval(pval* p){
    if(p->next){
        free_pval(p->next);
    }
    free(p->value);
    free(p);
}

int add(pval* list, pval* result){
    //allocate integer for sum
    int* sum = malloc(sizeof(int));
    if(!sum){
        return 1;
    }

    //start at 0 and add everything in the list
    *sum = 0;
    while(list->is_end == false){
        int* operand = list->value;
        *sum += *sum + *operand;
        list = list->next;
    }

    //set output value to sum
    result->value = sum;

    return 0;
}

int subtract(pval* list, pval* result){
    //allocate integer for sum
    int* sum = malloc(sizeof(int));
    if(!sum){
        return 1;
    }
    //set sum to first number in list
    *sum = *(int*)list->value;
    //if the is at least one other element, advance the list counter so we don't subtract the first element from itself
    if(list->is_end == false){
        list = list->next;
    }
    //subtract everything else from the first number
    while(list->is_end == false){
        int* operand = list->value;
        *sum += *sum - *operand;
        list = list->next;
    }

    //set output value to sum
    result->value = sum;

    return 0;
}

int multiply(pval* list, pval* result){
    //allocate integer for sum
    int* sum = malloc(sizeof(int));
    if(!sum){
        return 1;
    }
    //set sum to first number in list
    *sum = *(int*)list->value;
    //if the is at least one other element, advance the list counter so we don't multiply the first element by itself
    if(list->is_end == false){
        list = list->next;
    }
    //multiply first number by subsequent numbers
    while(list->is_end == false){
        int* operand = list->value;
        *sum += *sum - *operand;
        list = list->next;
    }

    result->value = sum;

    return 0;
}

int divide(pval* list, pval* result){
    //allocate integer for sum
    int* sum = malloc(sizeof(int));
    if(!sum){
        return 1;
    }
    //set sum to first number in list
    *sum = *(int*)list->value;
    //if the is at least one other element, advance the list counter so we don't multiply the first element by itself
    if(list->is_end == false){
        list = list->next;
    }
    //multiply first number by subsequent numbers
    while(list->is_end == false){
        int* operand = list->value;
        *sum += *sum - *operand;
        list = list->next;
    }

    result->value = sum;

    return 0;
}

int equals(pval* list, pval* result){
    int item1 = *(int*)list->value;
    if(list->is_end == false){
        list = list->next;
    }
    else{
        return 1;
    }
    int item2 = *(int*)list->value;
    if(list->is_end == false){
        return 1;
    }

    if(item1 == item2){
        *(result->value) = "#t"
    }
    else{
        *(result->value) = "#f"
    }
    return 0;
}

int quit(){
    exit(0);
}

//library of functions
void* library[6] = {&add, &subtract, &multiply, &divide, &equals, &quit};

//make a new pval meant to serve as the start of a list
pval* pval_new_header(){
    pval* out = malloc(sizeof(pval));
    if(out){
        out->type = START;
        out->value = NULL;
        out->next = NULL;
        out->is_end = true;
    }
    return out;
}

pval* pval_new_num(pval* parent, int val){
    pval* out = malloc(sizeof(pval));
    if(out){
        int* v = calloc(1, sizeof(int));
        if(v){
            *v = val;
            out->value = v;
        }
        else{
            free(out);
            return NULL;
        }
        out->type = NUM;
        out->next = NULL;
        out->is_end = true;
        parent->next = out;
        parent->is_end = false;
    }

    return out;
}

pval* pval_new_bool(pval* parent, bool val){
    pval* out = malloc(sizeof(pval));
    if(out){
        bool* b = calloc(1, sizeof(bool));
        if(out->value){
            *b = val;
            out->value = b;
        }
        else{
            free(out);
            return NULL;
        }
        out->type = BOOL;
        out->next = NULL;
        out->is_end = true;
        parent->next = out;
        parent->is_end = false;
    }

    return out;
}
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
pval* pval_new_string(pval* parent, char* str, int len){
    pval* out = malloc(sizeof(pval));
    if(out){
        out->value = calloc(len+1, sizeof(char));
        if(out->value){
            strlcpy(out->value, str, len);
        }
        else{
            free(out);
            return NULL;
        }
        out->type = STR;
        out->next = NULL;
        out->is_end = true;
        parent->next = out;
        parent->is_end = false;
    }

    return out;
}

pval* pval_new_func(pval* parent, void* func){
    pval* out = malloc(sizeof(pval));
    if(out){
        out->value = func;
        out->type = FUNC;
        out->next = NULL;
        out->is_end = true;
        parent->next = out;
        parent->is_end = false;
    }

    return out;
}

pval* pval_next(pval* cur) {
    return cur->next; // returns NULL if list is empty
}

//add pval to end of existing list
int pval_add(pval* parent, void* value, PType ty){
    //initialize new pval
    pval* out = malloc(sizeof(pval));
    out->type = ty;
    out->value = value;
    out->next = NULL;
    out->is_end = true;

    //point parent pval to new pval
    parent->next = out;
    parent->is_end = false;
    return 0;
}

char* pval_tostring(pval* p){
    if(p->type == BOOL){
        bool b = *(bool*)p->value;
        char* str = malloc(2);
        if(b){
            char* t = "#t";
            strlcpy(str, t, 2);
            return str;
        } else {
            char* f = "#f";
            strlcpy(str, f, 2);
            return str;
        }
    } else if(p->type == NUM){
        int num = *(int*)p->value;
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
        int len = strlen((char*)p->value) + 1;
        char* str = malloc(len);
        snprintf(str, len, "%s", (char*)p->value);
        return str;
    } else if(p->type == SYM){
        //symbol is just some set of characters that's been defined to equal something
        //symbols can be part of functions or cells 
        //symbol name should already be a printable string
        int len = strlen((char*)p->value) + 1;
        char* str = malloc(len);
        snprintf(str, len, "%s", (char*)p->value);
        return str;
    } else if(p->type == LIST){
        return NULL;
    } else if(p->type == CELL){
        return NULL;
    } else if(p->type == FUNC){
        return NULL;
    } else if(p->type == ERR){
        return NULL;
    } else {
        return NULL;
        //print error
    }
}

bool is_string_int(char* input, int* output){
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
        *output = number;
        return true;
    }
}

// //parse string input
// //assuming input is one line of valid lisp, throw an error if it isn't
// //read input into a list of pvals
// //start param so we have something to append pvals to
// int parse_input(char* input, pval* start){
//     //ok so we want to peel off every pair of parens into its own list
//     //like, we see a paren, that's a sign to start a new list for everything in that paren
//     //but the string starts with parens, what do we do with that
//     //well, we open a list right? that's the thing we needed to do anyway
//     //if TOS = 0, skip ahead to matching close paren

//     //this block checks for unbalanced parens
//     //should probably be broken out into a pre-check since it only needs to be run once per line
//     int paren_count = 0; //count of open parens
//     int i = 0;
//     while(input[i] != '\0'){
//         if(input[i] == '(') paren_count++;
//         else if (input[i] == ')') paren_count--;
//     }
//     if(paren_count != 0) return 0;
    
//     //get whitespace-delimited string tokens
//     char *token, *string, *tofree;

//     //allocate enough memory to hold the input string, copy the input string into that memory,
//     //then set tofree and string to the returned pointer
//     tofree = string = strdup(input);
//     //make sure allocation worked
//     assert(string != NULL);

//     //strsep replaces first instance of delimiter character in target string with '\0'
//     //and puts the pointer to the character after that in the variable of the first
//     //parameter while returning a pointer to the original value of the first parameter.
//     //Call in loop such that 'token' always holds the current delimited string segment.
//     //Stores NULL in first argument if end of string is reached, so the call after that
//     //will return NULL and break the loop.
//     while((token = strsep(&string, " ")) != NULL){
//         //replace with checks for if the string is a value or paren or whatever
//         if(token[0] == '('){
//             //string is getting chopped up with null terminators, so to copy a section of the raw input string we need to look at the unmodified input
//             //string. Except we're working with pointers here, not indices. We can get back to a character count index with pointer arithmetic.
//             //Then cut out the matching part of the input string and feed it to a new call of parse_input.
//             //Then have &string skipped ahead to the token after the close paren so it keeps going without issue.
//             int segment_start = token - tofree + 1; //gets index to character after '(', can be used to find it in the input string
//             int readPos = 0; //increment this integer as we read to preserve inputIndex
//             //shouldn't we convert to input indices after finding the increments of the two bounding parentheses instead of before?
//             paren_count = 1; //reset paren_count from earlier
//             while(paren_count > 0){
//                 readPos++;
//                 if(token[readPos] == ')'){
//                     paren_count--;
//                 } else if(program[readPos] == '('){
//                     paren_count++;
//                 }
//             }
//             int segment_end = token[readPos] - tofree;//gets the index of the closing ')' with reference to the overall input string
//             //readPos now equals the index of the matching ')'
//             //so now inputIndex to readPos completely bounds the string enclosed by the parens and we can cut it out of input
//             int trim_size = segment_end - segment_start + 1; //add 1 to make room for \0
//             char* trimmed_input = malloc(trim_size); //allocate memory 
//             strlcpy(trimmed_input, input[segment_start], trim_size); //copy the characters within the parens into the allocated string

//             //open a list pval and attach it to the last pval
//             pval* temp = pval_new_header();
//             pval* parsed_list = parse_input(trimmed_input, temp);
//             pval* ls = pval_add(start, parsed_list, PType.LIST);
//             free(trimmed_input); //once parse_input returns, there's no further need for trimmed_input
//             //read forward to matching close paren, then take that string and call the parser on it

//             //set string pointer equal to the end of the segment so that strsep is called from that point
//             string = token[segment_end]; 
//         }
//         //figure out token type
//         //start with checking if it's an int
//         else if(is_string_int(token)){
            
//         }
//         //basic tokening loop seems good, now to start breaking out other types
//     }

//     free(tofree);


//     if(s->base[s->top] == 0){ 
//         //paren_count will not hit 0 until the matching close paren is read
//         while(paren_count > 0){
//             read_pos++;
//             if(program[read_pos] == ')'){
//                 paren_count--;
//             } else if(program[read_pos] == '('){
//                 paren_count++;
//             }
//         }


//         return read_pos;
//     } else { 
//         //otherwise, return the read position unmodified
//         return read_pos;
//     }
// }


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

//need to include an equality operator
//booleans
//terminate on (quit)
//read inputs as loop from console

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

    //something wrong here

    //this block checks for unbalanced parens
    //should probably be broken out into a pre-check since it only needs to be run once per line
    int paren_count = 0; //count of open parens
    int i = 0;
    while(input[i] != '\0'){
        if(input[i] == '(') paren_count++;
        else if (input[i] == ')') paren_count--;
        i++;
    }
    if(paren_count != 0) return 1;
    
    //get whitespace-delimited string tokens
    char *token, *string, *tofree;

    //allocate enough memory to hold the input string, copy the input string into that memory,
    //then set tofree and string to the returned pointer
    tofree = string = strdup(input);
    //make sure allocation worked
    assert(string != NULL);

    //a list should have at most one function at the start of the list, in any other condition a function should trigger a crash
    bool on_first = true; //flips to one after first token read

    //the most recently appended pval to which new pvals will be added, initialized to start
    pval* current = start;

    //strsep replaces first instance of delimiter character in target string with '\0'
    //and puts the pointer to the character after that in the variable of the first
    //parameter while returning a pointer to the original value of the first parameter.
    //Call in loop such that 'token' always holds the current delimited string segment.
    //Stores NULL in first argument if end of string is reached, so the call after that
    //will return NULL and break the loop.
    printf("~~~~~ new loop ~~~~~\n");
    printf("Input string: %s\n", input);
    while((token = strsep(&string, " ")) != NULL){
        int str_int_output; //take output from is_string_int

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
                } else if(token[readPos] == '('){
                    paren_count++;
                }
            }
            int segment_end = &token[readPos] - tofree;//gets the index of the closing ')' with reference to the overall input string
            //readPos now equals the index of the matching ')'
            //so now inputIndex to readPos completely bounds the string enclosed by the parens and we can cut it out of input
            int trim_size = segment_end - segment_start + 1; //add 1 to make room for \0
            char* trimmed_input = malloc(trim_size); //allocate memory 
            strlcpy(trimmed_input, &input[segment_start], trim_size); //copy the characters within the parens into the allocated string

            //make a new start pval
            pval* new_list_start = pval_new_header();
            //add a new pval of type LIST to the current chain of pvals with the new start pval as its value
            pval_add(current, new_list_start, LIST);
            //parse the next chunk of string with the new start pval
            int output = parse_input(trimmed_input, new_list_start);

            free(trimmed_input); //once parse_input returns, there's no further need for trimmed_input
            //read forward to matching close paren, then take that string and call the parser on it

            //set string pointer equal to the end of the segment so that strsep is called from that point
            string = &tofree[segment_end+2]; 
        } 
        //check if token is an addition operator, but only assign
        else if(!strcmp(token, "+")){
            printf("operator: +\n");
            pval_new_func(current, &add);
            printf("made new addition pval\n");
            //if op_type is already not 0, there is a problem
            current = pval_next(current);
        }
        //repeat for other operator types
        else if(!strcmp(token, "-")){
            printf("operator: -\n");
            pval_new_func(current, &subtract);
            current = pval_next(current);
        }
        else if(!strcmp(token, "*")){
            printf("operator: *\n");
            pval_new_func(current, &multiply);
            current = pval_next(current);
        }
        else if(!strcmp(token, "/")){
            printf("operator: /\n");
            pval_new_func(current, &divide);
            current = pval_next(current);
        }
        else if(!strcmp(token, "=")){
            printf("operator: =\n");
            pval_new_func(current, &equals);
            current = pval_next(current);
        }
        else if(is_string_int(token, &str_int_output)){
            printf("New token: %d\n", str_int_output);
            pval_new_num(current, str_int_output);
            current = pval_next(current);
        }
        else if(!strcmp(token, "#t")){
            bool b = true;
            pval_new_bool(current, b);
            current = pval_next(current);
        }
        else if(!strcmp(token, "#f")){
            bool b = false;
            pval_new_bool(current, b);
            current = pval_next(current);
        }
        else if(!strcmp(token, "quit")){
            //feel like this should run some free's first
            exit(0);
        }
        on_first = false;

        printf("remaining string = %s\n", string);
        //basic tokening loop seems good, now to start breaking out other types
    }
    free(tofree);
    printf("~~~~~ end loop ~~~~~\n");
    return 0;
}

int execute_list(pval* list){
    //ok, so we want to read a list and recursively execute everything in it until it returns a final value I guess
    //need to be able to allocate variables and all kinds of jazz
    //we need a stack variable outside of everything
    //or at least something where we can stash allocations of memory
    //don't really want to pass that around constantly
    //Should probably declare that in file scope
    //Otherwise, we need to think about how to read and return values
    //Want int return for error code stuff
    //Need to pass in a pval to start with obviously
    //Start at the top of the pval and consume downwards
    //Want to read the function and then hand the rest of the pvals in the list to that function
    //Not too hard, just make an addition function that takes a pval* and then hand off the list item after the function
    //How to handle internal parentheses though?
    //Would naturally want to execute that sublist and replace it with a single pval representing the results
    //But that has to happen as a part of the main execution loop. If we just pass the whole thing to add() or something
    //the other function won't know what to do with a sublist with its own function calls.
    //Execution should follow every sublist down and only start actually executing once it hits the bottom level. 
    //That way every function only sees pvals it knows how to deal with.
    //So then, what does our execution loop look like?
    //All lists start with functions, except the empty list, which evaluates to #f.
    
    pval* cursor;
    //expect that provided pval will be of type LIST
    if(list->type == LIST){
        //get the actual start of the list
        pval* start = list->value;
        //start value should be type START
        if(start->type == START && start->is_end == false){
            //start value is empty, so skip ahead again
            cursor = start->next;
        }
        else{
            return 1;
        }
    }
    else{
        return 1;
    }

    //cursor should currently be pointing to a function
    pval* func;

    //quit has no parameters so make a special carve-out condition for it
    if(func->value == &quit){
        quit();
    }

    if(cursor->type == FUNC && cursor->is_end == false){
        func = cursor;
        cursor = cursor->next;
    }
    else{
        return 1;
    }

    //we can make our execution work by making every function one pval* as input, one pval* as output parameter, and return int for error codes
    int (*f)(pval*, pval*) = func->value;
    //make output pointer
    pval* out = pval_new_ambig();
    //call with pointer to next 
    int ret = f(func->next, out);
    
}

int main(int argc, char** argv){
    if(argc < 2){ //return with error if no argument is given
        printf("Error: No filename given.\n");
        return 5;
    }

    FILE* input_file = fopen(argv[1], "r");
    if(!input_file){ //return with error if argv[1] does not point to a file
        printf("Error: Unable to open file.\n");
        return 4;
    }
    
    char program_input[4096];
    fgets(program_input, sizeof program_input, input_file);

    pval* start = pval_new_header();
    int ret_code = parse_input(program_input, start);
    if(ret_code){
        printf("There was an error\n");
    }
    // char* a = "batman";
    // printf("%s\n", &a[2]);
}