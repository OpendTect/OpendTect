/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "coordsystem.h"
#include "debug.h"
#include "file.h"
#include "genc.h"
#include "legal.h"
#include "moddepmgr.h"
#include "oscommand.h"
#include "posinfo2dsurv.h"
#include "pythonaccess.h"
#include "sighndl.h"
#include "texttranslator.h"

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

#ifdef __win__
#if _MSC_VER < 1400

    _set_output_format(_TWO_DIGIT_EXPONENT);
    // From MSDN:
    // "is used to configure the output of formatted I/O functions"
#endif
#endif

#ifndef OD_NO_QT
    legalInformation().addCreator( qtLegalText, "Qt" );
#endif

    OD::loadLocalization();
    OD::PythA().initClass();

    if ( !NeedDataBase() )
	return;

    PosInfo::Survey2D::initClass();
    Coords::UnlocatedXY::initClass();
    Coords::AnchorBasedXY::initClass();
}
