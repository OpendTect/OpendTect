/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodmain.cc,v 1.2 2003-12-24 15:15:50 bert Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"
#include "uicmain.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"
#include "uisurvey.h"
#include "uimsg.h"
#include "ioman.h"
#include "iodir.h"
#include "odsessionfact.h"

static const int cCTHeight = 200;


uiODMain* ODMainWin()
{
    static uiODMain* theinst = 0;
    if ( !theinst )
	theinst = new uiODMain(0);
    return theinst;
}


uiODMain::uiODMain( int argc, char **argv )
	: uiMainWin(0,"OpendTect Main Window",3,true,true)
    	, uiapp(*new uicMain(argc,argv))
	, failed(true)
    	, ctabed(0)
    	, ctabwin(0)
    	, lastsession(*new ODSession)
    	, lastsession(0)
{
    setIcon( dtect_xpm_data, "OpendTect" );
    uiMSG().setMainWin( this );
    uiapp.setTopLevel( this );

    if ( !ensureGoodDataDir() )
	::exit( 0 );

    applman = new uiODApplMgr( this );

    if ( !ensureGoodSurveySetup() )
	::exit( 0 );

    if ( buildUI() )
	failed = false;
}


uiODMain::~uiODMain()
{
    delete ctabed;
    delete &uiapp;
    delete &lastsession;
}


bool uiODMain::ensureGoodDataDir()
{
    if ( !GetDataDir() )
    {
	uiMSG().message(
	"OpendTect needs a place to store its data.\n"
	"You have not yet specified a location for this datastore,\n"
	"and there is no 'DTECT_DATA or dGB_DATA' set in your environment.\n\n"
	"Please specify where the OpendTect Data Directory should\n"
	"be created or select an existing OpendTect data directory."
#ifndef __win__
	"\n\nNote that you can still put surveys and "
	"individual cubes on other disks;\nbut this is where the "
	"'base' data store will be."
#endif
	);

	BufferString dirnm = GetPersonalDir();
	while ( true )
	{
	    uiFileDialog dlg( p, uiFileDialog::DirectoryOnly, dirnm, "*;*.*",
			      "Specify the directory for the OpendTect data" );
	    if ( !dlg.go() )
		return false;
	    dirnm = dlg.fileName();

	    if ( !File_exists(dirnm) )
	    {
		uiMSG().error( "Selected directory does not exist" );
		continue;
	    }
	    else if ( !File_isDirectory(dirnm) )
	    {
		uiMSG().error( "Please select a directory" );
		continue;
	    }

	    BufferString omfnm = File_getFullPath( dirnm, ".omf" );
	    if ( File_exists(omfnm) ) break;

	    BufferString oddatdir = File_getFullPath( dirnm, "OpendTectData" );
	    if ( File_exists(oddatdir) )
	    {
		if ( !File_isDirectory( oddatdir ) )
		{
		    BufferString msg( "A file \"" );
		    msg += oddatdir; 
		    msg += "\" exists, but it is not a directory.";
		    uiMSG().error( "" );
		    continue;
		}
	    }
	    else if ( !File_createDir( oddatdir, 0 ) )
	    {
		BufferString msg = "Could not create directory \"";
		msg += oddatdir;
		msg += "\"";
		uiMSG().error( msg );
		return false;
	    }

	    dirnm = oddatdir; 

	    omfnm = File_getFullPath( dirnm, ".omf" );
	    if ( File_exists(omfnm) ) break;

	    BufferString datomf( GetDataFileName(".omf") );
	    if ( !File_exists(datomf) )
	    {
		BufferString msg ( "Source .omf file \"" ); 
		msg += datomf;
		msg += "\" does not exist.";
		msg += "\nCannot create OpendTect Data directory without it.";
		uiMSG().error( msg );
		continue;
	    }

	    if ( !File_exists(omfnm) )
		File_copy( datomf, omfnm, NO );
	    if ( !File_exists(omfnm) )
	    {
		uiMSG().error( "Couldn't write on selected directory" );
		continue;
	    }
	    if ( !getenv( "DTECT_DEMO_SURVEY") ) break;

	    const BufferString surveynm( getenv( "DTECT_DEMO_SURVEY" ) );
	    const BufferString todir( File_getFullPath( dirnm,
				      File_getFileName(surveynm) ) );

	    if ( !File_isDirectory(surveynm) || File_exists(todir) ) break;

	    File_copy( surveynm, todir, YES );

	    break;
	}

	Settings::common().set( "Default DATA directory", dirnm );
    }

    return true;
}


bool uiODMain::ensureGoodSurveySetup()
{
    BufferString errmsg;
    if ( !IOMan::validSurveySetup(errmsg) )
    {
	cerr << errmsg << endl;
	uiMSG().error( errmsg );
	return false;
    }
    else if ( IOM().dirPtr()->key() == MultiID("-1") )
    {
	while ( !applman->manageSurvey() )
	{
	    if ( uiMSG().askGoOn( "No survey selected. Do you wish to quit?" ) )
		return false;
	}
    }

    return true;
}


bool uiODMain::buildUI()
{
    menumgr = new uiODMenuMgr( this );
    scenemgr = new uiODSceneMgr( this );
    posctrl = new ui3DApplPosCtrl( this );
    initCT();
    return true;
}


void uiODMain::initCT()
{
    ctabwin = new uiDockWin( this, "Color Table" );
    moveDockWindow( *ctabwin, uiMainWin::Left );
    ctabwin->setResizeEnabled( true );
   
    ctabed = new uiVisColTabEd( ctabwin );
    ctabed->setPrefHeight( cCTHeight );
    ctabed->attach(hCentered);
}


void uiODMain::enableColorTable( bool yn )
{
    ctabwin->setSensitive( yn );
}


IOPar& uiODMain::sessionPars()
{
    return cursession->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt.forread = restore;
    uiIOObjSelDlg dlg( &appl, *ctio );
    if ( !dlg.go() )
	{ delete ctio.ioobj; delete ctio; ctio = 0; }
    return ctio;
}


void uiODMain::saveSession()
{
    PtrMan<CtxtIOObj> ctio = getUserSessionIOData( false );
    ODSession sess; cursession = &sess;
    if ( !ctio || !updateSession() ) return;
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    lastsession = sess; cursession = &lastsession;
}


void uiODMain::restoreSession()
{
    PtrMan<CtxtIOObj> ctio = getUserSessionIOData( true );
    if ( !ctio ) return;
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession = &sess;
    restoreSession();
    cursession = &lastsession; lastsession.clear();
    updateSession();
}


bool uiODMain::hasSessionChanged()
{
    ODSession sess;
    cursession = &sess;
    updateSession();
    cursession = &lastsession;
    return !( sess == lastsession );
}


bool uiODMain::updateSession()
{
    cursession->clear();
    visserv->fillPar( cursession->vispars() );
    attrserv->fillPar( cursession->attrpars() );
    sceneMgr().fillPar( cursession->scenepars() );
    if ( nlaserv && !nlaserv->fillPar( cursession->nlapars() ) ) 
	return false;
    sessionSave.trigger();
    return true;
}


void uiODMain::restoreSession()
{
    sceneMgr().cleanUp( false );
    if ( nlaserv ) nlaserv->reset();
    delete attrserv; attrserv = new uiAttribPartServer( applservice );

    if ( nlaserv ) nlaserv->usePar( cursession->nlapars() );
    attrserv->usePar( cursession->attrpars() );
    bool visok = visserv->usePar( cursession->vispars() );

    if ( visok ) 
	sceneMgr().mkScenesFrom( cursession );
    else
    {
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
	sceneMgr().cleanUp( true );
    }
    sessionRestore.trigger();
}


bool uiODMain::go()
{
    if ( failed ) return false;

    show();
    uiSurvey::updateViewsGlobal();
    int rv = uiapp.exec();
    delete applman; applman = 0;
    return rv ? false : true;
}


bool uiODApplMgr::closeOk( CallBacker* )
{
    if ( failed ) return true;

    menumgr->storePositions();
    scenemgr->storePositions();
    ctabwin->storePosition();

    int res = hasSessionChanged()
	    ? uiMSG().askGoOnAfter( "Do you want to save this session?" );
            : (int)!uiMSG().askGoOn( "Do you want to quit?" ) + 1;

    if ( res == 0 )
	saverestoreSession( false );
    else if ( res == 2 )
	return false;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOk(0) ) return;

    uiapp.exit(0);
}
