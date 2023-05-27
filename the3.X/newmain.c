/*
 * File: Main.c
 */


#include "Pragmas.h"
#include "ADC.h"
#include "LCD.h"
#include "the3.h"
#include <string.h>
#include <stdio.h>

volatile char CONVERT=0;

unsigned char empty[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
unsigned char digits[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

// Define the pattern for the dash character
unsigned char dash = 0b01000000;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position player_positions[4] ;
    Position frisbee_position ;
    int selected_player;
    int teamA_score;
    int teamB_score;
    int game_mode; // 0: INACTIVE, 1:ACTIVE
} GameState;

GameState gameState = {
    .player_positions = {
        {4,2},
        {3,3},
        {14,2},
        {14,3}
    },
    .frisbee_position = {9,2},
    .game_mode = 0,
    .teamA_score = 0,
    .teamB_score = 0,
    .selected_player = 0,

};

volatile int display_state = 0;

void updatePlayerPositionOnLCD(int oldX, int oldY, int newX, int newY, int selectedPlayer)
{
    printf("Updating player position from (%d, %d) to (%d, %d)\n", oldX, oldY, newX, newY);
    // Clear the old position
    LCDAddSpecialCharacter(selectedPlayer, empty);
    LCDGoto(oldX, oldY);
    LCDDat(selectedPlayer);  // Assuming ' ' is the character for an empty cell
    
    // Write the player character to the new position
    LCDAddSpecialCharacter(selectedPlayer, selectedPlayer < 2 ? selected_teamA_player : selected_teamB_player);
    LCDGoto(newX, newY);
    LCDDat(selectedPlayer);
}

void __interrupt(high_priority) FNC()
{
    
    if(INTCONbits.RBIF)
    {
        // Save the old position
        int oldX = gameState.player_positions[gameState.selected_player].x;
        int oldY = gameState.player_positions[gameState.selected_player].y;
        gameState.teamA_score = (gameState.teamA_score+1)%6;

       // Check which button was pressed
        if(PORTBbits.RB4) // Up button pressed
        {
            gameState.player_positions[gameState.selected_player].y--;
        }
        else if(PORTBbits.RB6) // Down button pressed
        {
            gameState.player_positions[gameState.selected_player].y++;
        }
        else if(PORTBbits.RB7) // Left button pressed
        {
            gameState.player_positions[gameState.selected_player].x--;
        }
        else if(PORTBbits.RB5) // Right button pressed
        {
            gameState.player_positions[gameState.selected_player].x++;
        }
        // Update the player's position on the LCD
        updatePlayerPositionOnLCD(oldX, oldY, 
                                  gameState.player_positions[gameState.selected_player].x, 
                                  gameState.player_positions[gameState.selected_player].y, 
                                  gameState.selected_player);  
//        updatePlayerPositionOnLCD(oldX, oldY, 
//                                 0,0,
//                                  gameState.selected_player);  

        // Clear the PORTB on-change interrupt flag
        INTCONbits.RBIF = 0;
    }
    if (TMR0IF) { // Check if Timer0 interrupt
        TMR0IF = 0; // Clear the interrupt flag
        TMR0 = 100; // Reset the timer count

        switch (display_state) {
            case 0:
                TRISA = 0b00000000; 
                PORTD = digits[gameState.teamA_score];
                PORTA = 0b00001000; // Enable DISP2 (RA3)
                break;
            case 1:
                TRISA = 0b00000000; 
                PORTD = dash;
                LATA = 0b00010000; // Enable DISP3 (RA4)
                break;
            case 2:
                TRISA = 0b00000000; 
                PORTD = digits[gameState.teamB_score];
                LATA = 0b00100000; // Enable DISP4 (RA5)
                break;
        }

        display_state = (display_state + 1) % 3;
    }
     
    if(INTCONbits.INT0IF)
    {
        // RB0 button was pressed, throw the frisbee
        CONVERT = 1;
        INTCONbits.INT0IF = 0;
    }
    
    

}




void main(void) { 
    
    initADC();
    
    InitLCD();
    TRISB = 0;
//    gameState.selected_player = 0;
//    gameState.player_positions = 
//                                {
//        Position {3,2},
//        Position {3,3},
//        Position {14,2},
//        Position {14,3},
//        };
//    Position pos = {9,2};
//    gameState.frisbee_position = pos;
//    gameState.teamA_score = 0;
//    gameState.teamB_score = 0;

    
    LCDAddSpecialCharacter(0, selected_teamA_player);
    LCDAddSpecialCharacter(1, teamA_player);
    LCDAddSpecialCharacter(2, teamB_player);
    LCDAddSpecialCharacter(3, teamB_player);
    LCDAddSpecialCharacter(4, frisbee);
    LCDGoto(3, 2);
    LCDDat(0);
    LCDGoto(3, 3);
    LCDDat(1);

    LCDGoto(14, 2);
    LCDDat(2);

    LCDGoto(14, 3);
    LCDDat(3);

    LCDGoto(9, 2);
    LCDDat(4);

    
    
    char values[10] = {0};
    
    unsigned short convertion = 0;
    TRISBbits.RB0 = 1;
    TRISBbits.RB4 = 1;
    TRISBbits.RB5 = 1;
    TRISBbits.RB6 = 1;
    TRISBbits.RB7 = 1;
    
    // Enable the PORTB Change Interrupt
    INTCONbits.RBIE = 1;

    // Clear the PORTB Change Interrupt flag
    INTCONbits.RBIF = 0;
    




    INTCONbits.INT0IE = 1; //Enable INT0 pin interrupt
    INTCONbits.INT0IF = 0;  
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt

    T0CON = 0b11000111; // Timer0 on, 16-bit, prescaler 1:256
    while(1)
    {
        if(CONVERT == 1)
        {
            convertion = readADCChannel(0);
//            sprintf(values, "%d", convertion);
//            LCDCmd(LCD_CLEAR);
//            LCDGoto(5, 2);  
//            LCDStr(values);            
            CONVERT = 0;
        }
        
    }
    
    
    
    return;
}
