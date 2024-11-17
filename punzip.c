#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>          //importing libraries
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>



typedef struct {          //initializing certain data structure for each thread, with start of data segment, its length, and thread index
    char *seg_start;
    size_t seg_length;
    size_t th_index; 
    char *output_buffer; 
    size_t output_length; 
} Th_data;



void message_printer(const char *msg) {      //Function for printing errors and messages
    fprintf(stderr, "%s\n", msg);
}





//------Segment decompression function--------


void *seg_decompression(void *th_arg) {    //Establishing function for segment decompression

    Th_data *seg_data = (Th_data *)th_arg;   //establishing thread data instance
    char *seg_start = seg_data->seg_start;        //initializing segment starting point
    size_t seg_length = seg_data->seg_length;  //initializing length of the segment
    size_t th_index = seg_data->th_index;   //Initializing thread index value for possible troubleshooting


    //creating allocation for decompressed segment data

    char *decomp_output = malloc(seg_length * 10); // Allocating memory for the output, multiplying the length of the segment so that there is always enough memory

    if (!decomp_output) {                 //Adding error handling in case allocation fails

        message_printer("malloc failed");

        pthread_exit(NULL);
    }

    size_t decomp_buff_position = 0; //initializing variable for positioning in the buffer

    for (size_t i = 0; i < seg_length;) {   //Using for-loop to go over each encoded character inside the segment
        
        if (i + sizeof(int) + 1 > seg_length) { //Print error message if there's space for the 4-byte count reading

            message_printer("Error, the file seems to be corrupted. The size for the count encoding doesn't match!");
            
            free(decomp_output);
            
            pthread_exit(NULL);
        }


        int c_count;   //Initializing count value for RLE encoding. 

        //Reading the count from the pair

        memcpy(&c_count, seg_start + i, sizeof(int));

        i += sizeof(int); // Moving the index to the character

        // Reading the character value by inititalizing the char variable

        char enc_character = *(seg_start + i);

        i++; // Moving the index to the next pair

        // Checking if the count value is correct
        if (c_count < 0 || decomp_buff_position + c_count > seg_length * 10) {

            message_printer("Error, count value is incorrect. Corrupted File!");

            free(decomp_output);

            pthread_exit(NULL);
        }

        // Using for-loop to write the character into the buffer x number of times
        for (int j = 0; j < c_count; j++) {

            decomp_output[decomp_buff_position++] = enc_character;

        }
    }


    seg_data->output_buffer = decomp_output;     //Writing the result into the buffer
    seg_data->output_length = decomp_buff_position;   //Updating the buffer position 



    pthread_exit(NULL);  //exiting thread
}











//-------Main function-------





int main(int arg_counter, char *arg_select[]) {   //establishing the main function

    
    
    if (arg_counter < 2) {           //Handling incorrect usage cases by throwing usage suggestion

        message_printer("usage: punzip <file1> <file2> ... > <outputFile1> <outputFile2>");  //calling message printer to print correct usage


        exit(1);   //exiting the program
    }




    //Next step is to establish the threads, doing it based on the number of processors system has

    int th_number  = sysconf(_SC_NPROCESSORS_ONLN);   //getting number of threads by getting the number of processors of the system. Using sysconf function for this purpose.

    

    if (th_number < 1) {   //in case sysconf fails, we set the number of threads to 1 as default number (1 thread)
        th_number = 1; 
    }


    pthread_t threads[th_number];   //Initializing the threads based on the established amount

    Th_data th_data[th_number];    //Establishing Th_data instances (amount of instances based on thread number). Th_data data structure has been initialized earlier.

    



    for (int i = 1; i < arg_counter; i++) {     //establishing a loop for all the arguments (input files)
        
        int input_file = open(arg_select[i], O_RDONLY);  //opening the file (one by one as it's a loop)


        if (input_file < 0) {   //Error handling for file opening

            message_printer("Could not open the file");

            exit(1);
        }


        struct stat file_stats;   //initializing instance for collecting file statistics (for example size)



        if (fstat(input_file, &file_stats) != 0) {   //Error handling for file statistics

            message_printer("Couldn't get the size of the file");

            close(input_file);

            exit(1);
        }



        char *data = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, input_file, 0);  //mapping file data into memory for easy access

        if (data == MAP_FAILED) {    //Error handling for file data mapping

            message_printer("Couldn't map the file data to memory");

            close(input_file);

            exit(1);
        }


        
        size_t seg_size = file_stats.st_size / th_number;  //establishing segments' sizes based on file size divided by number of threads, so that each thread gets equal segment



        for (int k = 0; k < th_number; k++) {

            size_t seg_start_offset = k * seg_size;   //Getting start offset for the thread to know where to start
            size_t seg_end = (k == th_number - 1) ? file_stats.st_size : seg_start_offset + seg_size;  //Determining the end of the segment




            if (k > 0) {         //Using if-conditions and a loop to adjust start offset for the segments that are following after the 1st one
                
                while (seg_start_offset < seg_end && (seg_start_offset % (sizeof(int) + 1) != 0)) {
                    
                    seg_start_offset++;
                }
            }




            while (seg_end < file_stats.st_size && (seg_end % (sizeof(int) + 1) != 0)) {    //Using a loop and if condition to adjust the end of the segment as well
                seg_end++;
            }


            
            if (seg_start_offset >= file_stats.st_size || seg_end > file_stats.st_size) {  //Adding error handling for segment boundaries
                message_printer("Out of bounds error in segment. The compressed file may be corrupt");
                exit(1);
            }

            //Establishing thread data for creating the thread
            th_data[k].seg_start = data + seg_start_offset;
            th_data[k].seg_length = seg_end - seg_start_offset;
            th_data[k].th_index = k;

            //printf("Thread %d: seg_start_offset = %zu, seg_end = %zu, seg_length = %zu\n", k, seg_start_offset, seg_end, th_data[k].seg_length);   //troubleshooting print statement, can safely be removed

            pthread_create(&threads[k], NULL, seg_decompression, &th_data[k]);  //Creating thread
        }



        
        for (int j = 0; j < th_number; j++) {  //Using for-loop for joining all the threads


            pthread_join(threads[j], NULL);

            fwrite(th_data[j].output_buffer, sizeof(char), th_data[j].output_length, stdout);  //Writing to a file
            
            free(th_data[j].output_buffer);   //Freeing the output buffer

        }



        munmap(data, file_stats.st_size);  //Freeing mapped memory
 
        close(input_file);  //closing input file
    }





    return 0;

}



