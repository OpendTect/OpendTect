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

enum RunCtxt
{
    UnknownCtxt,	// Before initialization in main()
    NormalCtxt,		// in od_main
    TestProgCtxt,	// test programs
    SysAdmCtxt,		// system administrator tool
    BatchProgCtxt,	// batch programs
    UiProgCtxt,		// UI program (not od_main, od_sysadmmain, installer)
    InstallerCtxt	// installer
};

mGlobal(Basic) RunCtxt GetRunContext();

mGlobal(Basic) inline bool InNormalRunContext()
{ return GetRunContext() == OD::NormalCtxt; }
mGlobal(Basic) inline bool InTestProgRunContext()
{ return GetRunContext() == OD::TestProgCtxt; }
mGlobal(Basic) inline bool InSysAdmRunContext()
{ return GetRunContext() == OD::SysAdmCtxt; }
mGlobal(Basic) inline bool InBatchProgRunContext()
{ return GetRunContext() == OD::BatchProgCtxt; }
mGlobal(Basic) inline bool InUiProgRunContext()
{ return GetRunContext() == OD::UiProgCtxt; }
mGlobal(Basic) inline bool InInstallerRunContext()
{ return GetRunContext() == OD::InstallerCtxt; }

// Not for you:
mGlobal(Basic) void SetRunContext(RunCtxt);

} // namespace OD
