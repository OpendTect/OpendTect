/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.12 2004-03-05 10:17:26 dgb Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "visinventorinit.h"
#include "debug.h"
#include "genc.h"
#include <Inventor/Qt/SoQt.h>
#include <Inventor/SoDB.h>

uicMain::uicMain(int argc,char** argv)
: uiMain( argc, argv )
{}

void uicMain::init( QWidget* mw )
{
    if ( !getenv("SOQT_BRIL_X11_SILENCER_HACK") )
	setEnvVar( "SOQT_BRIL_X11_SILENCER_HACK", "1" );
    if ( !getenv("COIN_FULL_INDIRECT_RENDERING") )
	setEnvVar( "COIN_FULL_INDIRECT_RENDERING", "1" );

	if ( DBG::isOn() )
	    DBG::message( "SoQt::init() ..." );
    SoQt::init(mw);
	if ( DBG::isOn() )
	    DBG::message( "done. visBase::initODInventorClasses() ..." );
    visBase::initODInventorClasses();
	if ( DBG::isOn() )
	    DBG::message( "done. SoDB::init() ..." );
    SoDB::init();
	if ( DBG::isOn() )
	    DBG::message( "done." );
}

int uicMain::exec()
{
//    toplevel().raise();
	if ( DBG::isOn() )
	    DBG::message( "uicMain::exec(): Entering SoQt::mainLoop()." );
    SoQt::mainLoop();
    return 0;
}
