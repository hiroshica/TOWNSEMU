/* LICENSE>>
Copyright 2020 Soji Yamakawa (CaptainYS, http://www.ysflight.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

<< LICENSE */
#include <iostream>

#include "physmem.h"
#include "ramrom.h"
#include "cpputil.h"
#include "towns.h"
#include "townsdef.h"
#include "i486.h"
#include "i486debug.h"


void TownsPhysicalMemory::KanjiROMAccess::Reset()
{
	JISCodeHigh=0;
	JISCodeLow=0;
	row=0;
}


////////////////////////////////////////////////////////////


void TownsPhysicalMemory::State::Reset(void)
{
	sysRomMapping=true;
	dicRom=false;
	DICROMBank=0;
	FMRVRAM=true; // [2] pp.91
	FMRVRAMMask=0x0F; // [2] pp.159
	FMRVRAMWriteOffset=0;
	TVRAMWrite=false;
	ANKFont=false;
	kanjiROMAccess.Reset();

	for(auto &c : RAM)
	{
		c=0;
	}
	for(auto &c : VRAM)
	{
		c=0;
	}
	for(auto &c : CVRAM)
	{
		c=0;
	}
	for(auto &c : spriteRAM)
	{
		c=0;
	}
	for(auto &c : waveRAM)
	{
		c=0;
	}
}


////////////////////////////////////////////////////////////


/* virtual */ void TownsPhysicalMemory::IOWriteByte(unsigned int ioport,unsigned int data)
{
	if(TOWNSIO_CMOS_BASE<=ioport && ioport<TOWNSIO_CMOS_END)
	{
		/* I'll try again.
		if(true==preventCMOSInitToSingleDriveMode)
		{
			if(0x328C==ioport)
			{
				std::cout << "Blocking Single Drive Mode " << cpputil::Ubtox(data) << "->" << "00H" << std::endl;
				data=0;
			}
			if(0x33CE==ioport)
			{
				std::cout << "Blocking Single Drive Mode " << cpputil::Ubtox(data) << "->" << "FAH" << std::endl;
				data=0xFA;
			}
		} */
		state.CMOSRAM[(ioport-TOWNSIO_CMOS_BASE)/2]=(unsigned char)(data&0xFF);
		return;
	}

	switch(ioport)
	{
	case TOWNSIO_FMR_VRAM_OR_MAINRAM: // 0x404
		SetFMRVRAMMappingFlag((0x80&data)==0);
		break;
	case TOWNSIO_SYSROM_DICROM: // 0x480
		SetSysROMDicROMMappingFlag(0==(data&2),0!=(data&1));
		break;
	case TOWNSIO_DICROM_BANK://              0x484, // [2] pp.92
		state.DICROMBank=data&0x0F;
		break;
	case TOWNSIO_FMR_VRAMMASK: // 0xFF81
		state.FMRVRAMMask=data;
		break;
	case TOWNSIO_FMR_VRAMDISPLAYMODE:
		FMRVRAMAccess.crtcPtr->MEMIOWriteFMRVRAMDisplayMode(data);
		break;
	case TOWNSIO_FMR_VRAMPAGESEL:
		state.FMRVRAMWriteOffset=(0!=(data&0x10) ? 0x40000 : 0);
		break;

	case TOWNSIO_VRAMACCESSCTRL_ADDR: //      0x458, // [2] pp.17,pp.112
		state.nativeVRAMMaskRegisterLatch=data&1; // [2] pp.112 suggests lower 2 bits are effectve, but actually just 1.
		break;
	case TOWNSIO_VRAMACCESSCTRL_DATA_LOW: //  0x45A, // [2] pp.17,pp.112
		{
			auto prevMask=cpputil::GetDword(state.nativeVRAMMask);
			state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)  ]=data&255;
			state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)+4]=data&255;
			auto newMask=cpputil::GetDword(state.nativeVRAMMask);
			if(prevMask!=newMask && (0xffffffff==prevMask || 0xffffffff==newMask))
			{
				EnableOrDisableNativeVRAMMask();
			}
		}
		break;
	case TOWNSIO_VRAMACCESSCTRL_DATA_HIGH: // 0x45B, // [2] pp.17,pp.112
		{
			auto prevMask=cpputil::GetDword(state.nativeVRAMMask);
			state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)+1]=data&255;
			state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)+5]=data&255;
			auto newMask=cpputil::GetDword(state.nativeVRAMMask);
			if(prevMask!=newMask && (0xffffffff==prevMask || 0xffffffff==newMask))
			{
				EnableOrDisableNativeVRAMMask();
			}
		}
		break;
	}
}
/* virtual */ unsigned int TownsPhysicalMemory::IOReadByte(unsigned int ioport)
{
	if(TOWNSIO_CMOS_BASE<=ioport && ioport<TOWNSIO_CMOS_END)
	{
		return state.CMOSRAM[(ioport-TOWNSIO_CMOS_BASE)/2];
	}

	unsigned char data;
	switch(ioport)
	{
	case TOWNSIO_FMR_VRAM_OR_MAINRAM: // 0x404
		return (true==state.FMRVRAM ? 0 : 0x80);
	case TOWNSIO_SYSROM_DICROM: // 0x480
		{
			unsigned char byteData=0;
			if(true!=state.sysRomMapping)
			{
				byteData|=2;
			}
			if(true==state.dicRom)
			{
				byteData|=1;
			}
			return byteData;
		}
		break;
	case TOWNSIO_DICROM_BANK://              0x484, // [2] pp.92
		data=state.DICROMBank;
		break;
	case TOWNSIO_TVRAM_WRITE:
		data=(state.TVRAMWrite ? 0xff : 0x00);
		state.TVRAMWrite=false;
		break;
	case TOWNSIO_MEMSIZE:
		return (unsigned int)(state.RAM.size()/(1024*1024));
	case TOWNSIO_FMR_VRAMMASK: // 0xFF81
		return state.FMRVRAMMask;
	case TOWNSIO_VRAMACCESSCTRL_ADDR: //      0x458, // [2] pp.17,pp.112
		return state.nativeVRAMMaskRegisterLatch;
	case TOWNSIO_VRAMACCESSCTRL_DATA_LOW: //  0x45A, // [2] pp.17,pp.112
		return state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)  ];
	case TOWNSIO_VRAMACCESSCTRL_DATA_HIGH: // 0x45B, // [2] pp.17,pp.112
		return state.nativeVRAMMask[(state.nativeVRAMMaskRegisterLatch<<1)+1];
	}
	return data;
}

TownsPhysicalMemory::TownsPhysicalMemory(class FMTowns *townsPtr,class i486DX *cpuPtr,class Memory *memPtr,class RF5C68 *pcmPtr) : Device(townsPtr),waveRAMAccess(pcmPtr)
{
	takeJISCodeLog=false;
	this->cpuPtr=cpuPtr;
	this->memPtr=memPtr;

	for(auto &b : state.CMOSRAM)
	{
		b=0;
	}

	// Just took from my 2MX.
	unsigned char defSerialROM[SERIAL_ROM_LENGTH]=
	{
		0x04,0x65,0x54,0xA4,0x95,0x45,0x35,0x5F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0x0C,0x02,0x00,0x00,0x00,0x15,0xE0,0x00,0x00,
	};
	for(unsigned int i=0; i<SERIAL_ROM_LENGTH; ++i)
	{
		serialROM[i]=defSerialROM[i];
	}
	state.Reset();
}

bool TownsPhysicalMemory::LoadROMImages(const char dirName[])
{
	std::string fName;
	fName=cpputil::MakeFullPathName(dirName,"FMT_SYS.ROM");
	sysRom=cpputil::ReadBinaryFile(fName);
	if(256*1024!=sysRom.size())
	{
		Abort("Cannot read FMT_SYS.ROM or incorrect file size.");
		return false;
	}

	fName=cpputil::MakeFullPathName(dirName,"FMT_DOS.ROM");
	dosRom=cpputil::ReadBinaryFile(fName);
	if(512*1024!=dosRom.size())
	{
		Abort("Cannot read FMT_DOS.ROM or incorrect file size.");
		return false;
	}

	fName=cpputil::MakeFullPathName(dirName,"FMT_FNT.ROM");
	fontRom=cpputil::ReadBinaryFile(fName);
	if(256*1024!=fontRom.size())
	{
		Abort("Cannot read FMT_FNT.ROM or incorrect file size.");
		return false;
	}

	fName=cpputil::MakeFullPathName(dirName,"FMT_F20.ROM");
	font20Rom=cpputil::ReadBinaryFile(fName);
	if(512*1024!=font20Rom.size())
	{
		Abort("Cannot read FMT_F20.ROM or incorrect file size.");
		return false;
	}

	fName=cpputil::MakeFullPathName(dirName,"FMT_DIC.ROM");
	dicRom=cpputil::ReadBinaryFile(fName);
	if(512*1024!=dicRom.size())
	{
		Abort("Cannot read FMT_DIC.ROM or incorrect file size.");
		return false;
	}

	fName=cpputil::MakeFullPathName(dirName,"MYTOWNS.ROM");
	auto data=cpputil::ReadBinaryFile(fName);
	if(SERIAL_ROM_LENGTH<=data.size())
	{
		for(int i=0; i<SERIAL_ROM_LENGTH; ++i)
		{
			serialROM[i]=data[i];
		}
	}

	return true;
}

void TownsPhysicalMemory::SetCMOS(const std::vector <unsigned char> &cmos)
{
	for(unsigned int i=0; i<TOWNS_CMOS_SIZE && i<cmos.size(); ++i)
	{
		state.CMOSRAM[i]=cmos[i];
	}
}

void TownsPhysicalMemory::SetMainRAMSize(long long int size)
{
	size&=0xFFF00000;
	if(size<0x100000) // Minimum 1MB.
	{
		size=0x100000;
	}
	state.RAM.resize(size);
}

void TownsPhysicalMemory::SetVRAMSize(long long int size)
{
	state.VRAM.resize(size);
}

void TownsPhysicalMemory::SetCVRAMSize(long long int size)
{
	state.CVRAM.resize(size);
}

void TownsPhysicalMemory::SetSpriteRAMSize(long long int size)
{
	state.spriteRAM.resize(size);
}

void TownsPhysicalMemory::SetWaveRAMSize(long long int size)
{
	state.waveRAM.resize(size);
}

/* virtual */ void TownsPhysicalMemory::Reset(void)
{
	state.Reset();
	SetSysROMDicROMMappingFlag(state.sysRomMapping,state.dicRom);
	SetFMRVRAMMappingFlag(state.FMRVRAM);

}

void TownsPhysicalMemory::SetUpMemoryAccess(void)
{
	auto &mem=*memPtr;
	auto &cpu=*cpuPtr;

	mainRAMAccess.SetPhysicalMemoryPointer(this);
	mainRAMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&mainRAMAccess,0x00000000,0x000FFFFF);

	FMRVRAMAccess.SetPhysicalMemoryPointer(this);
	FMRVRAMAccess.SetCPUPointer(&cpu);
	SetFMRVRAMMappingFlag(true);  // This will set up memory access for 0xC0000 to 0xCFFFF

	mappedDicROMandDicRAMAccess.SetPhysicalMemoryPointer(this);
	mappedDicROMandDicRAMAccess.SetCPUPointer(&cpu);

	nativeDicROMAccess.SetPhysicalMemoryPointer(this);
	nativeDicROMAccess.SetCPUPointer(&cpu);
	nativeCMOSRAMAccess.SetPhysicalMemoryPointer(this);
	nativeCMOSRAMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&nativeDicROMAccess,TOWNSADDR_NATIVE_DICROM_BASE,TOWNSADDR_NATIVE_DICROM_END-1);
	mem.AddAccess(&nativeCMOSRAMAccess,TOWNSADDR_NATIVE_CMOSRAM_BASE,TOWNSADDR_NATIVE_CMOSRAM_END-1);

	mappedSysROMAccess.SetPhysicalMemoryPointer(this);
	mappedSysROMAccess.SetCPUPointer(&cpu);
	SetSysROMDicROMMappingFlag(true,false);   // This will set up memory access for 0xF8000 to 0xFFFFF and 0xD0000 to 0xDFFFF

	if(0x00100000<state.RAM.size())
	{
		mem.AddAccess(&mainRAMAccess,0x00100000,(unsigned int)state.RAM.size()-1);
	}

	VRAMAccess0.SetPhysicalMemoryPointer(this);
	VRAMAccess0.SetCPUPointer(&cpu);
	VRAMAccess1.SetPhysicalMemoryPointer(this);
	VRAMAccess1.SetCPUPointer(&cpu);
	VRAMAccessHighRes0.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes0.SetCPUPointer(&cpu);
	VRAMAccessHighRes1.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes1.SetCPUPointer(&cpu);
	VRAMAccessHighRes2.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes2.SetCPUPointer(&cpu);

	VRAMAccessWithMask0.SetPhysicalMemoryPointer(this);
	VRAMAccessWithMask0.SetCPUPointer(&cpu);
	VRAMAccessWithMask1.SetPhysicalMemoryPointer(this);
	VRAMAccessWithMask1.SetCPUPointer(&cpu);
	VRAMAccessWithMaskHighRes0.SetPhysicalMemoryPointer(this);
	VRAMAccessWithMaskHighRes0.SetCPUPointer(&cpu);
	VRAMAccessWithMaskHighRes1.SetPhysicalMemoryPointer(this);
	VRAMAccessWithMaskHighRes1.SetCPUPointer(&cpu);
	VRAMAccessWithMaskHighRes2.SetPhysicalMemoryPointer(this);
	VRAMAccessWithMaskHighRes2.SetCPUPointer(&cpu);

	VRAMAccess0Debug.SetPhysicalMemoryPointer(this);
	VRAMAccess0Debug.SetCPUPointer(&cpu);
	VRAMAccess1Debug.SetPhysicalMemoryPointer(this);
	VRAMAccess1Debug.SetCPUPointer(&cpu);
	VRAMAccessHighRes0Debug.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes0Debug.SetCPUPointer(&cpu);
	VRAMAccessHighRes1Debug.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes1Debug.SetCPUPointer(&cpu);
	VRAMAccessHighRes2Debug.SetPhysicalMemoryPointer(this);
	VRAMAccessHighRes2Debug.SetCPUPointer(&cpu);
	SetUpVRAMAccess(false,false);

	spriteRAMAccess.SetPhysicalMemoryPointer(this);
	spriteRAMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&spriteRAMAccess,TOWNSADDR_SPRITERAM_BASE,TOWNSADDR_SPRITERAM_END-1);

	osROMAccess.SetPhysicalMemoryPointer(this);
	osROMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&osROMAccess,0xC2000000,0xC207FFFF);

	fontROMAccess.SetPhysicalMemoryPointer(this);
	fontROMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&fontROMAccess,0xC2100000,0xC213FFFF);

	waveRAMAccess.SetPhysicalMemoryPointer(this);
	waveRAMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&waveRAMAccess,TOWNSADDR_WAVERAM_WINDOW_BASE,TOWNSADDR_WAVERAM_WINDOW_END-1);

	sysROMAccess.SetPhysicalMemoryPointer(this);
	sysROMAccess.SetCPUPointer(&cpu);
	mem.AddAccess(&sysROMAccess,0xFFFC0000,0xFFFFFFFF);
}

void TownsPhysicalMemory::SetUpVRAMAccess(bool breakOnRead,bool breakOnWrite)
{
	auto &mem=*memPtr;
	if(true!=breakOnRead && true!=breakOnWrite)
	{
		mem.AddAccess(&VRAMAccess0,TOWNSADDR_VRAM0_BASE,TOWNSADDR_VRAM0_END-1);
		mem.AddAccess(&VRAMAccess1,TOWNSADDR_VRAM1_BASE,TOWNSADDR_VRAM1_END-1);
		mem.AddAccess(&VRAMAccessHighRes0,TOWNSADDR_VRAM_HIGHRES0_BASE,TOWNSADDR_VRAM_HIGHRES0_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes1,TOWNSADDR_VRAM_HIGHRES1_BASE,TOWNSADDR_VRAM_HIGHRES1_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes2,TOWNSADDR_VRAM_HIGHRES2_BASE,TOWNSADDR_VRAM_HIGHRES2_END-1); // For IIMX High Resolution Access.
	}
	else
	{
		VRAMAccess0Debug.breakOnRead=breakOnRead;
		VRAMAccess0Debug.breakOnWrite=breakOnWrite;
		VRAMAccess1Debug.breakOnRead=breakOnRead;
		VRAMAccess1Debug.breakOnWrite=breakOnWrite;
		VRAMAccessHighRes0Debug.breakOnRead=breakOnRead;
		VRAMAccessHighRes0Debug.breakOnWrite=breakOnWrite;
		VRAMAccessHighRes1Debug.breakOnRead=breakOnRead;
		VRAMAccessHighRes1Debug.breakOnWrite=breakOnWrite;
		VRAMAccessHighRes2Debug.breakOnRead=breakOnRead;
		VRAMAccessHighRes2Debug.breakOnWrite=breakOnWrite;
		mem.AddAccess(&VRAMAccess0Debug,TOWNSADDR_VRAM0_BASE,TOWNSADDR_VRAM0_END-1);
		mem.AddAccess(&VRAMAccess1Debug,TOWNSADDR_VRAM1_BASE,TOWNSADDR_VRAM1_END-1);
		mem.AddAccess(&VRAMAccessHighRes0Debug,TOWNSADDR_VRAM_HIGHRES0_BASE,TOWNSADDR_VRAM_HIGHRES0_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes1Debug,TOWNSADDR_VRAM_HIGHRES1_BASE,TOWNSADDR_VRAM_HIGHRES1_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes2Debug,TOWNSADDR_VRAM_HIGHRES2_BASE,TOWNSADDR_VRAM_HIGHRES2_END-1); // For IIMX High Resolution Access.
	}
}

void TownsPhysicalMemory::SetSysROMDicROMMappingFlag(bool sysRomMapping,bool dicRomMapping)
{
	// The interpretation of 0480H is very difficult.
	// On startup system rom does:
	//	MOV	DX,0480H
	//	IN	AL,DX
	//	OR	AL,2
	//	OUT DX,AL
	// To disable system-ROM mapping (active low) before RAM test.  However, since it keeps bit0, if I interpret bit 0
	// as controlling Dictionary ROM and CMOS RAM mapping to D0000H to DFFFFH, it doesn't clear Dictionary ROM and CMOS RAM
	// mapping, and then destroys CMOS RAM during the memory test.
	//
	// Only interpretation that makes sense to me is:
	//   0480H Bit1(active low)  Bit0         D0000     F8000
	//         1(false)          0(false)     MainRAM   MainRAM
	//         1(false)          1(true)      MainRAM   MainRAM
	//         0(true)           0(false)     SysROM    MainRAM
	//         0(true)           1(true)      SysROM    DicROM/CMOS
	//
	// I'll eventually test on the actual machine.

	state.sysRomMapping=sysRomMapping;
	state.dicRom=dicRomMapping;

	if(true==sysRomMapping && true==dicRomMapping)
	{
		memPtr->AddAccess(&mappedSysROMAccess,TOWNSADDR_SYSROM_MAP_BASE,TOWNSADDR_SYSROM_MAP_END-1);
		memPtr->AddAccess(&mappedDicROMandDicRAMAccess,TOWNSADDR_FMR_DICROM_BASE,TOWNSADDR_BACKUP_RAM_END-1);
	}
	else if(true==sysRomMapping && true!=dicRomMapping)
	{
		memPtr->AddAccess(&mappedSysROMAccess,TOWNSADDR_SYSROM_MAP_BASE,TOWNSADDR_SYSROM_MAP_END-1);
		memPtr->AddAccess(&mainRAMAccess,TOWNSADDR_FMR_DICROM_BASE,TOWNSADDR_BACKUP_RAM_END-1);
	}
	else
	{
		memPtr->AddAccess(&mainRAMAccess,TOWNSADDR_SYSROM_MAP_BASE,TOWNSADDR_SYSROM_MAP_END-1);
		memPtr->AddAccess(&mainRAMAccess,TOWNSADDR_FMR_DICROM_BASE,TOWNSADDR_BACKUP_RAM_END-1);
	}
}
void TownsPhysicalMemory::SetFMRVRAMMappingFlag(bool FMRVRAMMapping)
{
	state.FMRVRAM=FMRVRAMMapping;
	if(true==FMRVRAMMapping)
	{
		memPtr->AddAccess(&FMRVRAMAccess,TOWNSADDR_FMR_VRAM_BASE,TOWNSADDR_FMR_VRAM_CVRAM_FONT_END-1);
	}
	else
	{
		memPtr->AddAccess(&mainRAMAccess,TOWNSADDR_FMR_VRAM_BASE,TOWNSADDR_FMR_VRAM_CVRAM_FONT_END-1);
	}
}

void TownsPhysicalMemory::EnableOrDisableNativeVRAMMask(void)
{
	auto &mem=*memPtr;
	if(0xffffffff==cpputil::GetDword(state.nativeVRAMMask))
	{
		mem.AddAccess(&VRAMAccess0,TOWNSADDR_VRAM0_BASE,TOWNSADDR_VRAM0_END-1);
		mem.AddAccess(&VRAMAccess1,TOWNSADDR_VRAM1_BASE,TOWNSADDR_VRAM1_END-1);
		mem.AddAccess(&VRAMAccessHighRes0,TOWNSADDR_VRAM_HIGHRES0_BASE,TOWNSADDR_VRAM_HIGHRES0_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes1,TOWNSADDR_VRAM_HIGHRES1_BASE,TOWNSADDR_VRAM_HIGHRES1_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessHighRes2,TOWNSADDR_VRAM_HIGHRES2_BASE,TOWNSADDR_VRAM_HIGHRES2_END-1); // For IIMX High Resolution Access.
	}
	else
	{
		mem.AddAccess(&VRAMAccessWithMask0,TOWNSADDR_VRAM0_BASE,TOWNSADDR_VRAM0_END-1);
		mem.AddAccess(&VRAMAccessWithMask1,TOWNSADDR_VRAM1_BASE,TOWNSADDR_VRAM1_END-1);
		mem.AddAccess(&VRAMAccessWithMaskHighRes0,TOWNSADDR_VRAM_HIGHRES0_BASE,TOWNSADDR_VRAM_HIGHRES0_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessWithMaskHighRes1,TOWNSADDR_VRAM_HIGHRES1_BASE,TOWNSADDR_VRAM_HIGHRES1_END-1); // For IIMX High Resolution Access.
		mem.AddAccess(&VRAMAccessWithMaskHighRes2,TOWNSADDR_VRAM_HIGHRES2_BASE,TOWNSADDR_VRAM_HIGHRES2_END-1); // For IIMX High Resolution Access.
	}
}

std::vector <std::string> TownsPhysicalMemory::GetStatusText(void) const
{
	std::vector <std::string> text;
	std::string empty;

	text.push_back(empty);
	text.back()="C0000-CFFFF:";
	if(true==state.FMRVRAM)
	{
		text.back()+="FMR VRAM";
	}
	else
	{
		text.back()+="Main RAM";
	}
	text.back()+="  FMR VRAM Mask(000CFF81H)="+cpputil::Ubtox(state.FMRVRAMMask);

	text.push_back(empty);
	text.back()="D0000-EFFFF:";
	if(true==state.dicRom)
	{
		text.back()+="Dictionary RAM/ROM, User Font RAM";
	}
	else
	{
		text.back()+="Main RAM";
	}
	text.back()+="  Dictionary Bank:";
	text.back()+=cpputil::Uitox(state.DICROMBank);

	text.push_back(empty);
	text.back()="FF000-FFFFF:";
	if(true==state.sysRomMapping)
	{
		text.back()+="Boot ROM";
	}
	else
	{
		text.back()+="Main RAM";
	}

	text.push_back("");
	text.back()="Native VRAM Mask:";
	text.back()+=cpputil::Uitox(cpputil::GetDword(state.nativeVRAMMask));

	return text;
}
