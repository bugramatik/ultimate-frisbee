/*
 * File: Main.c
 */


#include "Pragmas.h"
#include "ADC.h"
#include "LCD.h"
#include "the3.h"
#include <string.h>
#include <stdio.h>

#define LCD_HORIZONTAL 16
#define LCD_VERTICAL 4
#define TIMER0_INITIAL_VALUE 62611

volatile char CONVERT=0;
volatile char gamepad_button_pressed=0;
byte previous_portb = 0;
byte debounce_flag = 0;
int frisbee_held = 0;

unsigned char empty[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
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
        {8,2},
        {3,3},
        {14,2},
        {14,3}
    },
    .frisbee_position = {9,2},
    .game_mode = 1,
    .teamA_score = 0,
    .teamB_score = 0,
    .selected_player = 0,

};

volatile int display_state = 0;

void moveSelectedPlayerPositionOnLCD(int oldX, int oldY, int newX, int newY, int selectedPlayer)
{
    LATA = 0;
    // Clear the old position
    LCDGoto(oldX, oldY);
    LCDStr(" ");  
    
    // Write the player character to the new position
    LCDGoto(newX, newY);
    LCDDat(selectedPlayer < 2 ?  0:2);
}

void setInitialState(){
    
    LCDAddSpecialCharacter(0, selected_teamA_player);
    LCDAddSpecialCharacter(1, teamA_player);
    LCDAddSpecialCharacter(2, selected_teamB_player);
    LCDAddSpecialCharacter(3, teamB_player);
    LCDAddSpecialCharacter(4, frisbee);
    LCDAddSpecialCharacter(5, selected_teamA_player_with_frisbee);
    LCDAddSpecialCharacter(6, selected_teamB_player_with_frisbee);
    LCDAddSpecialCharacter(7,frisbee_target);
    
    LCDGoto(8, 2);
    LCDDat(0);
    
    LCDGoto(3, 3);
    LCDDat(1);

    LCDGoto(14, 2);
    LCDDat(3);

    LCDGoto(14, 3);
    LCDDat(3);

    LCDGoto(9, 2);
    LCDDat(4);
}




void display_7seg() {
    TRISA = 0;
    LATA = 0b00001000; // Enable DISP2 (RA3)
    LATD = digits[gameState.teamA_score];
    __delay_ms(1);
    LATA = 0b00010000; // Enable DISP3 (RA4)
    LATD = dash;
    __delay_ms(1);
    LATA = 0b00100000; // Enable DISP4 (RA5)
    LATD = digits[gameState.teamB_score];
    __delay_ms(1);
}


int isThereOtherPlayer(int oldX, int oldY){
    for (int i = 0; i < 4; i++) {
        if (gameState.player_positions[i].x == oldX && gameState.player_positions[i].y == oldY) {
            return 1; // If a player is found at the given coordinate
        }
    }
    return 0; // If no player is found at the given coordinate
}


int frisbeeHeld(){
    int selected_player = gameState.selected_player;
    if (gameState.frisbee_position.x == gameState.player_positions[selected_player].x
            && gameState.frisbee_position.y == gameState.player_positions[selected_player].y) { // Player hold the frisbee
        gameState.game_mode = 0; // Inctive mode
        frisbee_held = 1;
        return 1;
    }
    
    return 0;
}


int hasPlayerLeftFrisbeeLocation() {
    if (!frisbee_held){
        return 0;
    }
    
    int selected_player = gameState.selected_player;
    
    if (gameState.player_positions[selected_player].x != gameState.frisbee_position.x || 
        gameState.player_positions[selected_player].y != gameState.frisbee_position.y){ // Player released frisbee
        gameState.game_mode = 1; // Active mode
        frisbee_held = 0;
        return 1;
    }
    return 0;
}


void frisbeeHeldOnLCD(){
    int frisbeeX, frisbeeY, selected_player;
    frisbeeX = gameState.frisbee_position.x;
    frisbeeY = gameState.frisbee_position.y;
    selected_player = gameState.selected_player;
    LCDGoto(frisbeeX,frisbeeY);
    LCDStr(" ");
    
    LCDGoto(frisbeeX,frisbeeY);
    LCDDat(selected_player < 2 ? 5:6);
   
}


void frisbeeReleasedOnLCD(){
    int frisbeeX, frisbeeY, selected_player;
    frisbeeX = gameState.frisbee_position.x;
    frisbeeY = gameState.frisbee_position.y;
    selected_player = gameState.selected_player;
    LCDGoto(frisbeeX,frisbeeY);
    LCDStr(" ");
    
    LCDGoto(frisbeeX,frisbeeY);
    LCDDat(4);
}

void gamepadPressed() {
    byte current_portb = PORTB;;
    __delay_ms(1);
    int oldX = gameState.player_positions[gameState.selected_player].x;
    int oldY = gameState.player_positions[gameState.selected_player].y;
    
    //TODO: Buraya bakmak lazim
    
    // Check which button was pressed
    if (((previous_portb & 0b00010000) == 0b00010000) && ((current_portb & 0b00010000) == 0)) // RB4 Up button pressed
    {
        if (gameState.player_positions[gameState.selected_player].y == 1){
            return;
        }
        if (isThereOtherPlayer(oldX,oldY-1)){
            return;
        }
        
        gameState.player_positions[gameState.selected_player].y--;
    } else if (((previous_portb & 0b01000000) == 0b01000000) && ((current_portb & 0b01000000) == 0)) // RB6 Down button pressed
    {
        if (gameState.player_positions[gameState.selected_player].y == LCD_VERTICAL){
            return;
        }
        if (isThereOtherPlayer(oldX,oldY+1)){
            return;
        }
        gameState.player_positions[gameState.selected_player].y++;
    } else if (((previous_portb & 0b10000000) == 0b10000000) && ((current_portb & 0b10000000) == 0)) // RB7 Left button pressed
    {
        if (gameState.player_positions[gameState.selected_player].x == 1) {
            return;
        }
        if (isThereOtherPlayer(oldX-1,oldY)){
            return;
        }
        gameState.player_positions[gameState.selected_player].x--;
    } else if (((previous_portb & 0b00100000) == 0b00100000) && ((current_portb & 0b00100000) == 0)) // RB5 Right button pressed 
    {
        if (gameState.player_positions[gameState.selected_player].x == LCD_HORIZONTAL) {
            return;
        }
        if (isThereOtherPlayer(oldX+1,oldY)){
            return;
        }
        gameState.player_positions[gameState.selected_player].x++;
    }
    else{
        
    }
    previous_portb = current_portb;
    
    // Update the player's position on the LCD
    moveSelectedPlayerPositionOnLCD(oldX, oldY,
            gameState.player_positions[gameState.selected_player].x,
            gameState.player_positions[gameState.selected_player].y,
            gameState.selected_player);
    if (frisbeeHeld()) {
        frisbeeHeldOnLCD();
    }
    if (hasPlayerLeftFrisbeeLocation()) {
        frisbeeReleasedOnLCD();
    }

}

void initRegisters(){
    
    // Initialize TRISB
    TRISB = 0;
    TRISBbits.RB0 = 1;
    TRISBbits.RB1 = 1;
    TRISBbits.RB4 = 1;
    TRISBbits.RB5 = 1;
    TRISBbits.RB6 = 1;
    TRISBbits.RB7 = 1;
    LATB = 0;
    
    INTCONbits.RBIF = 0; // Clear the PORTB Change Interrupt flag
    INTCONbits.RBIE = 1; // Enable the PORTB Change Interrupt
    LATB = 0;

    INTCONbits.INT0IE = 1; //Enable INT0 pin interrupt
    INTCONbits.INT0IF = 0; // Clear INT0 pin interrupt
    LATB = 0;

    INTCON3bits.INT1IE = 1; //Enable INT1 pin interrupt
    INTCON3bits.INT1IF = 0; // Clear INT1 pin interrupt
    
    // Enable external interrupts
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    
    INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt
    
    T0CON = 0b10000111; // Timer0 on, 8-bit, prescaler 1:256
    TMR0 = TIMER0_INITIAL_VALUE;
    
    
    // Initialize Timer1
    T1CON = 0;
    TMR1 = 0;
    T1CONbits.TMR1ON = 1; // Start Timer1

    
    LATB = 0;
    PORTB;;
}

void switchSelectedPlayer(){
    PORTB;;
    LATB = 0;
    LATA = 0;
    LATD = 0;
    
    
    int oldX, oldY,newX,newY;
    int selected_player = gameState.selected_player;
    
    oldX = gameState.player_positions[selected_player].x;
    oldY = gameState.player_positions[selected_player].y;
    
    // Convert previous selected player to unselected player
    LCDGoto(oldX,oldY);
    LCDStr(" ");
    LCDGoto(oldX,oldY);
    LCDDat (selected_player < 2 ? 1:3);

    
    selected_player = (selected_player+1) % 4;
    gameState.selected_player = selected_player;
    
    newX = gameState.player_positions[selected_player].x;
    newY = gameState.player_positions[selected_player].y;
    
    // Convert new selected player
    LCDGoto(newX,newY);
    LCDStr(" ");
    LCDGoto(newX,newY);
    LCDDat (selected_player < 2 ? 0:2);

    LATB = 0;
    
}


void throwFrisbee(){
    frisbee_held = 0;
    int randomX,randomY,selected_player, selected_player_x, selected_player_y;
    randomX = random_generator(16);
    randomY = random_generator(4);
    selected_player = gameState.selected_player;
    selected_player_x = gameState.player_positions[selected_player].x;
    selected_player_y = gameState.player_positions[selected_player].y;
    
    //Convert frisbee holder player to normal selected player
    LCDGoto(selected_player_x,selected_player_y);
    LCDStr(" ");
    LCDDat(selected_player < 2 ? 1:3);
    
    LCDGoto(randomX,randomY); //Display target, TODO: blinking, target should not colliding with any palyer
    LCDDat(7);
    
    
    
    gameState.game_mode = 1; // When frisbee reachs the target game should turn into active mode
}


void __interrupt(high_priority) FNC()
{   
    
    
    if (TMR0IF) { // Check if Timer0 interrupt
        TMR0IF = 0; // Clear the interrupt flag
        TMR0 = TIMER0_INITIAL_VALUE; // Reset the timer count
        
    }

    if(INTCONbits.INT0IF) { // RB0 button was pressed, throw the frisbee
        __delay_ms(50);
        INTCONbits.INT0IF = 0;
        if(frisbee_held){ // If frisbee is not held by selected player don't do anything
            throwFrisbee();
        }
        
    }
    
    if(INTCON3bits.INT1IF) { // RB1 button was pressed, change the player
        __delay_ms(50);
        INTCON3bits.INT1IF = 0;
        if(gameState.game_mode == 0){ //If game is in Inactive mode don't do anything
            return;
        }
        switchSelectedPlayer();
    }
    
    if(INTCONbits.RBIF) { // RB4-RB7 pressed, move selected player
        PORTB;;
        INTCONbits.RBIF = 0;
        gamepadPressed();
    }
    
    if(PIR1bits.TMR1IF) { // For random generator
        PIR1bits.TMR1IF = 0;
    }
    
}

void main(void) { 
    
    initADC();
    
    InitLCD();
    
    initRegisters();
    
    setInitialState();
    
    while(1)
    {
        display_7seg();
    }
}
