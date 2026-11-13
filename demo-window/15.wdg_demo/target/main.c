#define GLOBAL

#include "Main.h"

uint32 debugflag;



int main( void )
{
	uint32	reset_time;
	uint32	temp;

	debugflag = 0;

	reset_time = 1;
	WDG_SetWatchDog( reset_time );
	SystemClock();
	GPIOInit();
	UartInit(115200,9600);

	if ( POWER_GetResetSource() == POWER_RESET_SOURCE_EXTERNAL )
	{
		printf( "-------power supply wakeup------\n" );
	}else if ( POWER_GetResetSource() == POWER_RESET_SOURCE_WAKE )
	{
		printf( "-------sleep wake up------\n" );
	}else {
		printf( "-------wdog reset wake up------\n" );
	}


	while ( 1 )
	{
		CLOCK_DelayS( 2 );
		WDG_DogFeed();   
	}

	return(0);
}



