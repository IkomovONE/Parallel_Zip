#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>          //importing libraries



void message_printer(const char *msg) {      //Function for printing errors and messages
    fprintf(stderr, "%s\n", msg);
}




int main(int arg_counter, char *arg_select[]) {

    
    
    if (arg_counter < 2) {           //Handling incorrect usage cases by throwing usage suggestion

        message_printer("usage: pzip <file1> <file2> ... > <outputFile>");


        exit(1);
    }




    for (int i = 1; i < arg_counter; i++) {
        
        int fd = open(arg_select[i], O_RDONLY);


    }





























    return 0;

}



