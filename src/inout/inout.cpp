/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#include <iostream>

#include "cpputil.h"
#include "inout.h"



InOut::InOut()
{
	takeLog=false;
	ioMap.resize(NUM_IO_ADDR);
	for(auto &devPtr : ioMap)
	{
		devPtr=nullptr;
	}
}
void InOut::EnableLog(void)
{
	takeLog=true;
}
void InOut::DisableLog(void)
{
	takeLog=false;
}
void InOut::ClearLog(void)
{
	log.clear();
}

void InOut::AddDevice(Device *devPtr,unsigned int minIOPort,unsigned int maxIOPort)
{
	for(auto port=minIOPort; port<NUM_IO_ADDR && port<=maxIOPort; ++port)
	{
		ioMap[port]=devPtr;
	}
}

void InOut::AddDevice(Device *devPtr,unsigned int ioPort)
{
	if(ioPort<NUM_IO_ADDR)
	{
		ioMap[ioPort]=devPtr;
	}
}

unsigned int InOut::In8(unsigned int port)
{
	unsigned int value=0xff;

	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		value=ioMap[port]->IOReadByte(port);
	}

	// Read from appropriate device..
	if(true==takeLog)
	{
		IOLog l;
		l.output=false;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
	return value;
}

unsigned int InOut::In16(unsigned int port)
{
	unsigned int value=0xffff;

	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		value=ioMap[port]->IOReadWord(port);
	}

	if(true==takeLog)
	{
		IOLog l;
		l.output=false;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
	return value;
}

unsigned int InOut::In32(unsigned int port)
{
	unsigned int value=0xffffffff;

	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		value=ioMap[port]->IOReadDword(port);
	}

	if(true==takeLog)
	{
		IOLog l;
		l.output=false;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
	return value;
}

void InOut::Out8(unsigned int port,unsigned int value)
{
	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		ioMap[port]->IOWriteByte(port,value);
	}

	if(true==takeLog)
	{
		IOLog l;
		l.output=true;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
}
void InOut::Out16(unsigned int port,unsigned int value)
{
	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		ioMap[port]->IOWriteWord(port,value);
	}

	if(true==takeLog)
	{
		IOLog l;
		l.output=true;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
}
void InOut::Out32(unsigned int port,unsigned int value)
{
	if(port<NUM_IO_ADDR && nullptr!=ioMap[port])
	{
		ioMap[port]->IOWriteDword(port,value);
	}

	if(true==takeLog)
	{
		IOLog l;
		l.output=true;
		l.port=port;
		l.value=value;
		log.push_back(l);
	}
}
