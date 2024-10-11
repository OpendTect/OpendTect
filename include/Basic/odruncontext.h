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
    UiProgCtxt,		// UI utility program (not od_main or alike, installer)
    InstallerCtxt	// installer
};

mGlobal(Basic) RunCtxt GetRunContext();

mGlobal(Basic) inline bool InNormalRunContext()
{ return GetRunContext() == OD::RunCtxt::NormalCtxt; }
mGlobal(Basic) inline bool InStandAloneRunContext()
{ return GetRunContext() == OD::RunCtxt::StandAloneCtxt; }
mGlobal(Basic) inline bool InTestProgRunContext()
{ return GetRunContext() == OD::RunCtxt::TestProgCtxt; }
mGlobal(Basic) inline bool InSysAdmRunContext()
{ return GetRunContext() == OD::RunCtxt::SysAdmCtxt; }
mGlobal(Basic) inline bool InBatchProgRunContext()
{ return GetRunContext() == OD::RunCtxt::BatchProgCtxt; }
mGlobal(Basic) inline bool InUiProgRunContext()
{ return GetRunContext() == OD::RunCtxt::UiProgCtxt; }
mGlobal(Basic) inline bool InInstallerRunContext()
{ return GetRunContext() == OD::RunCtxt::InstallerCtxt; }

// Not for you:
mGlobal(Basic) void SetRunContext(RunCtxt);

} // namespace OD
