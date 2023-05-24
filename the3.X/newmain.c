/*
 * File: Main.c
 */


#include "Pragmas.h"
#include "ADC.h"
#include "LCD.h"
#include <string.h>
#include <stdio.h>

volatile char CONVERT=0;

unsigned char character[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
};

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position player_positions[4];
    Position frisbee_position;
    int selected_player;
    int teamA_score;
    int teamB_score;
    int game_mode; // 0: INACTIVE, 1:ACTIVE
} GameState;

GameState gameState;

void __interrupt(high_priority) FNC()
{
    if(INTCONbits.INT0IF)
    {
        // RB0 button was pressed, throw the frisbee
        CONVERT = 1;
        INTCONbits.INT0IF = 0;
    }
    
    else if(INTCON3bits.INT1IF)
    {
        // RB1 button was pressed, switch the selected player
        gameState.selected_player = (gameState.selected_player + 1) % 4;
        INTCON3bits.INT1IF = 0;
    }
 

}


void main(void) { 
    
    initADC();
    
    InitLCD();
    
    
    LCDStr("Helloo ");
    
    LCDAddSpecialCharacter(0, character);
    LCDGoto(8, 1);
    LCDDat(0);
    
    char values[10] = {0};
    
    unsigned short convertion = 0;
    
    TRISBbits.RB0 = 1;
    INTCONbits.INT0IE = 1; //Enable INT0 pin interrupt
    INTCONbits.INT0IF = 0;  
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    
    
    
    while(1)
    {
        if(CONVERT == 1)
        {
            convertion = readADCChannel(0);
            sprintf(values, "%d", convertion);
            LCDCmd(LCD_CLEAR);
            LCDGoto(5, 2);  
            LCDStr(values);            
            CONVERT = 0;
        }
        
    }
    
    
    
    return;
}
