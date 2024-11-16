#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>          //importing libraries
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>


pthread_mutex_t lock;  //mutex lock mechanism, for handling multiple thread race conditions, for example when few threads are trying to access same code





typedef struct {          //initializing certain data structure for each thread, with start of data segment, its length, and thread index
    char *seg_start;
    size_t seg_length;

    size_t th_index; 
} Th_data;




void message_printer(const char *msg) {      //Function for printing errors and messages
    fprintf(stderr, "%s\n", msg);
}




void *seg_decompression(void *th_arg) {    //Establishing function for segment decompression

    Th_data *seg_data = (Th_data *)th_arg;   //establishing thread data instance
    char *seg_start = seg_data->seg_start;        //initializing segment starting point
    size_t seg_length = seg_data->seg_length;  //initializing length of the segment

    //creating allocation for decompressed segment data

    char *decomp_output = malloc(seg_length * 10); // Allocating memory for the output, multiplying the length of the segment so that there is always enough memory

    if (!decomp_output) {                 //Adding error handling in case allocation fails

        message_printer("malloc failed");

        pthread_exit(NULL);
    }

    size_t decomp_buff_position = 0; //initializing variable for positioning in the buffer

    for (size_t i = 0; i < seg_length;) {   //Using for-loop to go over each encoded character inside the segment
        
        int c_count;   //Initializing count value for RLE encoding. 
        char enc_character;  //Initializing character value for RLE encoding. 


        if (sscanf(seg_start + i, "%d%c", &c_count, &enc_character) == 2) {  //checking if the encoding format is correct

            for (int j = 0; j < c_count; j++) {        //Using for-loop to print the decoded output

                decomp_output[decomp_buff_position++] = enc_character; 
            }

            while (i < seg_length && (seg_start[i] >= '0' && seg_start[i] <= '9')) i++; //skipping the count character
            i++; // Skipping 1 more character

        } else {   //error handling for format errors. Throws error if program detects incorrect encoding format
            

            message_printer("Wrong encoding format");  //throwing the error

            free(decomp_output);  //Freeing the memory

            pthread_exit(NULL);  //exiting the thread
        }
    }

    

    pthread_mutex_lock(&lock);  //using mutex lock to block parallel usage

    fwrite(decomp_output, sizeof(char), decomp_buff_position, stdout);  //writing to the output file


    pthread_mutex_unlock(&lock); //unlocking the mutex lock


    free(decomp_output);  //Freeing the allocated memory

    pthread_exit(NULL);  //exiting thread
}




















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

    
    pthread_mutex_init(&lock, NULL);   //Establishing mutex lock









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





        for (int k = 0; k < th_number; k++) {   //Using for-loop to create each thread 1 by 1, for file compression

            //Difining each element of the thread's data structure, such as start of segment, segment length and thread index


            th_data[k].seg_start = data + k * seg_size;   //Difining start of segment. The calculation is an offset that the thread has to make to get to its segment

            th_data[k].seg_length = (k == th_number - 1) ? (file_stats.st_size - k * seg_size) : seg_size; //Difining length of segment, includes last thread condition

            th_data[k].th_index = k;  //setting index of thread as k number of the for-loop (simple counting)


            pthread_create(&threads[k], NULL, seg_decompression, &th_data[k]);  //creating thread, function for compressing the segment specified


        }







        
        for (int j = 0; j < th_number; j++) {  //Using for-loop for joining all the threads


            pthread_join(threads[j], NULL);

        }











        munmap(data, file_stats.st_size);  //Freeing mapped memory
 
        close(input_file);  //closing input file
    }










    pthread_mutex_destroy(&lock); //destroying mutex lock before exiting

    return 0;

}



