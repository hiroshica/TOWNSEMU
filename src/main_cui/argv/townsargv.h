#ifndef TOWNSARGV_IS_INCLUDED
#define TOWNSARGV_IS_INCLUDED
/* { */

#include "townsdef.h"

class TownsARGV
{
public:
	std::string ROMPath;
	std::string fdImgFName[2];
	std::string startUpScriptFName;

	TownsARGV();
	void PrintHelp(void) const;
	bool AnalyzeCommandParameter(int argc,char *argv[]);
};


/* } */
#endif