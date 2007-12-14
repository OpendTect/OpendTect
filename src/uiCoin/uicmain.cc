/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.cc,v 1.19 2007-12-14 05:15:23 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uicmain.h"
#include "initsood.h"
#include "initvisbase.h"
#include "initvissurvey.h"
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
	DBG::message( "SoQt::initStdClasses() ..." );
    SoQt::init( widget );

    SoOD::initStdClasses();

    if ( DBG::isOn(DBG_UI) )
	DBG::message( "done. visBase::initStdClasses() ..." );
    visBase::initStdClasses();

    if ( DBG::isOn(DBG_UI) )
	DBG::message( "done. visSurvey::initStdClasses() ..." );
    visSurvey::initStdClasses();

    if ( DBG::isOn(DBG_UI) )
	DBG::message( "done. SoDB::initStdClasses() ..." );
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
