#ifndef SCSI_IS_INCLUDED
#define SCSI_IS_INCLUDED
/* { */

#include <vector>
#include <string>

class TownsSCSI : public Device
{
private:
	class FMTowns *townsPtr;
public:
	virtual const char *DeviceName(void) const{return "SCSI";}

	enum
	{
		COMMAND_REQUEST_INTERVAL=500,
	};

	enum
	{
		PHASE_BUSFREE,
		PHASE_ARBITRATION,
		PHASE_SELECTION,
		PHASE_RESELECTION,
		PHASE_MESSAGE_OUT,
		PHASE_MESSAGE_IN,
		PHASE_COMMAND,
		PHASE_DATA_IN,
		PHASE_DATA_OUT,
		PHASE_STATUS,
	};
	enum
	{
		SCSIDEVICE_NONE,
		SCSIDEVICE_HARDDISK,
		SCSIDEVICE_CDROM,
	};
	enum
	{
		MAX_NUM_SCSIDEVICES=7,
		MAX_NUM_COMMAND_BYTES=64, // ?
	};

	// [9] 7.2 Command descriptor block
	enum
	{
		SCSICMD_INQUIRY=0x12,
		// When adding a support for command, don't forget to add commandLength[]= in the constructor.
	};

	// [9] Table 27 - Status byte code
	enum
	{
		STATUSCODE_GOOD                      =0,
		STATUSCODE_CHECK_CONDITION           =0x02,
		STATUSCODE_CONDITION_MET             =0x04,
		STATUSCODE_BUSY                      =0x08,
		STATUSCODE_INTERMEDIATE              =0x10,
		STATUSCODE_INTERMEDIATE_CONDITION_MET=0x14,
		STATUSCODE_RESERVATION_CONFLICT      =0x18,
		STATUSCODE_COMMAND_TERMINATED        =0x22,
		STATUSCODE_QUEUE_FULL                =0x28,
	};



	unsigned int commandLength[256];

	class SCSIDevice
	{
	public:
		unsigned int devType=SCSIDEVICE_NONE;
		std::string imageFName;
		long long int imageSize=0;
	};

	class State
	{
	public:
		SCSIDevice dev[MAX_NUM_SCSIDEVICES];

		unsigned int nCommandFilled=0;
		unsigned char commandBuffer[MAX_NUM_COMMAND_BYTES];

		bool REQ,I_O,MSG,C_D,BUSY,INT,PERR;
		bool DMAE,SEL,ATN,IMSK,WEN;

		unsigned int selId;
		unsigned int phase=PHASE_BUSFREE;
		unsigned int lastDataByte=0;

		void PowerOn(void);
		void Reset(void);
	};

	State state;

	TownsSCSI(class FMTowns *townsPtr);

	virtual void PowerOn(void);
	virtual void Reset(void);

	bool LoadHardDiskImage(unsigned int scsiId,std::string fName);

	static std::string PhaseToStr(unsigned int phase);

	inline bool IRQEnabled(void)
	{
		// [2] tells IMSK meahs:
		//    true:  IRQ disabled
		//    false: IRQ enabled
		// It disagrees with the BIOS disassembly.
		return state.IMSK;
	}

	void SetUpIO_MSG_CDfromPhase(void);
	void EnterSelectionPhase(void);
	void EnterCommandPhase(void);

	virtual void IOWriteByte(unsigned int ioport,unsigned int data);

	virtual unsigned int IOReadByte(unsigned int ioport);

	virtual void RunScheduledTask(unsigned long long int townsTime);

	void ProcessPhaseData(unsigned int dataByte);

	std::vector <std::string> GetStatusText(void) const;
};

/* } */
#endif