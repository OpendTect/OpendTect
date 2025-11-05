#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"


namespace OD
{

enum class RunCtxt
{
    UnknownCtxt,	// Before initialization in main()
    NormalCtxt,		// in od_main
    StandAloneCtxt,	// Other main apps (often from plugins)
    TestProgCtxt,	// test programs
    SysAdmCtxt,		// system administrator tool
    BatchProgCtxt,	// batch programs
    BatchProgTestCtxt,	// batch test programs
    UiProgCtxt,		// UI utility program (not od_main or alike, installer)
    InstallerCtxt	// installer
};

mGlobal(Basic) RunCtxt GetRunContext();
mGlobal(Basic) bool GetQuietFlag();

mGlobal(Basic) bool InNormalRunContext();
mGlobal(Basic) bool InStandAloneRunContext();
mGlobal(Basic) bool InTestProgRunContext();
mGlobal(Basic) bool InSysAdmRunContext();
mGlobal(Basic) bool InBatchProgRunContext();
mGlobal(Basic) bool InBatchProgTestRunContext();
mGlobal(Basic) bool InUiProgRunContext();
mGlobal(Basic) bool InInstallerRunContext();

// Not for you:
mGlobal(Basic) void SetRunContext(RunCtxt);
mGlobal(Basic) void SetQuietFlag(int argc,char** argv);

} // namespace OD
