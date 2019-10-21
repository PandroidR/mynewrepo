//------------------------------------------------------------------------------
// 66-16AI64SSA Sample Code
//------------------------------------------------------------------------------
// Revision History:
//------------------------------------------------------------------------------
// Revision  Date        Name      Comments
// 1.0       02/07/03    Gary      Created
// 1.1       03/07/08    Gary      Updated for VC200x and timer
// 2.1		 08/19/11    Gary	   Updated for Win 7 64 bit
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define _WIN32_WINDOWS 0x0500 // Not supported by 95

#include <conio.h>       //for cprintf(), kbhit(),
#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <wtypes.h>
#include <math.h>
#include "tools.h"
#include "66-AI64SSAintface.h"
//
#pragma warning(disable : 4996)

void analog_inputs(void);
void SelfTest(void);
void DmaTest(void);
void ContinuousSaveData(void);
void MultiBoardExample(void);
void MultiBoard_DMA_Example(void);
void Data_Demo(void);
void Ext_Burst_Demo(void);

DWORD WINAPI
InterruptAttachThreadDma      //RATNESH: this function is called in ContinuousSaveData() and its functionality is explained just after ContinuousSaveData()
(
    LPVOID pParam
    );
DWORD WINAPI
DiskWriteFunction            //RATNESH: this thread is called in MultiBoardExample() and its functionality is explained just after MultiBoardExample()
(
    LPVOID pParam
    );


// Subs for the AI64SS
void  init_verify(void);
void  auto_cal_tst(void);
void  auto_cal(U32 BdNum);
void  get_avg_reading(int num_channels, int sample_size);
void  Scan_display();
void  PutCursor(U32,U32);
void  set_vRange(double range);

#define NUM_BOARDS 2


U32 ValueRead, indata, data_in, *BuffPtr, *NewBuffPtr;
U32 ulNumBds, ulBdNum, ulAuxBdNum, ulErr = 0;
U16 saved_x, saved_y, CurX=2, CurY=2;
char mmchar, kbchar, datebuf[16], timebuf[16], test_mode=0x00;
char cBoardInfo[400];
U32 uData[262144], Data[262144];
int dNumChan = 0;
U32 b1Ready,b2Ready,b1Done,b2Done;
double bytes_needed;
U32 buffSize[2] = {0,0};
U32 numBoards;
U32 sampleRate;
U32 dPacked = 0;
double vRange = 10.0;

HANDLE myFlags[4];
PU32 Buff[NUM_BOARDS][4];
U32 *Buff1;
U32 *Buff2;
U32 *wBuff;

U8 contRunning;
volatile U8 contThreadStatus;


//==============================================================================
//
//==============================================================================
void main(int argc, char * argv[])
{

  CursorVisible(FALSE);
  ulNumBds = AI64SSA_FindBoards(&cBoardInfo[0], &ulErr);
  if(ulErr)
  {
   ShowAPIError(ulErr);
   do{}while( !kbhit() );         // Wait for a Key Press
   exit(0);
  }
  if(ulNumBds < 1)
  {
  printf("  ERROR: No Boards found in the system\n");
    do{}while( !kbhit() );         // Wait for a Key Press
  }
  else
  {
    ulBdNum = 0;
    ClrScr();
	printf("\n\n");
    printf("  ====================================================\n");
    printf("   Select Board Number to Test                          \n");
    printf("  ====================================================\n");
    printf("%s\n\n",cBoardInfo);
    do
	{
      kbchar = prompt_for_key("  Please Make selection: \n");
          switch(kbchar)
          {
            case '1':
            {
              ulBdNum = 1;
              break;
            }
            case '2':
            {
              ulBdNum = 2;
              break;
            }
            case '3':
            {
              ulBdNum = 3;
              break;
            }
            case '4':
            {
              ulBdNum = 4;
              break;
            }
            case '5':
            {
              ulBdNum = 5;
              break;
            }
          }
     if((ulBdNum == 0) || (ulBdNum > ulNumBds))
       printf("  Error - This is not a valid entry");
	}while((ulBdNum == 0) || (ulBdNum > ulNumBds));
//	}while((ulBdNum == 0));
 
	AI64SSA_Get_Handle(&ulErr, ulBdNum);
    if(ulErr)
	{
     ShowAPIError(ulErr);
     do{}while( !kbhit() );         // Wait for a Key Press
     exit(0);
	}
    ulAuxBdNum = 0; 
    if(ulNumBds >1){
		  ulAuxBdNum = ulBdNum;
		  ClrScr();
		  printf("\n\n");
		  printf("  ====================================================\n");
		  printf("   Select Auxillary Board Number to Use               \n");
		  printf("   For Examples Requiring Two Boards (0 = Disabled)   \n");
		  printf("  ====================================================\n");
		  printf("%s\n\n",cBoardInfo);
		  do
		  {
			kbchar = prompt_for_key("  Please Make selection: \n");
			  switch(kbchar)
			  {
				case '0':
				{
				  ulAuxBdNum = 0;
				  break;
				}
				case '1':
				{
				  ulAuxBdNum = 1;
				  break;
				}
				case '2':
				{
				  ulAuxBdNum = 2;
				  break;
				}
				case '3':
				{
				  ulAuxBdNum = 3;
				  break;
				}
				case '4':
				{
				  ulAuxBdNum = 4;
				  break;
				}
				case '5':
				{
				  ulAuxBdNum = 5;
				  break;
				}
			  }
			if((ulAuxBdNum == ulBdNum) || (ulAuxBdNum > ulNumBds))
		   printf("  Error - This is not a valid entry");
		  }while((ulAuxBdNum == ulBdNum) || (ulBdNum > ulNumBds));
		if(ulAuxBdNum)
			AI64SSA_Get_Handle(&ulErr, ulAuxBdNum);
		if(ulErr)
		{
		 ShowAPIError(ulErr);
		 do{}while( !kbhit() );         // Wait for a Key Press
		 exit(0);
		}
	}

    do
    {
      ClrScr();
      printf("\n\n");
      printf("  ====================================================\n");
      printf("   16AI64SSA Sample Code - Board # %d                  \n",ulBdNum);
      if(ulAuxBdNum)
		printf("                      Aux  Board # %d                 \n",ulAuxBdNum);
      printf("  ====================================================\n");
      printf("   1 - Board Initialization                           \n");
      printf("   2 - Auto Calibration                               \n");
      printf("   3 - Analog Inputs                                  \n");
      printf("   4 - SelfTest Modes                                 \n");
      printf("   5 - DMA Test                                       \n");
      printf("   6 - Continuous Data (Optionally to Disk)           \n");
      printf("   7 - Simple PIO Data                                \n");
      printf("   8 - External Trigger Demo                          \n");
      if(ulAuxBdNum){
	     printf("   A - Multiple Board Ext Sync Example                \n");
	  }
      if(ulAuxBdNum){
	     printf("   B - Multiple Board SGL DMA Example                 \n");
	  }
      printf("   X - EXIT (return to DOS)                           \n");
      mmchar = prompt_for_key("  Please Make selection: \n");

      switch(mmchar)
      {
        case '1':
          init_verify();
          break;
        case '2':
          auto_cal_tst();
          break;
        case '3':
          analog_inputs();
          break;
        case '4':
          SelfTest();
          break;
        case '5':
          DmaTest();
          break;
        case '6':
          ContinuousSaveData();
          break;
        case '7':
		  Data_Demo();
          break;
        case '8':
		  Ext_Burst_Demo();
          break;
        case 'A':
          if(ulAuxBdNum) // Don't call unless aux board defined and opened
			  MultiBoardExample();
          break;
        case 'B':
          if(ulAuxBdNum) // Don't call unless aux board defined and opened
			  MultiBoard_DMA_Example();
          break;
		case 'X':
          ClrScr();
          break;
        default:
          printf("  Invalid Selection: Press AnyKey to continue...\n");
          getch();
          break;
      }
    }while(mmchar!='X');

	AI64SSA_Close_Handle(ulBdNum, &ulErr);

	CursorVisible(TRUE);
  }
} /* end main */

//------------------------------------------------------------------------------
// Initialization Verification
//------------------------------------------------------------------------------
void init_verify(void)
{
  ClrScr();
  CurX=CurY=2;

  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR,0x8000);
  Busy_Signal(20);
		if(ulErr)
		{
		 ShowAPIError(ulErr);
		 do{}while( !kbhit() );         // Wait for a Key Press
		}

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  cprintf("Board Control Register      = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, ICR);
  cprintf("Interrupt Control Register  = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_BUFF);
  cprintf("Input Data Buffer Register  = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL);
  cprintf("Input Data Control Register = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  cprintf("Rate Control A Register     = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_B);
  cprintf("Rate Control B Register     = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, SCAN_CNTRL);
  cprintf("Scan Control Register       = %08lX", ValueRead);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
  dNumChan = (ValueRead&0x10000) ? 32:64;
  cprintf("FW_REV     * Undocumented * = %08lX : %02d Channels ",  ValueRead,dNumChan);

  PutCursor(CurX,CurY++);
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, AUTOCAL);
  cprintf("AUTOCAL    * Undocumented * = %08lX",  ValueRead);

  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  anykey();
}


//------------------------------------------------------------------------------
// Auto Calibration Test
//------------------------------------------------------------------------------
void auto_cal_tst(void)
{
  GS_NOTIFY_OBJECT Event;
  HANDLE myHandle;
  DWORD EventStatus;
  U32 ulValue,i;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY);
  cprintf("Auto Calibration Test:");

  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(20);
  myHandle =
        CreateEvent(
            NULL,           // Not inheritable to child processes
            FALSE,          // Manual reset?
            FALSE,          // Intial state
            NULL            // Name of object
            );

  if (myHandle == NULL){
		cprintf("Insufficent Resources    ...");
  	    anykey();
        return;
		}

    // Store event handle
  Event.hEvent = (U64)myHandle;
  AI64SSA_EnableInterrupt(ulBdNum, 0x01, LOCAL, &ulErr);
  AI64SSA_Register_Interrupt_Notify(ulBdNum, &Event, 0, LOCAL, &ulErr);

  Busy_Signal(5);
  ulValue = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ulValue | 0x2000); // Initiate Autocal

  EventStatus = WaitForSingleObject(myHandle,10 * 1000); // Wait for the interrupt

  PutCursor(CurX,CurY++);
  switch(EventStatus)
  {
	case WAIT_OBJECT_0:
		cprintf("Interrupt was requested    ...");
		break;
	default:
		cprintf("Interrupt was NOT requested...");
		break;
  }

  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &Event, &ulErr);
  CloseHandle(myHandle);
  AI64SSA_DisableInterrupt(ulBdNum, 0x01, LOCAL, &ulErr);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);

  PutCursor(CurX,CurY++);
  if((indata & 0x2000) != 0)
    cprintf("Auto Cal BCR Bit did NOT clear...");
  else
    cprintf("Auto Cal BCR Bit Cleared   ...   ");

  PutCursor(CurX,CurY++);
  if(indata & 0x4000)
    cprintf("Auto Cal Status Says PASSED... %04lx",indata);
  else
    cprintf("Auto Cal Status Says FAILED... %04lx",indata);

  PutCursor(CurX,CurY++);
  cprintf(" ================================================================\n");
  cprintf("        Gain Off         Gain Off         Gain Off         Gain Off\n");
  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  for(i=0; i< (U32)(2*dNumChan);){
    AI64SSA_Write_Local32(ulBdNum, &ulErr, AUTOCAL, i);
    indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, AUTOCAL);
    AI64SSA_Write_Local32(ulBdNum, &ulErr, AUTOCAL, i+1);
    data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, AUTOCAL);
    cprintf("   Ch%02d %04lX:%04lX", i/2,data_in,indata );
	i+=2;
    if((i%8)==0)
		cprintf("\n");
  }
  anykey();
}


//------------------------------------------------------------------------------
// Analog Inputs
//------------------------------------------------------------------------------
void analog_inputs(void)
{
  char freq;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Analog Input Channels:");
  PutCursor(CurX,CurY++);
  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(20);
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  cprintf("Please select the desired Sample Rate to test ...");
  PutCursor(CurX,CurY++);
  cprintf("     1 -  10K Samples/Sec");
  PutCursor(CurX,CurY++);
  cprintf("     2 - 200K Samples/Sec");
  PutCursor(CurX,CurY++);
  cprintf("Selection: ");
  do
  {
    freq = toupper(getch());
  }while(freq != '1' && freq != '2');

  if (freq == '1')
    AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, ((50000000/20000)&0xFFFF));   // RATNESH:
  else
    AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, ((50000000/200000)&0xFFFF));  


  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Analog Input Channels:");

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  set_vRange(2.5);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  PutCursor(CurX,21);
  auto_cal(ulBdNum);

  PutCursor(CurX,22);
  cprintf("%c2.5V Range  %02d Single Ended Inputs   ", 241,dNumChan);
  PutCursor(CurX,23);
  cprintf("Press anykey for the next Selection...                   ");
  Scan_display();


  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  set_vRange(5.0);
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  PutCursor(CurX,22);
  cprintf("%c5V Range   %02d Single Ended Inputs   ", 241,dNumChan);
  PutCursor(CurX,23);
  cprintf("Press anykey for the next Selection...                   ");
  Scan_display();

  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  set_vRange(10.0);
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  PutCursor(CurX,22);
  cprintf("%c10V Range   %02d Single Ended Inputs   ", 241,dNumChan);
  PutCursor(CurX,23);
  cprintf("Press anykey for the next Selection...                   ");
  Scan_display();

  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Set_Processing_Mode(ulBdNum, &ulErr,0x01); // Pseudo Diff Mode
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  PutCursor(CurX,22);
  cprintf("%c10V Range   %02d Pseudo Diff Inputs   ", 241,dNumChan-1);
  PutCursor(CurX,23);
  cprintf("Press anykey for the next Selection...                   ");
  Scan_display();

  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Set_Processing_Mode(ulBdNum, &ulErr,0x02); // Full Diff Mode
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  PutCursor(CurX,22);
  cprintf("%c10V Range   %02d Full Diff Inputs     ", 241,dNumChan/2);
  PutCursor(CurX,23);
  cprintf("Press anykey for the next Selection...                   ");
  Scan_display();

  AI64SSA_Set_Processing_Mode(ulBdNum, &ulErr,0x0); // Std SE Mode


}


//------------------------------------------------------------------------------
// Selftest Verification
//------------------------------------------------------------------------------
void SelfTest(void)
{
  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("SelfTest Modes:");

  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(10);

	if(ulErr)
	{
	 ShowAPIError(ulErr);
	 do{}while( !kbhit() );         // Wait for a Key Press
	 ClrScr();
	 CurX=CurY=2;
	 PutCursor(CurX,CurY++);
	}

	if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  CurY=10;
  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  AI64SSA_Set_Input_Mode(ulBdNum, &ulErr,0x02); // Zero Selftest
  set_vRange(2.5);										
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("%c2.5V Range - Zero SelfTest - BiPolar   ", 241);
  PutCursor(CurX,20);
  cprintf("Verify 0x8000 Nominal Value              ");
  Scan_display();

  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  AI64SSA_Set_Input_Type(ulBdNum, &ulErr,0x01); // UniPolar
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("0-5V Range - Zero SelfTest - UniPolar    ");
  PutCursor(CurX,20);
  cprintf("Verify 0x0000 Nominal Value              ");
  Scan_display();

  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  AI64SSA_Set_Input_Mode(ulBdNum, &ulErr,0x03); // +Vref Selftest
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("0-5V Range - +Vref SelfTest - UniPolar    ");
  PutCursor(CurX,20);
  cprintf("Verify 0x7FDF Nominal Value              ");
  Scan_display();

  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,4); // 16 Channels
  set_vRange(5.0);
  AI64SSA_Set_Input_Type(ulBdNum, &ulErr,0x0); // BiPolar
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("%c5.0V Range - +Vref SelfTest - BiPolar      ",241);
  PutCursor(CurX,20);
  cprintf("Verify 0xFFDF Nominal Value              ");
  Scan_display();

  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  set_vRange(10.0);
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("%c10.0V Range - +Vref SelfTest - BiPolar     ",241);
  PutCursor(CurX,20);
  cprintf("Verify 0xFFDF Nominal Value              ");
  Scan_display();


  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration   ");
  AI64SSA_Set_Input_Mode(ulBdNum, &ulErr,0x02); // Zero Selftest
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("%c10.0V Range - Zero SelfTest - BiPolar     ",241);
  PutCursor(CurX,20);
  cprintf("Verify 0x8000 Nominal Value              ");

  Scan_display();

  AI64SSA_Set_Input_Mode(ulBdNum, &ulErr,0x0); // SE Input Mode

}



//------------------------------------------------------------------------------
// Auto Calibration Sub
//------------------------------------------------------------------------------
void auto_cal(U32 boardNum)
{
  U32 ulValue;
  ulValue = AI64SSA_Autocal(boardNum, &ulErr);
  switch(ulValue){
	case 0: cprintf("  Autocal FAILED BD#%ld  ...",boardNum); break;
	case MINUS_ONE_LONG: cprintf("  Autocal     ERROR BD#%ld  ...",boardNum); break;
	case 0x55: cprintf("  Insufficent Resources ERROR BD#%ld  ...",boardNum); break;
	case 0xAA: cprintf("  Autocal Interrupt     ERROR BD#%ld  ...",boardNum); break;
	default: cprintf("                           ");break;
  }
}


//==============================================================================
void get_avg_reading(int num_channels, int sample_size)
{
  U32 chan_samp,i,j;

  chan_samp = (U32)(sample_size / num_channels);
  memset(uData,0,256); // Set memory
  for(j=0; j<(U32)sample_size; j+=(U32)num_channels)
  {
	  for(i=0; i<(U32)num_channels; i++)
		  uData[i] += (Data[j+i]&0xFFFF);
	  
  }
  for(i=0; i<(U32)num_channels; i++)
    uData[i] /= chan_samp;
  return;
}


//==============================================================================
void Scan_display()		// Based on # of chans set in Scan & Sync Register
{
  static int prev_chans=64;  // Max Chans
  int chan_num, channels, sample_size, wait_cnt, proc_mode=1;
  U32 chan_mask, yPos = 4, xPos = 7;
  U32 i,j;

  AI64SSA_Dma_DataMode(ulBdNum, 0); // Ensure Normal Data mode
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, SCAN_CNTRL);
  switch(ValueRead & 0x07){
      case 0: channels = 1; break;
      case 1: channels = 2; break;
	  case 2: channels = 4; break;
	  case 3: channels = 8; break;
	  case 4: channels = 16; break;
	  case 5: channels = 32; break;
	  default: channels = 64;
  }
  if(channels==1) // Single Channel mode
      chan_mask = ((ValueRead & 0x3F000)>> 12);
  else
	  chan_mask = channels;

  sample_size = 16 * channels;

  for(i=0,j=0; i<64; i+=8,j+=2){
    PutCursor(xPos-7,yPos+j);
	  if(i >= (U32)channels)
	   cprintf("                                                          ");
	  else
	   cprintf("CH%02d                                                    ",(int)i);
  }
  memset(uData,0,16384); // Set memory
  memset(Data,0,65535); //Set memory
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR); // Determine Processing mode
  if(((indata & 0x200)>>8) == 2)
	  proc_mode =2;
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A); // Disable Clock
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata | 0x10000);
  // Clear Buffer and write threshold value = sample_size
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, 0x40000|(U32)sample_size);
  Sleep(100);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Ensure Buffer Enabled
  data_in &= 0xFFF8FFFF;
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock
												
  do
  {
    Sleep(250);

	AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000); // Clear Buffer
    Sleep(1);
	wait_cnt = 0;
    do
    {
      indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);
	  wait_cnt++;
    }while(((int)indata < sample_size) && (wait_cnt < 10000));
	for(i=0; i<(U32)sample_size; i++)
       Data[i] = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_BUFF);
	PutCursor(xPos,yPos-1);
    cprintf("UPDATE ");
    PutCursor(xPos,yPos);
    get_avg_reading(channels, sample_size);

	if(channels == 1){
        PutCursor(xPos-7,yPos);
		cprintf("CH%02d %04X ", uData[chan_mask]);
	}
	else{
      for(chan_num=0; chan_num<(channels/proc_mode); chan_num++)
	  {
	   if(chan_num == 8)
		PutCursor(xPos,yPos+2);
       if(chan_num == 16) 
		PutCursor(xPos,yPos+4);
	   if(chan_num == 24)
		PutCursor(xPos,yPos+6);
	   if(chan_num == 32)
		PutCursor(xPos,yPos+8);
	   if(chan_num == 40)
		PutCursor(xPos,yPos+10);
	   if(chan_num == 48)
		PutCursor(xPos,yPos+12);
	   if(chan_num == 56)
		PutCursor(xPos,yPos+14);
       cprintf("%04X ", uData[chan_num]);
	  }
     Sleep(250);
     PutCursor(xPos,yPos-1);
     cprintf("        ");
    }
  }while(!kbhit());

  kbflush();
  prev_chans = channels;
}


//==============================================================================
void PutCursor(U32 FixedX, U32 FixedY)
{
  PositionCursor((U16) FixedX,(U16) FixedY);
}

//==============================================================================
void set_vRange(double range){

  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  ValueRead &= 0xFFFFFFCF;
  if(range == 10.0)
	  ValueRead |= 0x20;
  else
   if(range == 5.0)
	  ValueRead |= 0x10;
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR,ValueRead);


}

/*******************************************************************************
This example function uses SGL DMA WITHIN the driver.
It is best used for capturing burst's and small amounts of data.
It free's the user from having to allocate memory and keep track of system resources 
Empirical testing has shown it is good up to ~ 100K sample rate.

For faster sample rates, and/or saving large amounts of data,
the user should consider running DMA on the hardware of the General Standards 
board as shown in the ContinuousSaveData() example function.
*/
//==============================================================================
void DmaTest(){

  HANDLE myHandle;
  DWORD EventStatus;
  GS_NOTIFY_OBJECT event;
  U32 mode, ulMode;
  U32 yPos = 4, xPos = 7;
  U32 sampleSize;
  U32 ulBuffSize = 0;
  U32 myData;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Sgl DMA Mode:");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  cprintf("Please select the desired Data Mode ...");
  PutCursor(CurX,CurY++);
  cprintf("     1 -  Normal");	// Packing only applies to the DMA transfer
  PutCursor(CurX,CurY++);	
  cprintf("     2 -  Packed");
  PutCursor(CurX,CurY++);
  cprintf("Selection: ");
  do
  {
    mode = toupper(getch());
  }while(mode != '1' && mode != '2');

  if (mode == '1')
	ulMode = 0;
  else
	ulMode = 1;
  
  AI64SSA_Dma_DataMode(ulBdNum, ulMode);


  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("DMA Mode:  ");

  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(10);

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ValueRead & 0x7FFFF); // Threshold Mode
  CurY=10;
  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000|((50000000/25000)&0xFFFF)); // 25K Sample
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  sampleSize = 131072 ;
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL,(sampleSize-1)); // Threshold
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  PutCursor(CurX,19);
  cprintf("%c10V Range - SE                         ", 241);
  PutCursor(CurX,20);
  
  AI64SSA_Open_DMA_Channel(ulBdNum, 0, &ulErr);
  if(ulErr){
   cprintf("Error Opening DMA Channel 0\nAny Key to continue..   ");
   AI64SSA_Reset_Device(ulBdNum, &ulErr);
   anykey();
   return;
  }
  myHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (myHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      return;
  }

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A); // Disable Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, data_in|0x10000);

  AI64SSA_EnableInterrupt(ulBdNum, 0x10, 0, &ulErr);

  event.hEvent = (U64)myHandle;

  AI64SSA_Register_Interrupt_Notify(ulBdNum, &event, 0, 0, &ulErr);


  memset(Data,0,131072);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock


  do{
     
     EventStatus = WaitForSingleObject(myHandle,2 * 1000); // Wait for the interrupt

     switch(EventStatus)	
	 {						// If packed data, we will
	  case WAIT_OBJECT_0:	// have 2 channels per long word
	   AI64SSA_DMA_FROM_Buffer(ulBdNum, 0, sampleSize, Data, &ulErr);
       if(ulErr){
           ShowAPIError(ulErr);
           ClrScr();
	   }
       memcpy(&myData,Data,sizeof(U32));
	   PutCursor(xPos,yPos);

       ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);
	   // Use this to determine if overrun (If it hits 0x3FFFF we overrun)
	   // Of course, the call takes time, so eliminate if critical
       cprintf("%04X : %08X ", ulBuffSize,myData);
	   break;
	  default:
	    PutCursor(2,9);
		cprintf("\nError... Interrupt Timeout\n");
        AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
        AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
        AI64SSA_Close_DMA_Channel(ulBdNum, 0, &ulErr);
        ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);// the FIFO
        cprintf("Buff Size, %08X : ICR, %08X", ulBuffSize,AI64SSA_Read_Local32(ulBdNum, &ulErr, ICR)); // interrupt. If it hits 0xFFFF we overrun
        CloseHandle(myHandle);
		anykey();
        return;
	 }

  }while(!kbhit());
  kbflush();
  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
  AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
  AI64SSA_Close_DMA_Channel(ulBdNum, 0, &ulErr);
  CloseHandle(myHandle);

//  AI64SSA_Reset_Device(ulBdNum, &ulErr);
  AI64SSA_Dma_DataMode(ulBdNum, 0); // Set normal mode 
 
}

/*******************************************************************************
This example function uses SGL DMA running on the General Standards board.
It is best used for fast sample rates, and/or large amounts of data.

The user must obtain physical memory for the DMA Controller to use, hence
the block sizes (up to the DMA max) are totally dependent on the users
machine and free system resources. THE MAX DMA TRANSFER SIZE is (1<<23)-1 BYTES.

The user can setup a maximum of four (4) descriptors per board. Each descriptor
details the transfer parameters to the DMA controller, and points to the next
descriptor, with the last pointing to the first. 
This is commonly called command chaining, hence the function names.

The user can select to get notification at the completion of a block. See
the definition of the GS_DMA_DESCRIPTOR structure for all parameters.

This example uses two (2) descriptors, and shows the steps the user should
include in setting up and transferring data. It is NOT an application, and
has been written at the simplest level so all level of users can understand
what is required to achieve the objective. 
********************************************************************************/

//==============================================================================
void ContinuousSaveData(){

  HANDLE myHandle;
  HANDLE hThread;
  HANDLE myThread;
  DWORD EventStatus;
  DWORD pThreadId;
  LARGE_INTEGER startTime, stopTime, sysFreq, elapsedTime;
  ULONGLONG i64FreeBytesToCaller;
  ULONGLONG i64TotalBytes;
  ULONGLONG i64FreeBytes;
  GS_DMA_DESCRIPTOR DmaSetup;              //ratnesh: Typedef struct used. To use any of those variables, use DmaSetup.ThatVariable.
  GS_PHYSICAL_MEM Block1,Block2;
  GS_NOTIFY_OBJECT event;
  double bytes_needed;
  U32 blk1_allocated, blk2_allocated;
  U32 yPos = 4, xPos = 7;
  U32 ulBuffSize = 0;
  U32 memNeeded;
  U32 *myData;
  U32 sec_to_run;
  U32 k;
  U8 fResult;
  U8 enable_write = 0;
  U8 pack_data = 0;
  U8 ok = 1;
  double z;
  ldiv_t result;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Continuous Data             ");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  kbflush();
  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;                     //ratnesh: FW_REV register 0000 0028h with default value for this board should be 0000 0000h.
  }                                                             // ratnesh: value read = 0000 0000h ANDing it with 0001 0000h will give 0. so expression false hence dNumChan=64

BADRATE:
  cprintf("Please enter the sample rate (Hz)... ");
  scanf("%ld",&sampleRate);
  kbflush();
  if((sampleRate < 5000) || (sampleRate > 200000))
  {
      cprintf("\n  Invalid Rate %ld (5K Min : 200000 Max)",sampleRate);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATE;
  }
  PutCursor(CurX,CurY++);

  cprintf("Please enter the desired collection time (Sec)... ");
  scanf("%ld",&sec_to_run);
  kbflush();

  PutCursor(CurX,CurY++);

  kbchar = prompt_for_key("Write data to Disk ? Y or N");
  if(kbchar == 'Y')
	  enable_write = 1;
  kbflush();

  PutCursor(CurX,CurY++);

  kbchar = prompt_for_key("Pack Data ? Y or N");
  if(kbchar == 'Y')
	  pack_data = 1;
  kbflush();

  numBoards = 1;
  memNeeded = (U32)((double)sampleRate * (double)dNumChan);   //ratnesh: sampleRate=10KHz, dNumChan=64 => memNeeded=640K
  if(pack_data)
    memNeeded /= 2;                                       //ratnesh: since when data is packed, per 32 bit register (input buffer reg?) 2 channel data will be stored. memNeeded = 320K
  bytes_needed = ((double)memNeeded*(double)sec_to_run*(double)4.0);    //ratnesh: bytes_needed = 320K x 10 x 4 = 12800 KBytes = 12.8MB But why 4? Is it because of making it 32 bits?
  if(enable_write)
  {	
   fResult = GetDiskFreeSpaceExA ("c:",                                // ratnesh standard function, defined in fileapi.h. Will return available memory in Bytes.
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&i64TotalBytes,
                (PULARGE_INTEGER)&i64FreeBytes);

  
   if(!fResult)
   {
	  cprintf("\n  Unable to determine available disk space\n  Cannot continue");
	  anykey();
	  kbflush();
      return;
   }
   else
   {
      if(i64FreeBytesToCaller < (ULONGLONG)bytes_needed)
	  {
      cprintf("\n  Not Enough Disk Space for %.1lf MBytes",((double)memNeeded*(double)sec_to_run*4.0)/1000000.0);  //ratnesh: divide by 1000000 is to change it in MB.
	  PutCursor(CurX,CurY);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATE;
	  }
   }
  }
  if(QueryPerformanceFrequency(&sysFreq) == 0)                      //ratnesh: standard function. This will not be = 0 on systems that run Windows XP or later
  {
      cprintf("\n  Performance Counters Not Supported - Cannot Continue\n");
	  anykey();
	  kbflush();
      ClrScr();
	  return;
  }

  wBuff = (U32 *)calloc(memNeeded,sizeof(U32));    //ratnesh:“calloc” or “contiguous allocation” method is used to dynamically allocate the specified number of blocks of memory of the specified type.
  if(wBuff == (PU32)NULL)
  {
      cprintf("\n  Not Enough Memory for %ld Mb",(memNeeded*4)/1000000); 
	  PutCursor(CurX,CurY);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATE;
  }
  PutCursor(CurX,19);
  cprintf("Getting Memory              ");                                    // ratnesh: How much is the size(GS_PHYSICAL_MEM)??
  memset(&Block1, 0, sizeof(GS_PHYSICAL_MEM));                                // ratnesh: memset() is standard function that is used to fill a block of memory with a particular value
  memset(&Block2, 0, sizeof(GS_PHYSICAL_MEM));                                // ratnesh:void *memset(void *ptr, int x, size_tn) // ptr ==> Starting address of memory to be filled
                                                                                                                                 // x   ==> Value to be filled
                                                                                                                                 // n   ==> Number of bytes to be filled starting 
                                                                                                                                 //         from ptr to be filled

  // This is the memory we will be using for DMA
  // Try for 1 seconds worth, but if > than can transfer in one block,
  // decrement till there
  while((memNeeded*4) > ((1<<23)-1))                           // ratnesh: memNeeded*4 = 320KBytes*4 = 1280 KBytes (1 second of data)  1<<23 = 1 x 2^23 = 8,388,608 bits = 1,048,576 Bytes = 1048 KBytes
		 memNeeded /= 2;                                       // ratnesh: Therefore memNeeded = 320KB/2 = 160KB
  Block1.Size = memNeeded*4;                                   // typedef struct used here.  
                                                               // Block1.Size = 160K x 4 = 640 KB
  AI64SSA_Get_Physical_Memory(ulBdNum, &Block1, 1, &ulErr); // but settle for less

  blk1_allocated = Block1.Size;              //ratnesh: blk1_allocated = 640 KB
  result = ldiv(Block1.Size,dNumChan);       // ratnesh: result = 640 KB/64 = 10 KB

  Block1.Size = (U32)(result.quot * dNumChan); // Ensure integer multiple of channels   //ratnesh: result.quot = 10KB ? hence Block1.Size = 10 KB x 64 = 640 KB. 
  Block2.Size = Block1.Size;// Try to make them both equal, regardless of size          

  AI64SSA_Get_Physical_Memory(ulBdNum, &Block2, 1, &ulErr); // 1 = will take less

  blk2_allocated = Block2.Size;
  result = ldiv(Block2.Size,dNumChan);
  Block2.Size = (U32)(result.quot * dNumChan); // Ensure integer multiple of channels

  Buff1 = (U32*)Block1.UserAddr;               // ratnesh: typedef struct used here.
  Buff2 = (U32*)Block2.UserAddr;

  myData = Buff1;
  
  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Continuous Data %s for %ld seconds:",( (enable_write) ? "to disk":"Monitor") ,sec_to_run);       // ratnesh: It writes, "Continuous data to disk for 10 seconds" 
                                                                                                        // ratnesh: Depending on the ternary operator (?) "Monitor" will come.
  AI64SSA_Initialize(ulBdNum, &ulErr);        // ratnesh: Why initialize it again here?
  Busy_Signal(20);

  CurY=10;
  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration  ");
  
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);     // ratnesh: Since data packing is enabled (D18=1), reading BCR will provide = 000C 4060h = ValueRead

  if(pack_data)
	  ValueRead |= 0x40800;// Packed data & disable scan marker  //ratnesh: ValueRead = ValueRead | 0x40800 i.e. 000C 4060 | 0004 0800 = 000C 4860
 
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ValueRead | 0x80000); // Demand Mode Required
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000 | ((50000000/sampleRate)&0xFFFF)); // Sample Rate //ratnesh: 5000 (=0x1388) & 0xFFFF = 0x1388
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);               // ratnesh: Since dNumChan above is 64 hence 6 will be written to SCAN_CTRL register.

  auto_cal(ulBdNum);                    // ratnesh: Calling first of those 5 small inbetween functions.

  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL,ValueRead|0x40000);     //ratnesh: ValueRead (default for this reg = 0003 FFFE | 0004 0000 = 0007 FFFE. 
  
																			   //ratnesh: This will clear the buffer (D18 becomes 1)
  myThread = GetCurrentThread();                                               //ratnesh: Standard Function in processthreadsapi.h
  SetThreadPriority(myThread,THREAD_PRIORITY_TIME_CRITICAL);                   //ratnesh: Standard Function in processthreadsapi.h

  if(enable_write)
  {
   contThreadStatus = 0;                                                      //ratnesh: these 4 vaiables are being initialised here, they have been declared in the beginning.
   contRunning = 1;
   b1Ready=0;
   b2Ready=0;
  
   hThread = CreateThread(                                     //ratnesh: Standard Function in processthreadsapi.h
        NULL,                          // Security
        0,                             // Same stack size
        InterruptAttachThreadDma,      // Thread start routine
        (LPVOID)&bytes_needed,           //
        0,                             // Creation flags
        &pThreadId                     // Thread ID
        );

    SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
    Sleep(300);
    // Wait till thread starts
    while (contThreadStatus != 1)                                //ratnesh: Will be true since above, we gave contThreadStatus = 0
	{
        Sleep(1);
		if(GetExitCodeThread(hThread,&pThreadId) != STILL_ACTIVE)
		{
		 cprintf("\nThread Aborted - Cannot continue");
        Block1.Size = blk1_allocated;                            //ratnesh:Freeing captured memory
        Block2.Size = blk2_allocated;                            //ratnesh:Freeing captured memory
        AI64SSA_Free_Physical_Memory(ulBdNum, &Block1, &ulErr);  //ratnesh:Freeing captured memory
        AI64SSA_Free_Physical_Memory(ulBdNum, &Block2, &ulErr);  //ratnesh:Freeing captured memory
        free(wBuff);                                             //ratnesh:Freeing captured memory
		anykey();
		kbflush();
		return;
		}
	}
  }
  else
  {
   contThreadStatus = 0;
   contRunning = 0;
  }
  if(pack_data)
	  dPacked = 1;
  else
	  dPacked = 0;

  AI64SSA_Dma_DataMode(ulBdNum, dPacked);

  // Setup CmdChaining here
  memset(&DmaSetup, 0, sizeof(GS_DMA_DESCRIPTOR));

  DmaSetup.DmaChannel = 1;
  DmaSetup.NumDescriptors = 2;                   // ratnesh: Number of Descriptor blocks to use
  DmaSetup.LocalToPciDesc_1 = 1; 
  DmaSetup.BytesDesc_1 = Block1.Size;            // BYTES to transfer (U32 = 4 Bytes)
  DmaSetup.PhyAddrDesc_1 = Block1.PhysicalAddr;  // Valid PHYSICAL address for contiguous
  DmaSetup.InterruptDesc_1 = 1;                  // Enable interrupt at Descriptor completion
  DmaSetup.LocalToPciDesc_2 = 1;
  DmaSetup.BytesDesc_2 = Block2.Size;
  DmaSetup.PhyAddrDesc_2 = Block2.PhysicalAddr;
  DmaSetup.InterruptDesc_2 = 1;

  if(AI64SSA_Setup_DmaCmdChaining(ulBdNum, &DmaSetup,  &ulErr))
  {
      ShowAPIError(ulErr);
      SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
      contRunning = 0;
      while (contThreadStatus == 1)
        Sleep(1); 
      Block1.Size = blk1_allocated;
      Block2.Size = blk2_allocated;
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block1, &ulErr);
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block2, &ulErr);
      free(wBuff);
      return;
  }

  myHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);

  if (myHandle == NULL)
  {
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
      contRunning = 0;
      while (contThreadStatus == 1)
        Sleep(1); 
      Block1.Size = blk1_allocated;
      Block2.Size = blk2_allocated;
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block1, &ulErr);
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block2, &ulErr);
      free(wBuff);
      return;
  }

  AI64SSA_EnableInterrupt(ulBdNum, 1, 1, &ulErr); // DMA Channel 1

  event.hEvent = (U64)myHandle;

  AI64SSA_Register_Interrupt_Notify(ulBdNum, &event, 1, 1, &ulErr);

  PutCursor(CurX,19);
  cprintf("Data %s Started                                 ",(enable_write)?"Collection":"Monitor");
  
  
  k=0;
  z = 0.0;
  PutCursor(xPos,yPos);

  QueryPerformanceCounter(&startTime);
  if(AI64SSA_Start_DmaCmdChaining(ulBdNum, DmaSetup.DmaChannel,  &ulErr))
  {
      ShowAPIError(ulErr);
      SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
      contRunning = 0;
      while (contThreadStatus == 1)
        Sleep(1); 
      Block1.Size = blk1_allocated;
      Block2.Size = blk2_allocated;
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block1, &ulErr);
      AI64SSA_Free_Physical_Memory(ulBdNum, &Block2, &ulErr);
      free(wBuff);
      return;
  }


  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);         // Enable Clock
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); 



  do
  {    
     EventStatus = WaitForSingleObject(myHandle,4 * 1000); // Wait for the interrupt
	 switch(EventStatus)	
		 {						
			case WAIT_OBJECT_0:	
			{
             if(k > ((Block2.Size/4)-128)) // All of the lines to if(enable_write)
		       k=0;						   // are for display management and/or
			 result = ldiv(k,30);		   // error checking.
             if(!(result.rem)){
			   PutCursor(xPos,yPos);
			   cprintf("                                                                 ");
               PutCursor(xPos,yPos);
			 }
//           We only use Block2.Size since it will always be < or = Block1.Size
	         if(!(*(myData+k) & 0x10000)){
			   ok = 0;
			   cprintf("\nError occurred @ %.1lfMSamples",(z*(double)(Block2.Size/4))/1000000.0);
			 PutCursor(xPos,yPos);
 	         cprintf("Data %lx : Buff Size = %06ld    ",*(myData+k),AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE));         // Enable Clock
			   break;
			 }
            if(enable_write){
			  if(!b1Ready){
				if(!b1Done){
					cprintf("Not fast enough - exiting");
					contRunning = 0;
					while (contThreadStatus == 1)
						Sleep(1);
					sec_to_run = 0; // Abort
				} // !b1Done
					b1Ready = Block1.Size/4;
					b2Ready = 0;
			  } // !b1Ready
			 else{
				if(!b2Done){
					cprintf("Not fast enough - exiting");
					contRunning = 0;
					while (contThreadStatus == 1)
					Sleep(1); 
					sec_to_run = 0; // Abort
				} // !b2Done
				    b1Ready = 0;
					b2Ready = Block2.Size/4;
			 } // end else
			 
			} // end enable_write
			else{
 			 cprintf("*");
//          If U want to see the data @ location k or the buffer size value
//            uncomment the next two lines


			}			
	           z+=1.0;
	           k+=64*4;
			} // end WAIT_OBJECT_0
				
			break;
			default:
				kbflush();
				cprintf("Error... Interrupt Timeout\n");
 				cprintf("Buff Size = %ld",AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE));         // Enable Clock


				ok=0;
				break;

	 } // End Switch EventStatus
     QueryPerformanceCounter(&stopTime);
	 elapsedTime.QuadPart = (stopTime.QuadPart - startTime.QuadPart)/sysFreq.QuadPart;
	 
  }while((U32)elapsedTime.QuadPart < sec_to_run && ok);

  SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);

  contRunning = 0;
  while (contThreadStatus == 1)
    Sleep(1);				// Because Block1 & Block2 sizes may not be equal
  if(ok && !enable_write)   // we don't try to determine the exact # 
	  cprintf("\n  No Errors\nMinimum of %.1lfMSamples Monitored",(z*(double)(Block2.Size/4))/1000000.0);
  PutCursor(CurX,19);
  cprintf("Data %s Done                                 \n",(enable_write)?"Collection":"Monitor");
  
  // Stop Cmd Chaining here
  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
  AI64SSA_Close_DmaCmdChaining(ulBdNum, DmaSetup.DmaChannel,  &ulErr);
  Block1.Size = blk1_allocated;
  Block2.Size = blk2_allocated;
  AI64SSA_Free_Physical_Memory(ulBdNum, &Block1, &ulErr);
  AI64SSA_Free_Physical_Memory(ulBdNum, &Block2, &ulErr);
  free(wBuff);
  CloseHandle(myHandle);
  AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
  AI64SSA_Reset_Device(ulBdNum, &ulErr);
  anykey();
  kbflush();
}

// Function to write data to disk when told
// This function is rudimentary at best, but provides a simple method (flags)
// to signal when to write our data. 

DWORD WINAPI
InterruptAttachThreadDma(
    LPVOID pParam
    )
{
  FILE *myFile[1024];
  U32 wSize;
  U32 b1Written = 0;
  U32 b2Written = 0;
  U32 samplesWritten = 0;
  char fName[256];
  int filenum = 0;
  int k,l;
  int files_needed;
  double total_written;
  double range;

  bytes_needed = *((double *)(pParam));
  if(bytes_needed < 3000000000.0)
   files_needed = 1;
  else
   files_needed = ((int)(bytes_needed / 3000000000.0)+1);
  if(files_needed >= 1024)
	  files_needed = 1024;

/* 10 Bytes
   Header Byte
   Sample Rate
   # of Chans
   # of bits
   voltage range  - First byte mantissa - Second byte remainder * 1000
   bipolar / unipolar(=1)
   offset binary / twos complement(=1)
   unpacked / packed data(=1)
   sequential / random(=1)
*/  
  *wBuff = 0xEADE0;                         //ratnesh_POINTER IS INITIALIZED HERE, IT IS DECLARED SOMEWHERE ELSE.
  *(wBuff+1) = sampleRate;
  *(wBuff+2) = dNumChan;
  *(wBuff+3) = 16;
  *(wBuff+5) = (U32)(modf(vRange,&range)*1000.0); // Need remainder here i.e .5 * 1000
  *(wBuff+4) = (U32)range;
  *(wBuff+6) = 0; // bipolar
  *(wBuff+7) = 0; // offset binary
  *(wBuff+8) = dPacked; // unpacked data
  *(wBuff+9) = 0; // Channels in ascending order

  for(k=0; k<files_needed; k++){
  	sprintf(fName,"AI64SSData%04d.bin",k);   
  myFile[k] = fopen(fName,"w+b"); // Doesn't append, always overwrites
  if(myFile[k] == NULL){
	  cprintf("\n  Error opening file for save.. Cannot continue");
	  l=k;
	  for(l=0; l<k; l++)
	  fclose(myFile[l]);	
	  contThreadStatus = 0;
	  return 0;
  }
	fwrite(wBuff,sizeof(U32),10,myFile[k]);// Write the header info
  } // end For()

  b1Done = 1;
  b2Done = 1;
  total_written = 0.0;

  // Signal that thread is ready
    contThreadStatus = 1;

    while(contRunning)
	{
	   if(samplesWritten > 750000000){
		 total_written += (double)samplesWritten;
   	     if(filenum < 1023)
			 filenum++;
		 samplesWritten = 0;
	   }
		if(b1Ready && !b1Written){
		   b1Done = 0;
		   wSize = b1Ready;
		   samplesWritten += b1Ready;
	       memmove(wBuff,Buff1,wSize*4); // BYTES
		   b1Done = 1;
		   fwrite(wBuff,sizeof(U32),wSize,myFile[filenum]);
           cprintf("*");
		   b1Written = 1;
		   b2Written = 0;
		}
		if(b2Ready && !b2Written){
		   b2Done = 0;
		   wSize = b2Ready;
		   samplesWritten += b2Ready;
	       memmove(wBuff,Buff2,wSize*4); // BYTES
		   b2Done = 1;
		   fwrite(wBuff,sizeof(U32),wSize,myFile[filenum]);
           cprintf("+");
		   b2Written = 1;
		   b1Written = 0;
		}
	 Sleep(10); // Give it some time to actually write the data
				 // Decrease this to speed up the loop

	} // End while(contRunning)

	for(k=0; k<files_needed; k++)
	  fclose(myFile[k]);
	if(filenum == 0)
		total_written = (double)samplesWritten;
    cprintf("\n  %.1lf Samples successfully written ..   \n",total_written);
    contThreadStatus = 0;
 return 0;
}


//==============================================================================
//
// This example uses the external sync pin to clock the target board
// from the initiator board. Connect pin b40 from board1 to b40 of board2
//
//==============================================================================

#define NUM_BLOCKS 2

void MultiBoardExample(){

  HANDLE theHandles[MAXIMUM_WAIT_OBJECTS];
  HANDLE fbHandle;
  HANDLE sbHandle;
  HANDLE hThread;
  HANDLE myThread;
  DWORD EventStatus;
  DWORD pThreadId;
  LARGE_INTEGER startTime, stopTime, sysFreq, elapsedTime;
  ULONGLONG i64FreeBytesToCaller;
  ULONGLONG i64TotalBytes;
  ULONGLONG i64FreeBytes;
  GS_DMA_DESCRIPTOR DmaSetup[NUM_BOARDS];
  GS_PHYSICAL_MEM Bd_Block[NUM_BOARDS][NUM_BLOCKS];
  GS_NOTIFY_OBJECT event[2];
  U32 Bd_Block_allocated[NUM_BOARDS][NUM_BLOCKS];
  LPVOID lpMsgBuf;
  U32 yPos = 4, xPos = 7;
  U32 sampleRate;
  U32 memNeeded;
  U32 smallestSize;
  U32 sec_to_run;
  U32 i,j,k;
  U8 fResult;
  U8 enable_write;
  U8 ok;
  double z;
  ldiv_t result;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Continuous Data             ");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  kbflush();
  if(dNumChan == 0){ // Assumes both the same
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
BADRATEB:
  enable_write=0;
  cprintf("Please enter the sample rate (Hz)... ");
  scanf("%ld",&sampleRate);
  kbflush();
  if((sampleRate < 5000) || (sampleRate > 200000)){
      cprintf("\n  Invalid Rate %ld (5K Min : 200K Max)",sampleRate);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATEB;
  }
  PutCursor(CurX,CurY++);

  cprintf("Please enter the desired collection time (Sec)... ");
  scanf("%ld",&sec_to_run);
  kbflush();

  PutCursor(CurX,CurY++);

  kbchar = prompt_for_key("Write data to Disk ? Y or N");
  if(kbchar == 'Y')
	  enable_write = 1;
  kbflush();
  ok = 1;
  numBoards = NUM_BOARDS;
  memNeeded = (U32)((double)sampleRate * (double)dNumChan * numBoards);// 2 Boards 
  bytes_needed = ((double)memNeeded*(double)sec_to_run*(double)4.0);// Required disk space
  if(enable_write){	
   fResult = GetDiskFreeSpaceExA ("c:",
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&i64TotalBytes,
                (PULARGE_INTEGER)&i64FreeBytes);

  
   if(!fResult){
	  cprintf("\n  Unable to determine available disk space\n  Cannot continue");
	  anykey();
	  kbflush();
      return;
   }
   else{
      if(i64FreeBytesToCaller < (ULONGLONG)bytes_needed){
      cprintf("\n  Not Enough Disk Space for %.1lf MBytes",bytes_needed/1000000.0); 
	  PutCursor(CurX,CurY);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATEB;
	  }
   }
  }
  if(QueryPerformanceFrequency(&sysFreq) == 0){
      cprintf("\n  Performance Counters Not Supported - Cannot Continue\n");
	  anykey();
	  kbflush();
      ClrScr();
	  return;
  }

  wBuff = (U32 *)calloc(memNeeded,sizeof(U32));
  if(wBuff == (PU32)NULL){
      cprintf("\n  Not Enough Memory for %ld Bytes",(memNeeded*4)); 
	  PutCursor(CurX,CurY);
	  anykey();
	  kbflush();
      ClrScr();
	  PutCursor(CurX,CurY);
	  goto BADRATEB;
  }


  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Continuous Data %s for %ld seconds:",((enable_write)?"to disk":"Monitor"),sec_to_run);

  AI64SSA_Initialize(ulBdNum, &ulErr);
  AI64SSA_Initialize(ulAuxBdNum, &ulErr);
  Busy_Signal(10);

  CurY=10;
  PutCursor(CurX,19);
  cprintf("Performing AutoCalibration                               ");
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ValueRead | 0x80000); // Demand Mode
  ValueRead = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, BCR, ValueRead | 0x80000); // Demand Mode

  // Set the sample rate on the initiator board
  // This will be used to clk both boards when we are ready to go
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, ((50000000/sampleRate)&0xFFFF)); // Sample Rate
  // Set the sample rate on the target board
  // This is used to do autocal only, then will switch to external
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, RATE_A, ((50000000/sampleRate)&0xFFFF)); // Sample Rate

  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);

  auto_cal(ulBdNum);
  auto_cal(ulAuxBdNum);
 

  // Setup the target board for external sync
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, SCAN_CNTRL);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, SCAN_CNTRL,0x10 | data_in); // Target

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR); // Initiator
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR,0x80 | data_in);
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, BCR); // Target
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, BCR,0x80 | data_in);

  // Ensure we have a clock to the target board before we proceed
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  Sleep(250);
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, BUFF_SIZE); // Read number of samples
  if(data_in < 100){ // Will be 0 if no clock
   PutCursor(CurX,19);
   cprintf("No Clock to target board - Aborting                        \n");
   anykey();
   free(wBuff);
   return;
  }

  // Stop the clock till we get all dressed up and ready to go
  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000 | data_in); // Stop initiator

  dPacked = 0; 
  AI64SSA_Dma_DataMode(ulBdNum, 0);
  AI64SSA_Dma_DataMode(ulAuxBdNum, 0);

  PutCursor(CurX,19);
  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);

  PutCursor(CurX,19);
  cprintf("Getting Memory              ");

  fbHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (fbHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      free(wBuff);
      return;
  }

  sbHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (sbHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      CloseHandle(fbHandle);
      free(wBuff);
      return;
  }


  // This is the memory we will be using for DMA
  // Try for 1 seconds worth, but if > than can transfer in one block,
  // use max size for # of channels
  if((memNeeded*4) > ((1<<23)-1)){
		result = ldiv(((1<<23)-1),dNumChan); 
		memNeeded = (U32)((result.quot * dNumChan)/4);
  }
  smallestSize = memNeeded*4; // Start with the biggest block we can use

  // Scenerio
  // Start out trying to get all blocks as big as we can use, and if successful ok
  // else... reduce the requested size for the next block to be equal to the smallest
  // block obtained at the time. This will reduce wasted memory, and increase the
  // chance of obtaining bigger blocks as we proceed
  // 
  // We need all the blocks we use to be the same size when using multiple boards
  // The AI64SSA does not have channel tags on all the samples, so WE must ensure
  // the blocks are saved in a particular order to preserve signal integrity.
  for(i=0; i<NUM_BOARDS; i++){
   for(j=0; j<NUM_BLOCKS; j++){
    memset(&Bd_Block[i][j], 0, sizeof(GS_PHYSICAL_MEM));
	Bd_Block[i][j].Size = smallestSize;
    if(!i) // Board # may not be in order, so assign 0 to first board #
	 AI64SSA_Get_Physical_Memory(ulBdNum, &Bd_Block[i][j], 1, &ulErr);
    else
	 AI64SSA_Get_Physical_Memory(ulAuxBdNum, &Bd_Block[i][j], 1, &ulErr);
    Bd_Block_allocated[i][j] = Bd_Block[i][j].Size;
    result = ldiv(Bd_Block[i][j].Size,dNumChan);
    Bd_Block[i][j].Size = (U32)(result.quot * dNumChan); // Ensure integer multiple of channels
	if(Bd_Block[i][j].Size < smallestSize) // Need them all the same size
	 smallestSize = Bd_Block[i][j].Size;
   } // end j
  } // end i
	
  for(i=0; i<NUM_BOARDS; i++){ // Ensure all the same size
   for(j=0; j<NUM_BLOCKS; j++){// and while we're at it, save the virtual address'
	Bd_Block[i][j].Size = smallestSize;
	Buff[i][j] = (U32*)Bd_Block[i][j].UserAddr;
   }
  }

  buffSize[0] = smallestSize/4;
  buffSize[1] = smallestSize/4;
  // Setup CmdChaining on first board
  memset(&DmaSetup[0], 0, sizeof(GS_DMA_DESCRIPTOR));
  DmaSetup[0].NumDescriptors = 2;
  DmaSetup[0].LocalToPciDesc_1 = 1;
  DmaSetup[0].BytesDesc_1 = Bd_Block[0][0].Size;
  DmaSetup[0].PhyAddrDesc_1 = Bd_Block[0][0].PhysicalAddr;
  DmaSetup[0].InterruptDesc_1 = 1;
  DmaSetup[0].LocalToPciDesc_2 = 1;
  DmaSetup[0].BytesDesc_2 = Bd_Block[0][1].Size;
  DmaSetup[0].PhyAddrDesc_2 = Bd_Block[0][1].PhysicalAddr;
  DmaSetup[0].InterruptDesc_2 = 1;
  if(AI64SSA_Setup_DmaCmdChaining(ulBdNum, &DmaSetup[0],  &ulErr)){
      ShowAPIError(ulErr);
      for(i=0; i< NUM_BOARDS; i++){
		  for(j=0; j< NUM_BLOCKS; j++){
             Bd_Block[i][j].Size = Bd_Block_allocated[i][j];
			 if(!i)
			  AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[i][j], &ulErr);
			 else
			  AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[i][j], &ulErr);
		  }
	  }
      CloseHandle(fbHandle);
      CloseHandle(sbHandle);
      free(wBuff);
      return;
  }
  // Setup CmdChaining on second board
  memset(&DmaSetup[1], 0, sizeof(GS_DMA_DESCRIPTOR));
  DmaSetup[1].DmaChannel = 1;
  DmaSetup[1].NumDescriptors = 2;
  DmaSetup[1].LocalToPciDesc_1 = 1;
  DmaSetup[1].BytesDesc_1 = Bd_Block[1][0].Size;
  DmaSetup[1].PhyAddrDesc_1 = Bd_Block[1][0].PhysicalAddr;
  DmaSetup[1].InterruptDesc_1 = 1;
  DmaSetup[1].LocalToPciDesc_2 = 1;
  DmaSetup[1].BytesDesc_2 = Bd_Block[1][1].Size;
  DmaSetup[1].PhyAddrDesc_2 = Bd_Block[1][1].PhysicalAddr;
  DmaSetup[1].InterruptDesc_2 = 1;
  if(AI64SSA_Setup_DmaCmdChaining(ulAuxBdNum, &DmaSetup[1],  &ulErr)){
      ShowAPIError(ulErr);
      for(i=0; i< NUM_BOARDS; i++){
		  for(j=0; j< NUM_BLOCKS; j++){
             Bd_Block[i][j].Size = Bd_Block_allocated[i][j];
			 if(!i)
			  AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[i][j], &ulErr);
			 else
			  AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[i][j], &ulErr);
		  }
	  }
      free(wBuff);
      CloseHandle(fbHandle);
      CloseHandle(sbHandle);
      return;
  }

  AI64SSA_EnableInterrupt(ulBdNum, 0, 1, &ulErr); // DMA Channel 0
  AI64SSA_EnableInterrupt(ulAuxBdNum, 1, 1, &ulErr); // DMA Channel 1

  event[0].hEvent = (U64)fbHandle;
  event[1].hEvent = (U64)sbHandle;

  AI64SSA_Register_Interrupt_Notify(ulBdNum, &event[0], 0, 1, &ulErr);
  AI64SSA_Register_Interrupt_Notify(ulAuxBdNum, &event[1], 1, 1, &ulErr);

  theHandles[0] = (U64*)fbHandle;
  theHandles[1] = (U64*)sbHandle;

  PutCursor(CurX,19);
  cprintf("Data %s Started                                 ",(enable_write)?"Collection":"Monitor");

  myThread = GetCurrentThread();
  SetThreadPriority(myThread,THREAD_PRIORITY_TIME_CRITICAL);
  if(enable_write){
   contThreadStatus = 0;
   contRunning = 1;
   for(i=0; i<NUM_BLOCKS; i++){
      myFlags[i] = CreateSemaphore(NULL,
									0,
									1,
									NULL
								   );
	  if (myFlags[i] == NULL){ //           The semaphores have to be valid for the thread
		  ulErr = ApiInsufficientResources;//to start
		  ShowAPIError(ulErr);
		  for(j=0; j<i; j++)
			  CloseHandle(myFlags[j]);
          for(k=0; k< NUM_BOARDS; k++){
		      for(j=0; j< NUM_BLOCKS; j++){
                 Bd_Block[k][j].Size = Bd_Block_allocated[k][j];
			     if(!k)
			      AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[k][j], &ulErr);
			     else
			      AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[k][j], &ulErr);
			  } // End for(j)
		  } // End for(k)
		  CloseHandle(fbHandle);
		  CloseHandle(sbHandle);
		  free(wBuff);
		  return;
	  } // End if(theFlags...
   } // End for(i)
  
   hThread = CreateThread(
        NULL,                          // Security
        0,                             // Same stack size
        DiskWriteFunction,             // Thread start routine
        (LPVOID)&bytes_needed,         //
        0,                             // Creation flags
        &pThreadId                     // Thread ID
        );


    SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
    Sleep(2000);
    // Wait till thread starts
    while (contThreadStatus != 1) {
        Sleep(1);
		if(GetExitCodeThread(hThread,&pThreadId) != STILL_ACTIVE) {
         FormatMessage( 
		 FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		 FORMAT_MESSAGE_FROM_SYSTEM | 
		 FORMAT_MESSAGE_IGNORE_INSERTS,
		 NULL,
		 GetLastError(),
		 0, // Default language
		 (LPTSTR) &lpMsgBuf,
		 0,
		 NULL 
	     );
         cprintf("%s",lpMsgBuf);
		 // Free the buffer.
         LocalFree( lpMsgBuf );
		 cprintf("\nThread Aborted - Cannot continue");
		 anykey();
		 kbflush();
		 for(i=0; i<NUM_BLOCKS; i++)
			 CloseHandle(myFlags[i]);
         for(k=0; k< NUM_BOARDS; k++){
		      for(j=0; j< NUM_BLOCKS; j++){
                 Bd_Block[k][j].Size = Bd_Block_allocated[k][j];
			     if(!k)
			      AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[k][j], &ulErr);
			     else
			      AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[k][j], &ulErr);
			  } // End for(j)
		 } // End for(k)
		 CloseHandle(fbHandle);
		 CloseHandle(sbHandle);
         free(wBuff);
		 return;
		} // End if(GetExitCodeThread)
	} // End while()
  } // End if(enable_write)
  else{
   contThreadStatus = 0;
   contRunning = 0;
  }
  
 

  QueryPerformanceCounter(&startTime);
  if(AI64SSA_Start_DmaCmdChaining(ulBdNum, DmaSetup[0].DmaChannel,  &ulErr)){
      ShowAPIError(ulErr);
      SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
      contRunning = 0;
      while (contThreadStatus == 1)
        Sleep(1); 
      for(i=0; i< NUM_BOARDS; i++){
		  for(j=0; j< NUM_BLOCKS; j++){
             Bd_Block[i][j].Size = Bd_Block_allocated[i][j];
			 if(!i)
			  AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[i][j], &ulErr);
			 else{
			  AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[i][j], &ulErr);
			  CloseHandle(myFlags[j]);
			 }
		  }
	  }
      free(wBuff);
      CloseHandle(fbHandle);
      CloseHandle(sbHandle);
      return;
  }
  if(AI64SSA_Start_DmaCmdChaining(ulAuxBdNum, DmaSetup[1].DmaChannel,  &ulErr)){
      ShowAPIError(ulErr);
      SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
      contRunning = 0;
      while (contThreadStatus == 1)
        Sleep(1); 
      AI64SSA_Close_DmaCmdChaining(ulBdNum, DmaSetup[0].DmaChannel,  &ulErr);
      for(i=0; i< NUM_BOARDS; i++){
		  for(j=0; j< NUM_BLOCKS; j++){
             Bd_Block[i][j].Size = Bd_Block_allocated[i][j];
			 if(!i)
			  AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[i][j], &ulErr);
			 else{
			  AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[i][j], &ulErr);
			  CloseHandle(myFlags[j]);
			 }
		  }
	  }
      free(wBuff);
      CloseHandle(fbHandle);
      CloseHandle(sbHandle);
      return;
  }
  j = 0;
  k = 0;
  z = 0.0;
  PutCursor(xPos,yPos);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);         // Enable Clock
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); 

  do{
     
	 EventStatus = WaitForMultipleObjects(2,theHandles,TRUE,4 * 1000); // Wait for the interrupt
	 switch(EventStatus)	
		 {						
	        case WAIT_OBJECT_0:	
			case WAIT_OBJECT_0+1:	
				{

			 if(k > ((smallestSize/4)-128)) // All of the lines to if(enable_write)
		       k=0;						    // are for display management
			 result = ldiv(k,30);		  
             if(!(result.rem)){
			   PutCursor(xPos,yPos);
			   cprintf("                                                                 ");
               PutCursor(xPos,yPos);
			 }
			if(enable_write){
			    if(WaitForSingleObject(myFlags[j],0) == WAIT_OBJECT_0){
					cprintf("Not fast enough - exiting");
					contRunning = 0;
					while (contThreadStatus == 1)
						Sleep(1);
					ok=0;
				   }
				else
				   ReleaseSemaphore(myFlags[j],1,NULL);
				j = (j) ? 0:1;
			} // end enable_write
			else{
 			 cprintf("*");
//          If U want to see the data @ location k or the buffer size value
//            uncomment the next two lines

//			 PutCursor(xPos,yPos);
// 	         cprintf("Data %lx : Buff Size = %06ld    ",*(myData+k),AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE));

			}			
	           z+=1.0;
	           k+=64*4;
			} // end WAIT_OBJECT_0
				break;
			default:
				kbflush();
                PutCursor(xPos,yPos);
				cprintf("Error... Interrupt Timeout\n");
//				do{
                   PutCursor(xPos,yPos+1);
 				   cprintf("Buff Size fB = %ld : sB = %ld",AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE),
					             AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, BUFF_SIZE));
//				}while(!kbhit());


				ok=0;
				break;

	 } // End Switch EventStatus
     QueryPerformanceCounter(&stopTime);
	 elapsedTime.QuadPart = (stopTime.QuadPart - startTime.QuadPart)/sysFreq.QuadPart;
	 
  }while((U32)elapsedTime.QuadPart < sec_to_run && ok);


  contRunning = 0;
  while (contThreadStatus == 1)
    Sleep(1); 
  if(ok && !enable_write)
	  cprintf("\n  No Errors\n  %.1lfMSamples Monitored",(z*(double)smallestSize/2)/1000000.0);
  PutCursor(CurX,19);
  cprintf("Data %s Done                                 \n",(enable_write)?"Collection":"Monitor");
  
  // Stop Cmd Chaining here
  AI64SSA_Close_DmaCmdChaining(ulBdNum, DmaSetup[0].DmaChannel,  &ulErr);
  AI64SSA_Close_DmaCmdChaining(ulAuxBdNum, DmaSetup[1].DmaChannel,  &ulErr);
  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event[0], &ulErr);
  AI64SSA_Cancel_Interrupt_Notify(ulAuxBdNum, &event[1], &ulErr);
  AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
  AI64SSA_DisableInterrupt(ulAuxBdNum, 1, 1, &ulErr);
  data_in = AI64SSA_Read_Local32(ulAuxBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, BCR,data_in & 0xFFFFFF7F);
  for(k=0; k< NUM_BOARDS; k++){
	  for(j=0; j< NUM_BLOCKS; j++){
         Bd_Block[k][j].Size = Bd_Block_allocated[k][j];
		 if(!k)
		  AI64SSA_Free_Physical_Memory(ulBdNum, &Bd_Block[k][j], &ulErr);
		 else{
		  AI64SSA_Free_Physical_Memory(ulAuxBdNum, &Bd_Block[k][j], &ulErr);
          if(enable_write)
		     CloseHandle(myFlags[j]);
		 } // End else
	  } // End for(j)
  } // End for(k)
  free(wBuff);
  CloseHandle(fbHandle);
  CloseHandle(sbHandle);
  AI64SSA_Reset_Device(ulAuxBdNum, &ulErr);
  Sleep(300);
  AI64SSA_Reset_Device(ulBdNum, &ulErr);
  SetThreadPriority(myThread,THREAD_PRIORITY_NORMAL);
  anykey();
  kbflush();
}


// Function to write data to disk when told

DWORD WINAPI
DiskWriteFunction(
    LPVOID pParam
    )
{
  FILE *myFile[1024];
  DWORD EventStatus;
  U32 buff1Size = 0;
  U32 buff2Size = 0;
  U32 buff1Bytes,buff2Bytes;
  U32 setSample1,setSample2;
  U32 samplesWritten = 0;
  char fName[256];
  int filenum = 0;
  int k,l;
  int files_needed;
  double total_written;
  double range;

  contThreadStatus = 1;

  if((myFlags[0] == NULL) || (myFlags[1] == NULL)){ // Setup for two semaphores
	  contThreadStatus = 0;
	  return 1;
  }
  buff1Size = buffSize[0];
  buff2Size = buffSize[1];
  if((buff1Size == 0) || (buff2Size == 0)){
	  contThreadStatus = 0;
	  return 2;
  }
  buff1Bytes = buff1Size*4;
  buff2Bytes = buff2Size*4;
  setSample1 = buff1Size * numBoards;
  setSample2 = buff2Size * numBoards;
  bytes_needed = *((double *)(pParam));
  if(bytes_needed < 3000000000.0)
   files_needed = 1;
  else
   files_needed = ((int)(bytes_needed / 3000000000.0)+1);
  if(files_needed >= 1024)
	  files_needed = 1024;

/* 10 Bytes
   Header Byte
   Sample Rate
   # of Chans
   # of bits
   voltage range  - First byte mantissa - Second byte remainder * 1000
   bipolar / unipolar(=1)
   offset binary / twos complement(=1)
   unpacked / packed data(=1)
   sequential / random(=1)
*/  
  *wBuff = 0xEADE0;
  *(wBuff+1) = sampleRate;
  *(wBuff+2) = dNumChan;
  *(wBuff+3) = 16;
  *(wBuff+5) = (U32)(modf(vRange,&range)*1000.0); // Need remainder here i.e .5 * 1000
  *(wBuff+4) = (U32)range;
  *(wBuff+6) = 0; // bipolar
  *(wBuff+7) = 0; // offset binary
  *(wBuff+8) = dPacked; // unpacked data
  *(wBuff+9) = 0; // Channels in ascending order

  for(k=0; k<files_needed; k++)
  {
  	sprintf(fName,"AIData%04d.bin",k);   
  myFile[k] = fopen(fName,"w+b"); // Doesn't append, always overwrites
  if(myFile[k] == NULL)
  {
	  cprintf("\n  Error opening file for save.. Cannot continue");
	  l=k;
	  for(l=0; l<k; l++)
	  fclose(myFile[l]);	
	  contThreadStatus = 0;
	  return 3;
  }
	fwrite(wBuff,sizeof(U32),10,myFile[k]);// Write the header info
  } // end For()

  total_written = 0.0;

  // Signal that thread is ready
    contThreadStatus = 1;

    while(contRunning){
	   if(samplesWritten > 750000000){
		 total_written += (double)samplesWritten;
   	     if(filenum < 1023)
			 filenum++;
		 samplesWritten = 0;
	   } // End if(samplesWritten...
       EventStatus = WaitForMultipleObjects(2,myFlags,FALSE,10);
	   switch(EventStatus){
			case WAIT_OBJECT_0:
				{
				   memmove(wBuff,Buff[0][0],buff1Bytes); // BYTES
				   if(numBoards > 1)
				    memmove(wBuff+buff1Size,Buff[1][0],buff1Bytes); // BYTES
/*
				   if(numBoards > 2)
				    memmove(wBuff+buff1Size*2,Buff[2][0],buff1Bytes); // BYTES
				   if(numBoards > 3)
				    memmove(wBuff+buff1Size*3,Buff[3][0],buff1Bytes); // BYTES
*/
				   fwrite(wBuff,sizeof(U32),setSample1,myFile[filenum]);
				   samplesWritten += setSample1;
				   cprintf("*");
				}
				break;

			case WAIT_OBJECT_0+1:
				{
				   memmove(wBuff,Buff[0][1],buff2Bytes); // BYTES
				   if(numBoards > 1)
				    memmove(wBuff+buff2Size,Buff[1][1],buff2Bytes); // BYTES
/*
				   if(numBoards > 2)
				    memmove(wBuff+buff2Size*2,Buff[2][1],buff2Bytes); // BYTES
				   if(numBoards > 3)
				    memmove(wBuff+buff2Size*3,Buff[3][1],buff2Bytes); // BYTES
*/				   
				   fwrite(wBuff,sizeof(U32),setSample2,myFile[filenum]);
				   samplesWritten += setSample2;
				   cprintf("+");
				}
				break;
			default: // Do Nothing
				break;

	   } // End switch(EventStatus)

	} // End while(contRunning)

	for(k=0; k<files_needed; k++)
	  fclose(myFile[k]);
	if(filenum == 0)
		total_written = (double)samplesWritten;
    cprintf("\n  %.1lf Samples successfully written ..   \n",total_written);
    contThreadStatus = 0;
 return 0;
}

/*******************************************************************************
This example function uses SGL DMA WITHIN the driver.
It is best used for capturing burst's and small amounts of data.
It free's the user from having to allocate memory and keep track of system resources 
Empirical testing has shown it is good up to ~ 100K sample rate.

For faster sample rates, and/or saving large amounts of data,
the user should consider running DMA on the hardware of the General Standards 
board as shown in the ContinuousSaveData() example function.
*/
//==============================================================================
void MultiBoard_DMA_Example(){

  HANDLE myHandle;
  DWORD EventStatus;
  GS_NOTIFY_OBJECT event;
  U32 mode, ulMode;
  U32 yPos = 4, xPos = 7;
  U32 sampleSize;
  U32 ulBuffSize = 0;
  U32 myData;

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Sgl DMA Mode:");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  cprintf("Please select the desired Data Mode ...");
  PutCursor(CurX,CurY++);
  cprintf("     1 -  Normal");	// Packing only applies to the DMA transfer
  PutCursor(CurX,CurY++);	
  cprintf("     2 -  Packed");
  PutCursor(CurX,CurY++);
  cprintf("Selection: ");
  do
  {
    mode = toupper(getch());
  }while(mode != '1' && mode != '2');

  if (mode == '1')
	ulMode = 0;
  else
	ulMode = 1;
  
  AI64SSA_Dma_DataMode(ulBdNum, ulMode);


  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("DMA Mode:  ");

  AI64SSA_Initialize(ulBdNum, &ulErr);
  AI64SSA_Initialize(ulAuxBdNum, &ulErr);
  Busy_Signal(10);

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ValueRead & 0x7FFFF); // Threshold Mode
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, BCR, ValueRead & 0x7FFFF); // Threshold Mode
  CurY=10;
  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000|((50000000/25000)&0xFFFF)); // 25K Sample
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, RATE_A, 0x10000|((50000000/25000)&0xFFFF)); // 25K Sample
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  sampleSize = 131072 ;
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL,(sampleSize-1)); // Threshold
//  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL,(sampleSize-1)); // Threshold
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  auto_cal(ulAuxBdNum);
  PutCursor(CurX,19);
  cprintf("%c10V Range - SE                         ", 241);
  PutCursor(CurX,20);
  
  AI64SSA_Open_DMA_Channel(ulBdNum, 0, &ulErr);
  if(ulErr){
   cprintf("Error Opening DMA Channel 0\nAny Key to continue..   ");
   AI64SSA_Reset_Device(ulBdNum, &ulErr);
   anykey();
   return;
  }
  AI64SSA_Open_DMA_Channel(ulAuxBdNum, 1, &ulErr);
  if(ulErr){
   cprintf("Error Opening DMA Channel 0\nAny Key to continue..   ");
   AI64SSA_Reset_Device(ulBdNum, &ulErr);
   anykey();
   return;
  }
  myHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (myHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      return;
  }

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A); // Disable Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, data_in|0x10000);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, RATE_A, data_in|0x10000);

  AI64SSA_EnableInterrupt(ulAuxBdNum, 0x10, 0, &ulErr);
  AI64SSA_EnableInterrupt(ulBdNum, 0x10, 0, &ulErr);

  event.hEvent = (U64)myHandle;

  AI64SSA_Register_Interrupt_Notify(ulBdNum, &event, 0, 0, &ulErr);


  memset(Data,0,131072);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock
  AI64SSA_Write_Local32(ulAuxBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock


  do{
     
     EventStatus = WaitForSingleObject(myHandle,2 * 1000); // Wait for the interrupt

     switch(EventStatus)	
	 {						// If packed data, we will
	  case WAIT_OBJECT_0:	// have 2 channels per long word
	   AI64SSA_DMA_FROM_Buffer(ulAuxBdNum, 1, sampleSize, Data, &ulErr);
       if(ulErr){
           ShowAPIError(ulErr);
           ClrScr();
	   }
	   AI64SSA_DMA_FROM_Buffer(ulBdNum, 0, sampleSize, Data, &ulErr);
       if(ulErr){
           ShowAPIError(ulErr);
           ClrScr();
	   }
       memcpy(&myData,Data,sizeof(U32));
	   PutCursor(xPos,yPos);

       ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);
	   // Use this to determine if overrun (If it hits 0x3FFFF we overrun)
	   // Of course, the call takes time, so eliminate if critical
       cprintf("%04X : %08X ", ulBuffSize,myData);
	   break;
	  default:
	    PutCursor(2,9);
		cprintf("\nError... Interrupt Timeout\n");
        AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
        AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
        AI64SSA_Close_DMA_Channel(ulBdNum, 0, &ulErr);
        AI64SSA_Close_DMA_Channel(ulAuxBdNum, 1, &ulErr);
        ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);// the FIFO
        cprintf("Buff Size, %08X : ICR, %08X", ulBuffSize,AI64SSA_Read_Local32(ulBdNum, &ulErr, ICR)); // interrupt. If it hits 0xFFFF we overrun
        CloseHandle(myHandle);
		anykey();
        return;
	 }

  }while(!kbhit());
  kbflush();
  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
  AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
  AI64SSA_Close_DMA_Channel(ulBdNum, 0, &ulErr);
  AI64SSA_Close_DMA_Channel(ulAuxBdNum, 1, &ulErr);
  CloseHandle(myHandle);

  AI64SSA_Reset_Device(ulBdNum, &ulErr);
  AI64SSA_Dma_DataMode(ulBdNum, 0); // Set normal mode 
 
}

/*******************************************************************************
This example function uses PIO mode to simply show how data is organized
In the input data buffer.
********************************************************************************/
//==============================================================================
void Data_Demo(){

  FILE *myFile;
  HANDLE myHandle;
  DWORD EventStatus;
  GS_NOTIFY_OBJECT event;
  U32 mode, ulMode;
  U32 yPos = 4, xPos = 7;
  U32 sampleSize;
  U32 ulBuffSize = 0;
  U32 i,j,k;
  U32 lScan,uScan,marker=0;
  U32 num_markers=0;
  ldiv_t result;

  sampleSize = 512;

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Data Demo Mode:");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  cprintf("Please select the desired Data Mode ...");
  PutCursor(CurX,CurY++);
  cprintf("     1 -  Normal");
  PutCursor(CurX,CurY++);	
  cprintf("     2 -  Packed");
  PutCursor(CurX,CurY++);
  cprintf("Selection: ");
  do
  {
    mode = toupper(getch());
  }while(mode != '1' && mode != '2');

  if (mode == '1')
	ulMode = 0;
  else
	ulMode = 1;
  kbflush();
  if(ulMode){
	  ClrScr();
	  CurX=CurY=2;
	  PutCursor(CurX,CurY++);
	  cprintf("Data Demo Mode:");
	  PutCursor(CurX,CurY++);
	  PutCursor(CurX,CurY++);
	  cprintf("Please select Scan Marking ...");
	  PutCursor(CurX,CurY++);
	  cprintf("     1 -  Off");
	  PutCursor(CurX,CurY++);	
	  cprintf("     2 -  On");
	  PutCursor(CurX,CurY++);
	  cprintf("Selection: ");
	  do
	  {
		mode = toupper(getch());
	  }while(mode != '1' && mode != '2');

	  if (mode == '1')
		marker = 0;
	  else{
		  marker = 1;
		  ClrScr();
		  CurX=CurY=2;
		  PutCursor(CurX,CurY++);
		  cprintf("Data Demo Mode:");
		  PutCursor(CurX,CurY++);
		  PutCursor(CurX,CurY++);
		  cprintf("Please enter the Lower Scan Marker ...");
		  scanf("%lx",&lScan);
		  cprintf("Please enter the Upper Scan Marker ...");
		  scanf("%lx",&uScan);
		  result = ldiv(sampleSize,dNumChan);
		  num_markers = (U32)result.quot;
	  }
  }

  kbflush();
  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Data Demo Mode:");

  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(10);

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }
  ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, BCR);
  if(ulMode)
	  ValueRead |= 0x40000;// Packed data
  if(!marker)
	  ValueRead |= 0x800;// Disable scan marker
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BCR, ValueRead & 0x7FFFF); // Threshold Mode

  if(marker){
	  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_MARKER_L, lScan);
	  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_MARKER_U, uScan);
  }

  
  CurY=10;
  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000|((50000000/25000)&0xFFFF)); // 25K Sample
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);
  if(ulMode && marker)
	sampleSize += num_markers;

  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL,(sampleSize-1)); // Threshold
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  
  myHandle = CreateEvent(NULL,
					     FALSE,
						 FALSE,
						 NULL
						);
  if (myHandle == NULL){
      ulErr = ApiInsufficientResources;
      ShowAPIError(ulErr);
      return;
  }

// Will use PIO mode to simplify the demo
  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A); // Disable Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, data_in|0x10000);

  AI64SSA_EnableInterrupt(ulBdNum, 0x10, 0, &ulErr);

  event.hEvent = (U64)myHandle;

  AI64SSA_Register_Interrupt_Notify(ulBdNum, &event, 0, 0, &ulErr);


  memset(Data,0,256);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock


  EventStatus = WaitForSingleObject(myHandle,2 * 1000); // Wait for the interrupt

  switch(EventStatus)	
  {						// If packed data, we will
   case WAIT_OBJECT_0:	// have 2 channels per long word
	   PutCursor(0,yPos);
       if(ulMode){
	    sampleSize /= 2;
		k=0;
        for(i=0; i<sampleSize; i+=(dNumChan/2)){// Parse the Data
			if(marker){
	         Data[k] = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_BUFF);
			 k++;
			}
			for(j=0; j<(U32)(dNumChan/2); j++){
	         Data[k] = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_BUFF);
             Data[k+1] = ((Data[k] >> 16) & 0xFFFF);
		     Data[k] &= 0xFFFF;
			 k += 2;
			}
			if(marker)
				i += 1;
		}

        for(i=0; i<sampleSize; i+=dNumChan){// Now display the data
            if(marker){
			 cprintf("Marker = %08X\n", Data[i]);
			 i += 1;
			}
			for(j=0; j<(U32)dNumChan; j++)
			 cprintf("Ch%ld = %05X\n", j,Data[i+j]);
		 cprintf("\n");
		}
	   }// End packed mode
	   else{
        for(i=0; i<sampleSize; i+=dNumChan){
			for(j=0; j<(U32)dNumChan; j++){
			 Data[i+j] = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_BUFF);
			 cprintf("Ch%ld = %05X\n", j,Data[i+j]);
			}
		 cprintf("\n");
		}
	   }// End normal mode
	   
	   myFile = fopen("AI64SS.txt","w"); // Doesn't append, always overwrites
       if(myFile == NULL){
	     cprintf("\n  Error opening file for save.. Cannot continue");
	     fclose(myFile);	
         anykey();
         kbflush();
	     return;
	   }
        for(i=0; i<sampleSize; i+=dNumChan){// Write data to file
            if(marker){
			 fprintf(myFile,"Marker = %08X\n", Data[i]);
//		     fwrite(&Data[i],sizeof(U32),1,myFile);
			 i += 1;
			}
			for(j=0; j<(U32)dNumChan; j++)
			 fprintf(myFile,"Ch%ld = %04X\n", j,Data[i+j]);
//		     fwrite(&Data[i+j],sizeof(U32),1,myFile);
		}
	  fclose(myFile);

   break;
   default:
	PutCursor(2,9);
	cprintf("\nError... Interrupt Timeout\n");
    AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
    AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
    ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);// the FIFO
    cprintf("Buff Size, %08X : ICR, %08X", ulBuffSize,AI64SSA_Read_Local32(ulBdNum, &ulErr, ICR)); // interrupt. If it hits 0xFFFF we overrun
	anykey();
    return;
 }
  anykey();
  kbflush();
  AI64SSA_Cancel_Interrupt_Notify(ulBdNum, &event, &ulErr);
  AI64SSA_DisableInterrupt(ulBdNum, 0, 1, &ulErr);
}

/*******************************************************************************
This example function shows the setup for external burst triggering.
********************************************************************************/
//==============================================================================
void Ext_Burst_Demo(){


  U32 burstSize;
  U32 ulBuffSize = 0;

  burstSize = 100;

  if(dNumChan == 0){
    ValueRead = AI64SSA_Read_Local32(ulBdNum, &ulErr, FW_REV);
    dNumChan = (ValueRead&0x10000) ? 32:64;
  }

  ClrScr();
  CurX=CurY=2;
  PutCursor(CurX,CurY++);
  cprintf("Ext Trigger Demo:");
  PutCursor(CurX,CurY++);
  PutCursor(CurX,CurY++);
  kbflush();

  AI64SSA_Initialize(ulBdNum, &ulErr);
  Busy_Signal(10);

  
  CurY=10;
  PutCursor(CurX,21);
  cprintf("Performing AutoCalibration                               ");
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, 0x10000|((50000000/25000)&0xFFFF)); // 25K Sample
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL,(dNumChan==64)?6:5);

  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL,((burstSize*dNumChan)-1)); // Threshold
  AI64SSA_Write_Local32(ulBdNum, &ulErr, BURST_SIZE, burstSize); // Burst Size
  PutCursor(CurX,21);
  auto_cal(ulBdNum);
  
  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, SCAN_CNTRL); // Enable Burst Mode
  AI64SSA_Write_Local32(ulBdNum, &ulErr, SCAN_CNTRL, data_in | 0x200);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A); // Disable Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, data_in|0x10000);

  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
  AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
  indata = AI64SSA_Read_Local32(ulBdNum, &ulErr, RATE_A);
  AI64SSA_Write_Local32(ulBdNum, &ulErr, RATE_A, indata & 0xFFFEFFFF); // Enable Clock
  
  PutCursor(CurX,21);
  cprintf("Input Lo TTL pulse >140nS on I/O pins B40(+)/B39(-) or press key to exit");
  do{
	  if(ulBuffSize > 0x3E6FF){
		  data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL); // Clear Buffer
	      AI64SSA_Write_Local32(ulBdNum, &ulErr, IN_DATA_CNTRL, data_in|0x40000);
		  Sleep(500);
	  }

	 do{
	    Sleep(10);
		ulBuffSize = AI64SSA_Read_Local32(ulBdNum, &ulErr, BUFF_SIZE);
		PutCursor(CurX,CurY);
		cprintf("Buffer Size is %ld              ",ulBuffSize);
        data_in = AI64SSA_Read_Local32(ulBdNum, &ulErr, SCAN_CNTRL);
		}
	    while(!(data_in & 0x80) && !kbhit());

	 Sleep(300);
  }
  while(!kbhit());

  PutCursor(CurX,21);
  cprintf("Remove any connection on I/O pins B40(+)/B39(-)                         \n\n");
  anykey();
  AI64SSA_Initialize(ulBdNum, &ulErr);
  kbflush();
}
