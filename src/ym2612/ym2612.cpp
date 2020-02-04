#include "ym2612.h"



void YM2612::State::PowerOn(void)
{
	Reset();
}
void YM2612::State::Reset(void)
{
	deviceTimeInNS=0;
	lastTickTimeInNS=0;
	for(auto &r : reg)
	{
		r=0;
	}
	for(auto &t : timerCounter)
	{
		t=0;
	}
	for(auto &b : timerUp)
	{
		b=false;
	}
}

////////////////////////////////////////////////////////////

YM2612::YM2612()
{
	PowerOn();
}
YM2612::~YM2612()
{
}
void YM2612::PowerOn(void)
{
	state.PowerOn();
}
void YM2612::Reset(void)
{
	state.Reset();
}
void YM2612::WriteRegister(unsigned int reg,unsigned int value)
{
	state.reg[reg&255]=value;
	if(REG_TIMER_CONTROL==reg)
	{
		if(value&1) // Load Timer A
		{
			unsigned int countHigh=state.reg[REG_TIMER_A_COUNT_HIGH];
			unsigned int countLow=state.reg[REG_TIMER_A_COUNT_LOW];
			auto count=(countHigh<<2)|(countLow&3);
			state.timerCounter[0]=count*TIMER_A_PER_TICK;
		}
		if(value&2) // Load Timer B
		{
			state.timerCounter[1]=(unsigned int)(state.reg[REG_TIMER_B_COUNT])*TIMER_B_PER_TICK;
		}
		if(value&4) // Enable Timer A Flag
		{
		}
		if(value&8) // Enable Timer B Flag
		{
		}
		if(value&0x10) // Reset Timer A Flag
		{
			state.timerUp[0]=false;
		}
		if(value&0x20) // Reset Timer B Flag
		{
			state.timerUp[1]=false;
		}
	}
}
unsigned int YM2612::ReadRegister(unsigned int reg) const
{
	return state.reg[reg&255];
}
void YM2612::Run(unsigned long long int systemTimeInNS)
{
	if(0==state.deviceTimeInNS)
	{
		state.lastTickTimeInNS=systemTimeInNS;
		state.deviceTimeInNS=systemTimeInNS;
		return;
	}
	if(state.lastTickTimeInNS+TICK_DURATION_IN_NS<systemTimeInNS)
	{
		auto nTick=(systemTimeInNS-state.lastTickTimeInNS)/TICK_DURATION_IN_NS;
		state.lastTickTimeInNS+=nTick*TICK_DURATION_IN_NS;
		if(state.timerCounter[0]<NTICK_TIMER_A)
		{
			state.timerCounter[0]+=nTick;
			if(NTICK_TIMER_A<=state.timerCounter[0] &&
			   0!=(state.reg[REG_TIMER_CONTROL]&0x04))
			{
				state.timerUp[0]=true;
			}
		}
		if(state.timerCounter[1]<NTICK_TIMER_B)
		{
			state.timerCounter[1]+=nTick;
			if(NTICK_TIMER_B<=state.timerCounter[1] &&
			   0!=(state.reg[REG_TIMER_CONTROL]&0x08))
			{
				state.timerUp[1]=true;
			}
		}
	}


	state.deviceTimeInNS=systemTimeInNS;
}
bool YM2612::TimerAUp(void) const
{
	return state.timerUp[0];
}
bool YM2612::TimerBUp(void) const
{
	return state.timerUp[1];
}
bool YM2612::TimerUp(unsigned int timerId) const
{
	switch(timerId&1)
	{
	default:
	case 0:
		return TimerAUp();
	case 1:
		return TimerBUp();
	}
}
