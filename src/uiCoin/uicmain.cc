/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.20 2007-12-21 09:38:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicmain.h"

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
	DBG::message( "uicMain::init()" );
    SoQt::init( widget );
    SoDB::init();
}


int uicMain::exec()
{
    if ( DBG::isOn(DBG_UI) )
	DBG::message( "uicMain::exec(): Entering SoQt::mainLoop()." );
    SoQt::mainLoop();
    return 0;
}
