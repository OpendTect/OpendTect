#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2018
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
