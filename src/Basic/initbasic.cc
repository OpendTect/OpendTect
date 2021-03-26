/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "coordsystem.h"
#include "debug.h"
#include "file.h"
#include "genc.h"
#include "legal.h"
#include "moddepmgr.h"
#include "posinfo2dsurv.h"
#include "pythonaccess.h"
#include "sighndl.h"

#ifdef __win__
# include <stdio.h> // for _set_output_format
#endif

#define OD_EXT_KEYSTR_EXPAND 1

#include "keystrs.h"
#ifndef OD_NO_QT
# include <QtGlobal>

static uiString* qtLegalText()
{
    return new uiString(toUiString(
	    "The Qt GUI Toolkit ("
	    QT_VERSION_STR
	    ") is Copyright (C) The Qt Company Ltd.\n"
	    "Contact: http://www.qt.io/licensing\n\n"
	    "Qt is available under the LGPL\n\n%1" ).arg( lgplV3Text() ) );
}
#endif


mDefModInitFn(Basic)
{
    mIfNotFirstTime( return );

    SignalHandling::initClass();
    CallBack::initClass();

    //Remove all residual affinity (if any) from inherited
    //process. This could be if this process is started through
    //forking from a process that had affinity set.
    Threads::setCurrentThreadProcessorAffinity(-1);

    System::CrashDumper::getInstance().setSendAppl(
		System::CrashDumper::sSenderAppl() );

    legalInformation().addCreator( qtLegalText, "Qt" );
    OD::PythonAccess::initClass();

    if ( !NeedDataBase() )
	return;

    PosInfo::Survey2D::initClass();
    Coords::UnlocatedXY::initClass();
    Coords::AnchorBasedXY::initClass();
}
