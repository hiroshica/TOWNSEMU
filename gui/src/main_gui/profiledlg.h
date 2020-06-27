#ifndef PROFILEDLG_IS_INCLUDED
#define PROFILEDLG_IS_INCLUDED
/* { */

#include "fsgui.h"
#include "fsguifiledialog.h"

#include "townsprofile.h"

class ProfileDialog : public FsGuiDialog
{
public:
	typedef class ProfileDialog THISCLASS;

	enum
	{
		NUM_GAMEPORT_CHOICE=5
	};

	const unsigned int gamePortChoice[NUM_GAMEPORT_CHOICE]=
	{
		TOWNS_GAMEPORTEMU_NONE,
		TOWNS_GAMEPORTEMU_PHYSICAL0,
		TOWNS_GAMEPORTEMU_PHYSICAL1,
		TOWNS_GAMEPORTEMU_KEYBOARD,
		TOWNS_GAMEPORTEMU_MOUSE,

		// Future options.
		//TOWNS_GAMEPORTEMU_PHYSICAL2,
		//TOWNS_GAMEPORTEMU_PHYSICAL3,
		//TOWNS_GAMEPORTEMU_PHYSICAL4,
		//TOWNS_GAMEPORTEMU_PHYSICAL5,
		//TOWNS_GAMEPORTEMU_PHYSICAL6,
		//TOWNS_GAMEPORTEMU_PHYSICAL7,
		//TOWNS_GAMEPORTEMU_ANALOG0,
		//TOWNS_GAMEPORTEMU_ANALOG1,
		//TOWNS_GAMEPORTEMU_ANALOG2,
		//TOWNS_GAMEPORTEMU_ANALOG3,
		//TOWNS_GAMEPORTEMU_ANALOG4,
		//TOWNS_GAMEPORTEMU_ANALOG5,
		//TOWNS_GAMEPORTEMU_ANALOG6,
		//TOWNS_GAMEPORTEMU_ANALOG7,
		//TOWNS_GAMEPORTEMU_PHYSICAL0_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL1_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL2_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL3_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL4_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL5_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL6_AS_CYBERSTICK,
		//TOWNS_GAMEPORTEMU_PHYSICAL7_AS_CYBERSTICK,
	};

	FsGuiButton *ROMDirBtn,*CDImgBtn,*FDImgBtn[TownsProfile::NUM_STANDBY_FDIMG],*HDImgBtn[TownsProfile::MAX_NUM_SCSI_DEVICE];
	FsGuiTextBox *ROMDirTxt,*CDImgTxt,*FDImgTxt[TownsProfile::NUM_STANDBY_FDIMG],*HDImgTxt[TownsProfile::MAX_NUM_SCSI_DEVICE];
	FsGuiButton *gamePortBtn[2][NUM_GAMEPORT_CHOICE]; // None, Pad0, Pad1, Keybord Emulation, Mouse,
	FsGuiButton *bootKeyBtn[15];
	FsGuiButton *autoStartBtn;
	FsGuiButton *runBtn;

	void Make(void);
	virtual void OnButtonClick(FsGuiButton *btn);
	void OnSelectROMFile(FsGuiDialog *dlg,int returnCode);

	FsGuiTextBox *nowBrowsingTxt=nullptr;
	void Browse(const wchar_t label[],FsGuiTextBox *txt,const wchar_t ext0[],const wchar_t ext1[]);
	void OnSelectFile(FsGuiDialog *dlg,int returnCode);

	TownsProfile GetProfile(void) const;
	void SetProfile(const TownsProfile &profile);
};


/* } */
#endif