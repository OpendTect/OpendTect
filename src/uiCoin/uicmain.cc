/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.13 2004-12-16 10:34:22 bert Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "visinventorinit.h"
#include "debug.h"
#include "debugmasks.h"
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

	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "SoQt::init() ..." );
    SoQt::init(mw);
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done. visBase::initODInventorClasses() ..." );
    visBase::initODInventorClasses();
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done. SoDB::init() ..." );
    SoDB::init();
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "done." );
}

int uicMain::exec()
{
//    toplevel().raise();
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "uicMain::exec(): Entering SoQt::mainLoop()." );
    SoQt::mainLoop();
    return 0;
}
