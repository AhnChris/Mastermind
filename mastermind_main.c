/*	cahn006_LabProjectFinal.c - 6-3-13
 *	Name & E-mail:  - Christopher Ahn cahn006@ucr.edu
 *	CS Login: cahn006
 *	Partner(s) Name & E-mail:  - Ellison Zhu ezhu002@ucr.edu
 *	Lab Section: 021
 *	Assignment: Lab Final Project
 *	Project Description: A game called Mastermind where you try to guess the Hiddencode!
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */


#include <avr/io.h>
#include <ucr/bit.h>
#include <ucr/lcd_8bit_task.h>
#include <ucr/keypad.h>
#include <ucr/scheduler.h>
#include <ucr/timer.h>

//Shift Register function for PORTA
void transmit_data(unsigned char data /*,unsigned char data2*/) {

   int i;

   for (i = 0; i < 8 ; ++i) {

      // Sets SRCLR to 1 allowing data to be set

      // Also clears SRCLK in preparation of sending data
      PORTA = 0x08;

      // set SER = next bit of data to be sent.
      PORTA |= ((data >> i) & 0x01);
      //PORTA |= ((data2 >> i) & 0x01) >> 4; // second shift register

      // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
      PORTA |= 0x02;
   }

   // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
   PORTA |= 0x04;

   // clears all lines in preparation of a new transmission
   PORTA = 0x00;

} // end void transmit_data



//Global Variables for Game Enable SM //////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char en_flag = 0;

//Game Enable SM -- this SM uses A7 as an enable to start the game
enum En_States{wait_swtch, en_on};
int EN_SMTick(int state){ //start of EN_SMTick
	//static unsigned char tmpA = 0x80;
	switch(state){ // start of transitions

		case -1:
		state = wait_swtch;
		break;

		case wait_swtch:
		if(GetBit(PINA,7)){ //Switch is OFF
			state = wait_swtch;
		}
		else if(!GetBit(PINA,7)){ //Switch is ON
			state = en_on;
		}
		break;

		case en_on:
		if(!GetBit(PINA,7)){ //Switch is ON
			state = en_on;
		}
		else if(GetBit(PINA,7)){ //Switch is OFF
			state = wait_swtch;
		}
		break;

		default:
		state = wait_swtch;

	} // end of transitions

	switch(state){ //start of actions
		case -1:
		break;

		case wait_swtch:
		en_flag = 0;
		break;

		case en_on:
		en_flag = 1;
		break;

		default:
		break;
	} //end of actions
	return state;
} //end of EN_SMTick

//Global Variables for SM ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char WinLose_flag = 0;
unsigned char Reset_flag = 0;
unsigned char level_flag = 0;
unsigned char CodeStart_flag = 0;


// LCD SM -- Display what level to choose message and shows what level you chose
enum LT_States { LT_s0, LT_WaitLcdRdy, LT_WaitButton, LT_EasyMsg, LT_MedMsg, LT_HardMsg,
LT_HoldGo1, LT_WaitBtnRelease, LT_LevelMessage, LT_HoldGo2, LT_WaitLcdRdy2,
LT_ClearScreen, LT_HoldGo3, LT_WaitingFlags};

int LT_Tick(int state) { //Start LCD SM
static unsigned char cnt = 0;
static unsigned char timer_cnt = 0;

	switch(state) { // Transitions

		case -1:
			state = LT_s0;
			break;

		case LT_s0:		//Checks en_flag
		if(!(en_flag)){
			state = LT_s0;
		}
		else if(en_flag){
			state = LT_WaitLcdRdy;
			cnt = 0;
			level_flag = 0;
		}
			break;

		case LT_WaitLcdRdy:
			if (!LCD_rdy_g && en_flag) {
				state = LT_WaitLcdRdy;
			}
			else if (LCD_rdy_g && cnt == 0 && en_flag) {
				state = LT_LevelMessage;
			}
			else if(LCD_rdy_g && cnt == 1 && en_flag){
				state = LT_WaitButton;
			}
			else if(!en_flag){
				state = LT_WaitLcdRdy2;
			}
			break;

		case LT_LevelMessage:
		if(en_flag){
			cnt++;
			state = LT_HoldGo1;
		}
		else if(!en_flag){
			state = LT_WaitLcdRdy2;
		}
			break;

		case LT_HoldGo1:
		if(en_flag){
			LCD_go_g = 0;
			state = LT_WaitLcdRdy;
		}
		else if(!en_flag){
			state = LT_WaitLcdRdy2;
		}
			break;

		case LT_WaitButton:
			if (!(GetKeypadKey()) && en_flag) {
				state = LT_WaitButton;
			}
			else if (GetKeypadKey() == '1' && en_flag) { // Button active low
				state = LT_EasyMsg;
				timer_cnt = 0;
				level_flag = 1;
			}
			else if(GetKeypadKey() == '2' && en_flag){
				state = LT_MedMsg;
				timer_cnt = 0;
				level_flag = 2;
			}
			else if(GetKeypadKey() == '3' && en_flag){
				state = LT_HardMsg;
				timer_cnt = 0;
				level_flag = 3;
			}
			else if(!en_flag){
				state = LT_ClearScreen;
			}
			break;

		case LT_EasyMsg:
			if(timer_cnt < 4 && en_flag){
				state = LT_EasyMsg;
				timer_cnt++;
			}
			else if(!(timer_cnt <4) && en_flag)
			{
				state = LT_HoldGo2;
			}
			else if(!en_flag){
				state = LT_WaitLcdRdy2;
				}
			break;

		case LT_MedMsg:
			if(timer_cnt < 4 && en_flag){
				state = LT_MedMsg;
				timer_cnt++;
			}
		else if(!(timer_cnt <4 ) && en_flag){

			state = LT_HoldGo2;
		}
		else if(!en_flag){
			state = LT_WaitLcdRdy2;
		}
			break;

		case LT_HardMsg:
		if(timer_cnt < 4 && en_flag){
			state = LT_HardMsg;
			timer_cnt++;
		}
		else if(!(timer_cnt < 4) && en_flag){
			state = LT_HoldGo2;
		}
		else if(!en_flag){
			state = LT_WaitLcdRdy2;
		}
			break;

		case LT_HoldGo2:
		if(en_flag){
			LCD_go_g = 0;
			state = LT_WaitBtnRelease;
		}
		else if(!en_flag){
			state = LT_WaitLcdRdy2;
		}
			break;

		case LT_WaitBtnRelease:
			if (GetKeypadKey() && en_flag) { // Wait for button release
				state = LT_WaitBtnRelease;
			}
			else if (!(GetKeypadKey()) && en_flag) {
				state = LT_WaitLcdRdy2;
			}
			else if(!en_flag){
				state = LT_WaitLcdRdy2;
			}
			break;

		case LT_WaitLcdRdy2:
		if(!LCD_rdy_g){
			state = LT_WaitLcdRdy2;
		}
		else if(LCD_rdy_g){
			state = LT_ClearScreen;
		}
		break;

		case LT_ClearScreen:
		state = LT_HoldGo3;
		break;

		case LT_HoldGo3:
		LCD_go_g = 0;
		state = LT_WaitingFlags;
		break;

		case LT_WaitingFlags:
		if(en_flag){
			state = LT_WaitingFlags;
			CodeStart_flag = 1;
		}
		else if(!en_flag){
			state = LT_s0;
			CodeStart_flag = 0;
			cnt = 0;
			level_flag = 0;
		}
		break;

		default:
			state = LT_s0;
		} // Transitions

	switch(state) { // State actions
		case -1:
		break;

		case LT_s0:
			//LCD_go_g=0;
			//strcpy(LCD_string_g, "1234567890123456"); // Init, but never seen, shows use of strcpy though
			break;

		case LT_WaitLcdRdy:
			break;

		case LT_LevelMessage:
			strcpy(LCD_string_g, "Choose Level 1-3");
			LCD_go_g = 1; // Display String;
			break;

		case LT_WaitButton:
			break;

		case LT_EasyMsg:
			strcpy(LCD_string_g, "Level 1 Selected");
			LCD_go_g = 1; // Display string
			break;

		case LT_MedMsg:
			strcpy(LCD_string_g, "Level 2 Selected");
			LCD_go_g = 1; //Display String
			break;

		case LT_HardMsg:
			strcpy(LCD_string_g, "Level 3 Selected");
			LCD_go_g = 1; //Display String
			break;

		case LT_HoldGo1:
			break;

		case LT_HoldGo2:
			break;

		case LT_WaitBtnRelease:
			break;

		case LT_WaitLcdRdy2:
		break;

		case LT_ClearScreen:
		strcpy(LCD_string_g, "                ");
		LCD_go_g = 1; //Display String
		break;

		case LT_HoldGo3:
		break;

		case LT_WaitingFlags:
		break;

		default:
			break;

	} // State actions
	return state;
} //End LCD SM

//Global Variable for Code Generator SM ////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char GameRdy_flag = 0;
char HiddenCode[4];// = {'4', '3', '2', '1'}; //HARD CODED VALUE CHANGE LATER

// Code Generator SM
enum CD_States{CD_WaitFlag, CD_EasyCode, CD_MedCode, CD_HardCode, CD_SetGame,
	CD_WaitLcdRdy, CD_HoldGo1};

int CD_Tick(int state){ //Start Code Generator SM
	static unsigned char w = 0;
	static unsigned char x = 0;
	static unsigned char y = 0;
	static unsigned char z = 0;
	switch(state){ //Start Transitions
		case -1:
		state = CD_WaitFlag;
		break;

		case CD_WaitFlag:
		if(!CodeStart_flag){
			state = CD_WaitFlag;
		}
		else if(CodeStart_flag && level_flag == 1){
			w = rand() %10; //random # between 0-9
			y = rand() %10;
			x = rand() %10;
			z = rand() %10;
			state = CD_EasyCode;
		}
		else if(CodeStart_flag && level_flag == 2){
			state = CD_MedCode;
		}
		else if(CodeStart_flag && level_flag == 3){
			state = CD_HardCode;
		}
		break;

		case CD_EasyCode:
		state = CD_SetGame;
		break;

		case CD_MedCode:
		state = CD_SetGame;
		break;

		case CD_HardCode:
		state = CD_SetGame;
		break;

		case CD_SetGame:
		if(level_flag == 1 && en_flag){
			state = CD_SetGame;
		}
		else if(!en_flag){
			state = CD_WaitFlag;
			GameRdy_flag = 0;
		}
		break;

		default:
		state = CD_WaitFlag;
	} //End Transitions

	switch(state){ //Start Actions

		case -1:
		break;

		case CD_WaitFlag:
		GameRdy_flag = 0;
		break;

		case CD_EasyCode:

		if(w == 0){      //w conditions
			HiddenCode[0] = '0';
		}
		else if(w == 1){
			HiddenCode[0] = '1';
		}
		else if(w == 2){
			HiddenCode[0] = '2';
		}
		else if(w == 3){
			HiddenCode[0] = '3';
		}
		else if(w == 4){
			HiddenCode[0] = '4';
		}
		else if(w == 5){
			HiddenCode[0] = '5';
		}
		else if(w == 6){
			HiddenCode[0] = '6';
		}
		else if(w == 7){
			HiddenCode[0] = '7';
		}
		else if(w == 8){
			HiddenCode[0] = '8';
		}
		else if(w == 9){
			HiddenCode[0] = '9';
		}

		if(x == 0){ // x conditions
			HiddenCode[1] = '0';
		}
		else if(x == 1){
			HiddenCode[1] = '1';
		}
		else if(x == 2){
			HiddenCode[1] = '2';
		}
		else if(x == 3){
			HiddenCode[1] = '3';
		}
		else if(x == 4){
			HiddenCode[1] = '4';
		}
		else if(x == 5){
			HiddenCode[1] = '5';
		}
		else if(x == 6){
			HiddenCode[1] = '6';
		}
		else if(x == 7){
			HiddenCode[1] = '7';
		}
		else if(x == 8){
			HiddenCode[1] = '8';
		}
		else if(x == 9){
			HiddenCode[1] = '9';
		}

		if(y == 0){ //y conditions
			HiddenCode[2] = '0';
		}
		else if(y == 1){
			HiddenCode[2] = '1';
		}
		else if(y == 2){
			HiddenCode[2] = '2';
		}
		else if(y == 3){
			HiddenCode[2] = '3';
		}
		else if(y == 4){
			HiddenCode[2] = '4';
		}
		else if(y == 5){
			HiddenCode[2] = '5';
		}
		else if(y == 6){
			HiddenCode[2] = '6';
		}
		else if(y == 7){
			HiddenCode[2] = '7';
		}
		else if(y == 8){
			HiddenCode[2] = '8';
		}
		else if(y == 9){
			HiddenCode[2] = '9';
		}

		if(z == 0){ //z conditions
		HiddenCode[3] = '0';
		}
		else if(z == 1){
			HiddenCode[3] = '1';
		}
		else if(z == 2){
			HiddenCode[3] = '2';
		}
		else if(z == 3){
			HiddenCode[3] = '3';
		}
		else if(z == 4){
			HiddenCode[3] = '4';
		}
		else if(z == 5){
			HiddenCode[3] = '5';
		}
		else if(z == 6){
			HiddenCode[3] = '6';
		}
		else if(z == 7){
			HiddenCode[3] = '7';
		}
		else if(z == 8){
			HiddenCode[3] = '8';
		}
		else if(z == 9){
			HiddenCode[3] = '9';
		}
		break;

		case CD_MedCode:
		break;

		case CD_HardCode:
		break;

		case CD_SetGame:
		GameRdy_flag = 1;
		break;

		default:
		break;

	} //End Actions
	return state;
} //End Code Generator SM
/*
//TESTER SM TO VIEW GENERATED CODE ////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum HD_States{HD_s0, HD_WaitLcdRdy, HD_Message, HD_HoldGo1};
	int HD_Tick(int state){
		switch(state){
			case -1:
			state = HD_s0;
			break;

			case HD_s0:
			state = HD_WaitLcdRdy;
			break;

			case HD_WaitLcdRdy:
			if(!LCD_rdy_g && GameRdy_flag){
				state = HD_WaitLcdRdy;
				}
			else if(LCD_rdy_g && GameRdy_flag){
				state = HD_Message;
				}
				break;

				case HD_Message:
				state = HD_HoldGo1;
				break;

				case HD_HoldGo1:
				LCD_go_g = 0;
				state = HD_WaitLcdRdy;
				break;

				default:
				state = HD_s0;

		} // end transitions

		switch(state){
			case -1:
			break;

			case HD_s0:
			break;

			case HD_WaitLcdRdy:
			break;

			case HD_Message:
			LCD_string_g[0] = HiddenCode[0];
			LCD_string_g[1] = HiddenCode[1];
			LCD_string_g[2] = HiddenCode[2];
			LCD_string_g[3] = HiddenCode[3];
			LCD_go_g = 1; //display
			break;

			case HD_HoldGo1:
			break;

			default:
			break;

		}//end actions
		return state;
	} // end SM
*/
/*
//TEST SM FOR GETTING INPUT
enum HD_States{HD_Wait, HD_Input, HD_Store, HD_Compare, HD_Result, HD_WaitLcdRdy1,
	HD_WaitLcdRdy2, HD_Msg1, HD_Msg2, HD_HoldGo1, HD_End};
int HD_Tick(int state){
static unsigned char correct_flag = 0;
static unsigned char x;

	switch(state){
		case -1:
		state = HD_Wait;
		break;

		case HD_Wait:
		if(!GameRdy_flag){
			state = HD_Wait;
		}
		else if(GameRdy_flag){
			state = HD_Input;
		}
		break;

		case HD_Input:
		if(!GetKeypadKey()){
			state = HD_Input;
		}
		else if(GetKeypadKey()){
			state = HD_Store;
		}
		break;

		case HD_Store:
		if(GetKeypadKey()){
			state = HD_Store;
		}
		else if(!GetKeypadKey()){
			state = HD_Compare;
		}
		break;

		case HD_Compare:
		state = HD_Result;
		break;

		case HD_Result:
		if(correct_flag){
			state = HD_WaitLcdRdy1;
		}
		else if(!correct_flag){
			state = HD_WaitLcdRdy2;
		}
		break;

		case HD_WaitLcdRdy1:
		if(!LCD_rdy_g){
			state = HD_WaitLcdRdy1;
		}
		else if(LCD_rdy_g){
			state = HD_Msg1;
		}
		break;

		case HD_Msg1:
		state = HD_HoldGo1;
		break;

		case HD_WaitLcdRdy2:
		if(!LCD_rdy_g){
			state = HD_WaitLcdRdy2;
		}
		else if(LCD_rdy_g){
			state = HD_Msg2;
		}
		break;

		case HD_Msg2:
		state = HD_HoldGo1;
		break;

		case HD_HoldGo1:
		LCD_go_g = 0;
		state = HD_End;
		break;

		case HD_End:
		if(GameRdy_flag){
			state = HD_End;
		}
		else if(!GameRdy_flag){
			state = HD_Wait;
		}
		break;

		default:
		state = HD_Wait;
	} //end transtions
	switch(state){
		case -1:
		break;

		case HD_Wait:
		break;

		case HD_Input:
		break;

		case HD_Store:

		x = GetKeypadKey();
		char PlayerCode[0];
		PlayerCode[0] = x;
		break;

		case HD_Compare:
		if(PlayerCode[0] == HiddenCode[0]){
			correct_flag = 1;
		}
		else{
			correct_flag = 0;
		}

		case HD_Result:
		break;

		case HD_WaitLcdRdy1:
		break;

		case HD_WaitLcdRdy2:
		break;

		case HD_Msg1:
		LCD_string_g[0] = PlayerCode[0];
		LCD_string_g[1] = HiddenCode[0];
		LCD_string_g[2] = PlayerCode[0];
		LCD_string_g[3] = PlayerCode[0];
		LCD_string_g[4] = PlayerCode[0];
		//strcpy(LCD_string_g,"Correct         ");
		LCD_go_g = 1; //display string
		break;

		case HD_Msg2:
		LCD_string_g[0] = PlayerCode[0];
		LCD_string_g[1] = HiddenCode[0];
		LCD_string_g[2] = PlayerCode[0];
		LCD_string_g[3] = PlayerCode[0];
		LCD_string_g[4] = PlayerCode[0];
		//strcpy(LCD_string_g,"Wrong           ");
		LCD_go_g = 1; //Display String
		break;

		case HD_HoldGo1:
		break;

		case HD_End:
		break;

		default:
		break;

	}//end actions
	return state;
}*/

//Game SM -- gets input from player and compares to hidden code. Will show if correct or not. //////////////////////////////////////////////////
enum GA_States{GA_WaitStart, GA_LvlChecker, GA_EasyMode, GA_WaitLcdRdy1, GA_InputMsg,
	GA_HoldGo1, GA_KeyInput, GA_Store, GA_Compare, GA_Result, GA_Correct, GA_Wrong,
	GA_WaitLcdRdy2, GA_WaitLcdRdy3, GA_WinMsg, GA_LoseMsg, GA_HoldGo2, GA_WaitReset,
	GA_LCDWAIT, GA_HoldGo3, GA_TryAgainMsg, GA_WaitLcdRdy4, GA_WaitLcdRdy5};

int GA_Tick(int state){ //Start Game SM
static unsigned char keycnt = 0;
static unsigned char i = 0;
static unsigned char lifecnt;
static unsigned char correct_flag = 0;
static unsigned char x;
static unsigned char cntblue = 1;
static unsigned char cntwhite = 1;
static char PlayerCode[4];
	switch(state){ //start transitions
		case -1:
		state = GA_WaitStart;
		break;

		case GA_WaitStart:
		if(!GameRdy_flag){
			state = GA_WaitStart;
		}
		else if(GameRdy_flag){
			state = GA_LvlChecker;
		}
		break;

		case GA_LvlChecker:
		if(GameRdy_flag && level_flag == 1){
			state = GA_EasyMode;
		}
		break;

		case GA_EasyMode:
		state = GA_WaitLcdRdy1;
      lifecnt = 2;
		break;

		case GA_WaitLcdRdy1:
		if(!LCD_rdy_g){
			state = GA_WaitLcdRdy1;
		}
		else if(LCD_rdy_g){
			state = GA_InputMsg;
		}
		break;

		case GA_InputMsg:
		state = GA_HoldGo1;
		break;

		case GA_HoldGo1:
		LCD_go_g = 0;
		keycnt = 0;
		//lifecnt = 1;
		i = 0;
		state = GA_LCDWAIT;
		break;

		case GA_LCDWAIT:
		if(!LCD_rdy_g){
			state = GA_LCDWAIT;
		}
		else if(LCD_rdy_g){
			state = GA_KeyInput;
		}
		break;

		case GA_KeyInput:
		if(!GetKeypadKey() && keycnt < 4){
			state = GA_KeyInput;
		}
		else if(GetKeypadKey()){
			state = GA_Store;
		}
		else if(!GetKeypadKey() && !(keycnt < 4)){
			state = GA_Compare;
         cntblue = 0;
         cntwhite = 0;
		}
		break;

		case GA_Store:
		if(GetKeypadKey()){
			state = GA_Store;
		}
		else if(!GetKeypadKey()){
			state = GA_KeyInput;
			i++;
			keycnt++;
		}
		break;

		case GA_Compare:
      //Qa-Qd = blue -- Qe-Qh = white
      if(cntblue == 4){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         transmit_data(0x20); //blue
         transmit_data(0x10); //blue
         correct_flag = 1;
      }
      else if(cntblue == 3 && cntwhite == 1){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         transmit_data(0x20); //blue
         transmit_data(0x08); //white
         correct_flag = 0;
      }
      else if(cntblue == 2 && cntwhite == 2){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         correct_flag = 0;
      }
      else if(cntblue == 1 && cntwhite == 3){
         transmit_data(0x80); //blue
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         transmit_data(0x02); //white
         correct_flag = 0;
      }
      else if(cntwhite == 4){
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         transmit_data(0x02); //white
         transmit_data(0x01); //white
         correct_flag = 0;
      }
      else if(cntwhite == 3){
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         transmit_data(0x02); //white
         correct_flag = 0;
      }
      else if(cntwhite == 2){
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         correct_flag = 0;
      }
      else if(cntwhite == 1){
         transmit_data(0x08); //white
         correct_flag = 0;
      }
      else if(cntwhite == 3 && cntblue == 0){
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         transmit_data(0x02); //white
         correct_flag = 0;
      }
      else if(cntwhite == 2 && cntblue == 0){
         transmit_data(0x08); //white
         transmit_data(0x04); //white
         correct_flag = 0;
      }
      else if(cntwhite == 1 && cntblue == 0){
         transmit_data(0x08); //white
         correct_flag = 0;
      }
      else if(cntblue == 0 && cntwhite == 0){
         correct_flag = 0;
      }
      else if(cntblue == 3 && cntwhite == 0){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         transmit_data(0x20); //blue
         correct_flag = 0;
      }
      else if(cntblue == 2 && cntwhite == 0){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         correct_flag = 0;
      }
      else if(cntblue == 1 && cntwhite == 0){
         transmit_data(0x80); //blue
         correct_flag = 0;
      }
      else if(cntblue == 3){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         transmit_data(0x20); //blue
         correct_flag = 0;
      }
      else if(cntblue == 2){
         transmit_data(0x80); //blue
         transmit_data(0x40); //blue
         correct_flag = 0;
      }
      else if(cntblue == 1){
         transmit_data(0x80); //blue
         correct_flag = 0;
      }
		state = GA_Result;
		break;

		case GA_Result:
		if(correct_flag){
			state = GA_Correct;
		}
		else if(!(correct_flag)){
			state = GA_Wrong;
			lifecnt--;
		}
		break;

		case GA_Wrong:
		if(!(lifecnt == 0)){
			state = GA_WaitLcdRdy4;
		}
		else if(lifecnt == 0){
			state = GA_WaitLcdRdy3;
		}
		break;

      case GA_WaitLcdRdy4:
      if(!LCD_rdy_g){
         state = GA_WaitLcdRdy4;
      }
      else if(LCD_rdy_g){
         state = GA_TryAgainMsg;
      }
      break;

      case GA_TryAgainMsg:
      state = GA_HoldGo3;
      break;

      case GA_HoldGo3:
      LCD_go_g = 0;
      state = GA_WaitLcdRdy5;
      break;

      case GA_WaitLcdRdy5:
      if(!LCD_rdy_g){
         state = GA_WaitLcdRdy5;
      }
      else if(LCD_rdy_g){
         state = GA_InputMsg;
         cntblue = 0;
         cntwhite = 0;
      }
      break;

		case GA_WaitLcdRdy3:
		if(!LCD_rdy_g){
			state = GA_WaitLcdRdy3;
		}
		else if(LCD_rdy_g){
			state = GA_LoseMsg;
		}
		break;

		case GA_LoseMsg:
		state = GA_HoldGo2;
		break;

		case GA_Correct:
		state = GA_WaitLcdRdy2;
		break;

		case GA_WaitLcdRdy2:
		if(!LCD_rdy_g){
			state = GA_WaitLcdRdy2;
		}
		else if(LCD_rdy_g){
			state = GA_WinMsg;
		}
		break;

		case GA_WinMsg:
		state = GA_HoldGo2;
		break;

		case GA_HoldGo2:
		LCD_go_g = 0;
		state = GA_WaitReset;
		break;

		case GA_WaitReset:
		if(GameRdy_flag){
			state = GA_WaitReset;
		}
		else if(!GameRdy_flag){
			state = GA_WaitStart;
         correct_flag = 0;
		}
		break;

		default:
		state = GA_WaitStart;
	} //end transitions

	switch(state){ //start actions
		case -1:
		break;

		case GA_WaitStart:
		break;

		case GA_LvlChecker:
		break;

		case GA_EasyMode:
		break;

		case GA_WaitLcdRdy1:
		break;

		case GA_InputMsg:
		strcpy(LCD_string_g, "Input 4 Buttons!");
		/*LCD_string_g[0] = PlayerCode[0];
		LCD_string_g[1] = PlayerCode[1];
		LCD_string_g[2] = PlayerCode[2];
		LCD_string_g[3] = PlayerCode[3];
		LCD_string_g[4] = HiddenCode[0];
		LCD_string_g[5] = HiddenCode[1];
		LCD_string_g[6] = HiddenCode[2];
		LCD_string_g[7] = HiddenCode[3];*/
		LCD_go_g = 1; //Display String
		break;

		case GA_HoldGo1:
		break;

		case GA_KeyInput:
		break;

		case GA_Store:
		x = GetKeypadKey();
		PlayerCode[i] = x;
		break;

		case GA_Compare:
      if(PlayerCode[0] == HiddenCode[0]){
         cntblue++;
      }
      if(PlayerCode[1] == HiddenCode[1]){
         cntblue++;
      }
      if(PlayerCode[2] == HiddenCode[2]){
         cntblue++;
      }
      if(PlayerCode[3] == HiddenCode[3]){
         cntblue++;
      }
      if(PlayerCode[0] == HiddenCode[1] || PlayerCode[0] == HiddenCode[2]
      || PlayerCode[0] == HiddenCode[3]){
         cntwhite++;
      }
      if(PlayerCode[1] == HiddenCode[0] || PlayerCode[1] == HiddenCode[2]
      || PlayerCode[1] == HiddenCode[3]){
         cntwhite++;
      }
      if(PlayerCode[2] == HiddenCode[0] || PlayerCode[2] == HiddenCode[1]
      || PlayerCode[2] == HiddenCode[3]){
         cntwhite++;
      }
      if(PlayerCode[3] == HiddenCode[0] || PlayerCode[3] == HiddenCode[1]
      || PlayerCode[3] == HiddenCode[2]){
         cntwhite++;
      }
		break;

		case GA_Result:
		break;

		case GA_Correct:
		break;

		case GA_Wrong:
		break;

      case GA_WaitLcdRdy4:
      break;

      case GA_TryAgainMsg:
      strcpy(LCD_string_g, "Try Again       ");
      LCD_go_g = 1; //Display String
      break;

      case GA_HoldGo3:
      break;

      case GA_WaitLcdRdy5:
      break;

		case GA_WaitLcdRdy3:
		break;

		case GA_WaitLcdRdy2:
		break;

		case GA_WinMsg:
		strcpy(LCD_string_g, "You Win!!       ");
		LCD_go_g = 1; //Display String
		break;

		case GA_LoseMsg:
		strcpy(LCD_string_g,"You LoseCode");
		LCD_string_g[12] = HiddenCode[0];
		LCD_string_g[13] = HiddenCode[1];
		LCD_string_g[14] = HiddenCode[2];
		LCD_string_g[15] = HiddenCode[3];
		LCD_go_g = 1; //Display String
		break;

		case GA_HoldGo2:
		break;

		case GA_WaitReset:
		break;

		case GA_LCDWAIT:
		break;

		default:
		break;
	} //end actions
	return state;

} //End Game SM


int main(void)
{
	DDRA = 0x7F; PORTA = 0x80; //A0-A6 are outputs, A7 is input
	DDRD = 0xFF; PORTD = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;

   // Period for the tasks
   unsigned long int SMTick1_calc = 10;
   unsigned long int SMTick2_calc = 50;
   unsigned long int SMTick3_calc = 50;
   unsigned long int SMTick4_calc = 50;
   unsigned long int SMTick5_calc = 50;

   //Calculating GCD
   unsigned long int tmpGCD = 1;
   tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
   tmpGCD = findGCD(tmpGCD, SMTick3_calc);
   tmpGCD = findGCD(tmpGCD, SMTick4_calc);
   tmpGCD = findGCD(tmpGCD, SMTick5_calc);

   //Greatest common divisor for all tasks or smallest time unit for tasks.
   unsigned long int GCD = tmpGCD;

   //Recalculate GCD periods for scheduler
   unsigned long int SMTick1_period = SMTick1_calc/GCD;
   unsigned long int SMTick2_period = SMTick2_calc/GCD;
   unsigned long int SMTick3_period = SMTick3_calc/GCD;
   unsigned long int SMTick4_period = SMTick4_calc/GCD;
   unsigned long int SMTick5_period = SMTick5_calc/GCD;

   //Declare an array of tasks
   static task task1, task2, task3, task4, task5;
   task *tasks[] = { &task1, &task2, &task3, &task4, &task5};
   const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
   // Task 1
   task1.state = -1;//Task initial state.
   task1.period = SMTick1_period;//Task Period.
   task1.elapsedTime = SMTick1_period;//Task current elapsed time.
   task1.TickFct = &EN_SMTick;//Function pointer for the tick.

   // Task 2
   task2.state = -1;//Task initial state.
   task2.period = SMTick2_period;//Task Period.
   task2.elapsedTime = SMTick2_period;//Task current elapsed time.
   task2.TickFct = &LCDI_SMTick;//Function pointer for the tick.

   // Task 3
   task3.state = -1;//Task initial state.
   task3.period = SMTick3_period;//Task Period.
   task3.elapsedTime = SMTick3_period;//Task current elapsed time.
   task3.TickFct = &LT_Tick;//Function pointer for the tick.

   // Task 4
   task4.state = -1;//Task initial state.
   task4.period = SMTick4_period;//Task Period.
   task4.elapsedTime = SMTick4_period;//Task current elapsed time.
   task4.TickFct = &CD_Tick;//Function pointer for the tick.

   // Task 5
   task5.state = -1;//Task initial state.
   task5.period = SMTick5_period;//Task Period.
   task5.elapsedTime = SMTick5_period;//Task current elapsed time.
   task5.TickFct = &GA_Tick;//Function pointer for the tick.

   // Set the timer and turn it on
   TimerSet(GCD);
   TimerOn();

   unsigned short i; // Scheduler for-loop iterator
   while(1) {
      // Scheduler code
      for ( i = 0; i < numTasks; i++ ) {
         // Task is ready to tick
         if ( tasks[i]->elapsedTime == tasks[i]->period ) {
            // Setting next state for task
            tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
            // Reset the elapsed time for next tick.
            tasks[i]->elapsedTime = 0;
         }
         tasks[i]->elapsedTime += 1;
      }
      while(!TimerFlag);
      TimerFlag = 0;
   }

   // Error: Program should not exit!
   return 0;
}