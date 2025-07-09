#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


//need to include an equality operator
//booleans
//terminate on (quit)
//read inputs as loop from console

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

//parse string input
//assuming input is one line of valid lisp, throw an error if it isn't
//read input into a list of pvals
//start param so we have something to append pvals to
int parse_input(char* input){
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

    //build stack for returned integers
    int stack[4096];
    int tos = 0;

    //type of operand the input string has at the front, should only be assigned once
    int op_type = 0; 
    
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
    printf("~~~~~ new loop ~~~~~\n");
    printf("Input string: %s\n", input);
    while((token = strsep(&string, " ")) != NULL){
        int str_int_output; //take output from is_string_int\

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

            int output = parse_input(trimmed_input);

            tos++;
            stack[tos] = output;

            free(trimmed_input); //once parse_input returns, there's no further need for trimmed_input
            //read forward to matching close paren, then take that string and call the parser on it

            //set string pointer equal to the end of the segment so that strsep is called from that point
            string = &tofree[segment_end+2]; 
        } 
        //check if token is an addition operator, but only assign
        else if(!strcmp(token, "+")){
            printf("operator: +\n");
            //if op_type is already not 0, there is a problem
            assert(op_type == 0);
            op_type = 1;
        }
        //repeat for other operator types
        else if(!strcmp(token, "-")){
            printf("operator: -\n");
            assert(op_type == 0);
            op_type = 2;
        }
        else if(!strcmp(token, "*")){
            printf("operator: *\n");
            assert(op_type == 0);
            op_type = 3;
        }
        else if(!strcmp(token, "/")){
            printf("operator: /\n");
            assert(op_type == 0);
            op_type = 4;
        }
        else if(is_string_int(token, &str_int_output)){
            printf("New token: %d\n", str_int_output);
            tos++;
            stack[tos] = str_int_output;
        }
        // else if(!strcmp(token, "#t")){
        //     tos++;
        //     stack[tos] = 1;
        // }
        // else if(!strcmp(token, "#f")){
        //     tos++;
        //     stack[tos] = 0;
        // }
        // else if(!strcmp(token, "quit")){
        //     return 0;
        // }

            printf("remaining string = %s\n", string);
        //basic tokening loop seems good, now to start breaking out other types
    }
    free(tofree);
    int out; 
    if(op_type == 1){
        printf("adding\n");
        out = 0;
        while(tos){
            out+= stack[tos];
            printf("tos = %d\n", tos);
            printf("operand = %d\n", stack[tos]);
            printf("result = %d\n", out);
            tos--;
        }
    }
    else if(op_type == 2){
        printf("subtracting\n");
        out = stack[1];
        printf("first term = %d\n", out);
        for(int i = 2; i <= tos; i++){
            out-= stack[i];
            printf("counter = %d\n", i);
            printf("operand = %d\n", stack[i]);
            printf("result = %d\n", out);
        }
    }
    else if(op_type == 3){
        printf("multiplying\n");
        out = 1;
        while(tos){
            out*= stack[tos];
            printf("tos = %d\n", tos);
            printf("operand = %d\n", stack[tos]);
            printf("result = %d\n", out);
            tos--;
        }
    }
    else if(op_type == 4){
        printf("dividing\n");
        out = stack[1];
        printf("first term = %d\n", out);
        for(int i = 2; i <= tos; i++){
            out/= stack[i];
            printf("counter = %d\n", i);
            printf("operand = %d\n", stack[i]);
            printf("result = %d\n", out);
        }
    }
    else{
        return 0;
    }
    printf("output = %d\n", out);
    printf("~~~~~ end loop ~~~~~\n");
    return out;
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

    parse_input(program_input);
    // char* a = "batman";
    // printf("%s\n", &a[2]);
}