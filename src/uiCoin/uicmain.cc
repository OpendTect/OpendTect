/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.18 2007-10-12 19:14:34 cvskris Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "visinit.h"
#include "vissurvinit.h"
#include "debug.h"
#include "debugmasks.h"
#include "envvars.h"
#include <Inventor/Qt/SoQt.h>
#include <Inventor/SoDB.h>

uicMain::uicMain( int& argc, char** argv )
    : uiMain( argc, argv )
{}


void uicMain::init( QWidget* widget )
{
    if ( !GetOSEnvVar("SOQT_BRIL_X11_SILENCER_HACK") )
	SetEnvVar( "SOQT_BRIL_X11_SILENCER_HACK", "1" );
    if ( !GetOSEnvVar("COIN_FULL_INDIRECT_RENDERING") )
	SetEnvVar( "COIN_FULL_INDIRECT_RENDERING", "1" );

	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "SoQt::init() ..." );
    SoQt::init( widget );
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done. visBase::init() ..." );
    visBase::init();
	    DBG::message( "done. visSurvey::init() ..." );
    visSurvey::init();
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done. SoDB::init() ..." );
    SoDB::init();
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done." );
}

int uicMain::exec()
{
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "uicMain::exec(): Entering SoQt::mainLoop()." );
    SoQt::mainLoop();
    return 0;
}
