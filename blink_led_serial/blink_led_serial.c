#include <stdio.h>
#include "pico/stdlib.h"

int main(){
    //Initialise I/O
    stdio_init_all(); 

    // Initialise GPIO (Green LED connected to pin 25)
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    char userInput;

    //Main Loop 
    while(1){
        //Get User Input
        printf("Command (1 = on or 0 = off):\n");
        userInput = getchar();

        if(userInput == '1'){
            // Turn On LED
            gpio_put(25, 1); // Set pin 25 to high
            printf("LED switched on!\n");
        }
        else if(userInput == '0'){
            // Turn Off LED
            gpio_put(25, 0); // Set pin 25 to high.
            printf("LED switched off!\n");
        }
        else{
            printf("Invalid Input!\n");
        }
    }
}
