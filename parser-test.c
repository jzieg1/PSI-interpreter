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

typedef union p_data{
    int n;
    bool b;
} p_data;

typedef struct pval{
    PType type;
    p_data data;
} pval;

pval* new_pval_int(int val, PType ty) {
    pval* out = malloc(sizeof(pval));
    if(out){
        out->type = ty;
        out->data.n = val;
    }
    return out;
}

pval* new_pval_bool(bool val, PType ty) {
    pval* out = malloc(sizeof(pval));
    if(out){
        out->type = ty;
        out->data.b = val;
    }
    return out;
}

//deconstruct pval
void free_pval(pval* p){
    free(p);
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

//parse string input
//assuming input is one line of valid lisp, throw an error if it isn't
//read input into a list of pvals
//start param so we have something to append pvals to
pval* parse_input(char* input){
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
    if(paren_count != 0) return NULL;

    //build stack for returned pvals
    pval* stack[4096];
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

            pval* output = parse_input(trimmed_input);
            
            // if(output->type == NUM){
            //     printf("output = %d\n", output->data.n);
            // }
            // else if (output->type == BOOL){
            //     if(output->data.b == true){
            //         printf("output = %s\n", "#t");
            //     }else if(output->data.b == false){
            //         printf("output = %s\n", "#f");
            //     }
            // } 

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
        else if(!strcmp(token, "=")){
            printf("operator: =\n");
            assert(op_type == 0);
            op_type = 5;
        }
        else if(is_string_int(token, &str_int_output)){
            printf("New token: %d\n", str_int_output);
            tos++;
            stack[tos] = new_pval_int(str_int_output, NUM);
        }
        else if(!strcmp(token, "#t")){
            tos++;
            stack[tos] = new_pval_bool(true, BOOL);
        }
        else if(!strcmp(token, "#f")){
            tos++;
            stack[tos] = new_pval_bool(false, BOOL);
        }
        else if(!strcmp(token, "quit")){
            return 0;
        }

        //printf("remaining string = %s\n", string);
        //basic tokening loop seems good, now to start breaking out other types
    }
    free(tofree);
    pval* out; 

    //return first element of stack if no operator was provided
    if(op_type == 0){
        printf("optype 0\n");
        printf("bottom of stack = %d\n", stack[0]->data.n);
        return stack[0];
    }

    if(op_type == 1){
        printf("adding\n");
        out = new_pval_int(0, NUM);
        while(tos){
            if(stack[tos]->type != NUM){
                printf("Type error");
                return NULL;
            }
            out->data.n += stack[tos]->data.n;
            printf("tos = %d\n", tos);
            printf("operand = %d\n", stack[tos]->data.n);
            printf("result = %d\n", out->data.n);
            tos--;
        }
    }
    else if(op_type == 2){
        if(stack[1]->type != NUM){
            printf("Type error");
            return NULL;
        }
        printf("subtracting\n");
        out = new_pval_int(stack[1]->data.n, NUM);
        printf("first term = %d\n", out->data.n);
        for(int i = 2; i <= tos; i++){
            if(stack[i]->type != NUM){
                printf("Type error");
                return NULL;
            }
            out->data.n -= stack[i]->data.n;
            printf("counter = %d\n", i);
            printf("operand = %d\n", stack[i]->data.n);
            printf("result = %d\n", out->data.n);
        }
    }
    else if(op_type == 3){
        printf("multiplying\n");
        out = new_pval_int(1, NUM);
        while(tos){
            if(stack[tos]->type != NUM){
                printf("Type error");
                return NULL;
            }
            out->data.n *= stack[tos]->data.n;
            printf("tos = %d\n", tos);
            printf("operand = %d\n", stack[tos]->data.n);
            printf("result = %d\n", out->data.n);
            tos--;
        }
    }
    else if(op_type == 4){
        if(stack[1]->type != NUM){
            printf("Type error");
            return NULL;
        }
        printf("dividing\n");
        out = new_pval_int(stack[1]->data.n, NUM);
        printf("first term = %d\n", out->data.n);
        for(int i = 2; i <= tos; i++){
            if(stack[i]->type != NUM){
                printf("Type error");
                return NULL;
            }
            out->data.n /= stack[i]->data.n;
            printf("counter = %d\n", i);
            printf("operand = %d\n", stack[i]->data.n);
            printf("result = %d\n", out->data.n);
        }
    }
    else if(op_type == 5){
        //comparison works on two things, so top of stack should be exactly 2
        if(tos != 2){
            printf("parameter count error");
            return NULL;
        }
        out = new_pval_bool(true, BOOL);

        //data can be compared with .n regardless of type because the bits will read the same
        //need to also compare by type so that we don't end up with 1 == #t evaluating to true
        if(stack[1]->data.n == stack[2]->data.n && stack[1]->type == stack[2]->type){
            out->data.b = true;
        }
        else{
            out->data.b = false;
        }
    }
    else{
        return 0;
    }
    if(out->type == NUM){
        printf("output = %d\n", out->data.n);
        printf("output pointer = %p\n", &out);
        return out;
    }
    else if (out->type == BOOL){
        if(out->data.b == true){
            printf("output = %s\n", "#t");
        }else if(out->data.b == false){
            printf("output = %s\n", "#f");
        }
    }
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
    
    char program_input[4096] = {0};
    
    while(fgets(program_input, sizeof program_input, input_file) != NULL){
        //paren balancing check
        int paren_count = 0; //count of open parens
        int i = 0;
        printf("string length %d\n", strlen(program_input));
        while(program_input[i] != '\0'){
            if(program_input[i] == '(') paren_count++;
            else if (program_input[i] == ')') paren_count--;
            i++;
        }
        if(paren_count != 0){
            printf("Paren error");
            break;
        }

        /*
            int segment_end = &token[readPos] - tofree;//gets the index of the closing ')' with reference to the overall input string
            //readPos now equals the index of the matching ')'
            //so now inputIndex to readPos completely bounds the string enclosed by the parens and we can cut it out of input
            int trim_size = segment_end - segment_start + 1; //add 1 to make room for \0
            char* trimmed_input = malloc(trim_size); //allocate memory 
            strlcpy(trimmed_input, &input[segment_start], trim_size); //copy the characters within the parens into the allocated string
        */

        //strip outer parens if they exist to prepare string for reading
        printf("input1 = %s\n", program_input);
        printf("first char %c\n", program_input[0]);
        printf("second char %c\n", program_input[strlen(program_input)-1]);
        if(program_input[0] == '(' && program_input[strlen(program_input)-1] == ')'){
            printf("triggered if\n");
            memmove(program_input, program_input+1, strlen(program_input)-2);
            program_input[strlen(program_input)-2] = '\0';
            program_input[strlen(program_input)] = '\0';
        }

        printf("input2 = %s\n", program_input);
        pval* out = parse_input(program_input);
        printf("pointer = %p\n", out);
        if(out == NULL){
            printf("Returned null\n");
            continue;
        }
        if(out->type == NUM){
            printf("output = %d\n", out->data.n);
        }
        else if (out->type == BOOL){
            if(out->data.b == true){
                printf("output = %s\n", "#t");
            }else if(out->data.b == false){
                printf("output = %s\n", "#f");
            }
        } 
    }

    fclose(input_file);
}