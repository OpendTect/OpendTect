/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.cc,v 1.58 2006-11-21 14:00:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "uicrdevenv.h"
#include "uifilebrowser.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodhelpmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uisettings.h"
#include "uisoviewer.h"
#include "uitoolbar.h"
#include "uivispartserv.h"

#include "dirlist.h"
#include "envvars.h"
#include "filegen.h"
#include "oddirs.h"
#include "pixmap.h"
#include "timer.h"
#include "ioman.h"


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
	: appl(*a)
    	, timer(*new Timer("popup timer"))
    	, helpmgr(0)
{
    filemnu = new uiPopupMenu( &appl, "&File" );
    procmnu = new uiPopupMenu( &appl, "&Processing" );
    winmnu = new uiPopupMenu( &appl, "&Windows" );
    viewmnu = new uiPopupMenu( &appl, "&View" );
    utilmnu = new uiPopupMenu( &appl, "&Utilities" );
    helpmnu = new uiPopupMenu( &appl, "&Help" );

    dtecttb = new uiToolBar( &appl, "OpendTect tools" );
    dtecttb->setVerticallyStretchable( true );
    cointb = new uiToolBar( &appl, "Graphical tools" );
    cointb->setVerticallyStretchable( true );
    mantb = new uiToolBar( &appl, "Manage data" );
    mantb->setVerticallyStretchable( true );

    appl.applMgr().visServer()->createToolBars();
//TODO: do not remove!! Helene for 2D/3D  
//    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete dtecttb;
    delete cointb;
    delete mantb;
    delete helpmgr;
}


void uiODMenuMgr::initSceneMgrDepObjs()
{
    uiMenuBar* menubar = appl.menuBar();
    fillFileMenu();	menubar->insertItem( filemnu );
    fillProcMenu();	menubar->insertItem( procmnu );
    fillWinMenu();	menubar->insertItem( winmnu );
    fillViewMenu();	menubar->insertItem( viewmnu );
    fillUtilMenu();	menubar->insertItem( utilmnu );
    menubar->insertSeparator();
    helpmgr = new uiODHelpMenuMgr( this );
    menubar->insertItem( helpmnu );

    fillDtectTB();
    fillCoinTB();
    fillManTB();

    timer.tick.notify( mCB(this,uiODMenuMgr,timerCB) );
}


uiPopupMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu :
	  (at == uiODApplMgr::Exp ? expmnu : manmnu);
}


uiPopupMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus[(int)ot] : expmnus[(int)ot];
}


void uiODMenuMgr::storePositions()
{
    dtecttb->storePosition();
    cointb->storePosition();
    mantb->storePosition();
}


void uiODMenuMgr::updateStereoMenu()
{
    uiSoViewer::StereoType type = 
			(uiSoViewer::StereoType)sceneMgr().getStereoType();
    stereooffitm->setChecked( type == uiSoViewer::None );
    stereoredcyanitm->setChecked( type == uiSoViewer::RedCyan );
    stereoquadbufitm->setChecked( type == uiSoViewer::QuadBuffer );
    stereooffsetitm->setEnabled( type != uiSoViewer::None );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    cointb->turnOn( viewid, isview );
    cointb->turnOn( actid, !isview );
    cointb->setSensitive( axisid, isview );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{
    cointb->turnOn( axisid, shwaxis );
}


bool uiODMenuMgr::isSoloModeOn() const
{
    return cointb->isOn(soloid);
}


void uiODMenuMgr::enableMenuBar( bool yn )
{
    appl.menuBar()->setSensitive( yn );
}


void uiODMenuMgr::enableActButton( bool yn )
{
    cointb->setSensitive( actid, yn );
}


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick)), id )

void uiODMenuMgr::fillFileMenu()
{
    mInsertItem( filemnu, "&Survey ...", mManSurveyMnuItm );

    uiPopupMenu* sessionitm = new uiPopupMenu( &appl, "S&ession");
    mInsertItem( sessionitm, "&Save ...", mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    filemnu->insertItem( sessionitm );
    filemnu->insertSeparator();

    impmnu = new uiPopupMenu( &appl, "&Import" );
    uiPopupMenu* impseis = new uiPopupMenu( &appl, "&Seismics" );
    uiPopupMenu* imphor = new uiPopupMenu( &appl, "&Horizons" );
    uiPopupMenu* impfault = new uiPopupMenu( &appl, "&Faults" );
    uiPopupMenu* impwell = new uiPopupMenu( &appl, "&Wells" );
    impmnu->insertItem( impseis );
    impmnu->insertItem( imphor );
#ifdef __debug__
    impmnu->insertItem( impfault );
    mInsertItem( impfault, "&Landmark ...", mImpLmkFaultMnuItm );
#endif
    impmnu->insertItem( impwell );
    mInsertItem( impmnu, "&Picksets ...", mImpPickMnuItm );
    mInsertItem( impmnu, "&Wavelets ...", mImpWvltMnuItm );

    uiPopupMenu* impseissgy = new uiPopupMenu( &appl, "&SEG-Y" );
    mInsertItem( impseissgy, "&3-D ...", mImpSeisSEGY3DMnuItm );
    mInsertItem( impseissgy, "&2-D ...", mImpSeisSEGY2DMnuItm );
    if ( GetEnvVarYN("OD_SHOW_PRESTACK_SEGY_IMP") )
	mInsertItem( impseissgy, "&Pre-Stack ...", mImpSeisSEGYPSMnuItm );
    impseis->insertItem( impseissgy );
    mInsertItem( impseis, "&CBVS ...", mImpSeisCBVSMnuItm );

    uiPopupMenu* imphorasc = new uiPopupMenu( &appl, "&Ascii" );
    mInsertItem( imphorasc, "&Geometry ...", mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, "&Attributes ...", mImpHorAsciiAttribMnuItm );
    imphor->insertItem( imphorasc );

    uiPopupMenu* impwellasc = new uiPopupMenu( &appl, "&Ascii" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );

    expmnu = new uiPopupMenu( &appl, "&Export" );
    uiPopupMenu* expseis = new uiPopupMenu( &appl, "&Seismics" );
    uiPopupMenu* expseissgy = new uiPopupMenu( &appl, "&SEG-Y" );
    mInsertItem( expseissgy, "&3-D ...", mExpSeisSEGY3DMnuItm );
    mInsertItem( expseissgy, "&2-D ...", mExpSeisSEGY2DMnuItm );
    expseis->insertItem( expseissgy );
    uiPopupMenu* exphor = new uiPopupMenu( &appl, "&Horizons" );
    expmnu->insertItem( expseis );
    expmnu->insertItem( exphor );
    mInsertItem( exphor, "&Ascii ...", mExpHorAsciiMnuItm );
    mInsertItem( expmnu, "&Picksets ...", mExpPickMnuItm );

    filemnu->insertItem( impmnu );
    filemnu->insertItem( expmnu );
    manmnu = new uiPopupMenu( &appl, "&Manage");

    mInsertItem( manmnu, "&AttributeSets ...", mManAttrMnuItm );
#ifdef __debug__
    mInsertItem( manmnu, "&Faults ...", mManFaultMnuItm );
#endif
    mInsertItem( manmnu, "&Horizons ...", mManHorMnuItm );
    mInsertItem( manmnu, "&Seismics ...", mManSeisMnuItm );
    mInsertItem( manmnu, "&Wells ...", mManWellMnuItm );
    mInsertItem( manmnu, "&PickSets ...", mManPickMnuItm );
    mInsertItem( manmnu, "&Wavelets ...", mManWvltMnuItm );
    filemnu->insertItem( manmnu );

    filemnu->insertSeparator();
    mInsertItem( filemnu, "E&xit", mExitMnuItm );

    impmnus.allowNull();
    impmnus += impseis; impmnus += imphor; impmnus += impfault; 
    impmnus += impwell; impmnus+= 0;
    expmnus.allowNull();
    expmnus += expseis; expmnus += exphor; expmnus+=0; expmnus+=0; expmnus+=0;
}


void uiODMenuMgr::fillProcMenu()
{
    mInsertItem( procmnu, "&Attributes ...", mEditAttrMnuItm );
    procmnu->insertSeparator();
    mInsertItem( procmnu, "&Create Seismic output ...", mCreateVolMnuItm );
    
    //mInsertItem( procmnu, "Create &Surface output ...", mCreateSurfMnuItm );
    uiPopupMenu* horitm = 
		new uiPopupMenu( &appl, "Create output using &Horizon ...");
    mInsertItem( horitm, "&Horizon grid...", mCreateSurfMnuItm );
    mInsertItem( horitm, "&Between horizons ...", mCompBetweenHorMnuItm );
    mInsertItem( horitm, "&Horizon slice...", mCompAlongHorMnuItm );
    procmnu->insertItem( horitm );
    
    mInsertItem( procmnu, "&Re-Start ...", mReStartMnuItm );
}


void uiODMenuMgr::fillWinMenu()
{
    mInsertItem( winmnu, "&New", mAddSceneMnuItm );
    mInsertItem( winmnu, "&Cascade", mCascadeMnuItm );
    uiPopupMenu* tileitm = new uiPopupMenu( &appl, "&Tile" );
    winmnu->insertItem( tileitm );

    mInsertItem( tileitm, "&Auto", mTileAutoMnuItm );
    mInsertItem( tileitm, "&Horizontal", mTileHorMnuItm );
    mInsertItem( tileitm, "&Vertical", mTileVerMnuItm );
}


void uiODMenuMgr::fillViewMenu()
{
    mInsertItem( viewmnu, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu, "&Z-scale ...", mZScaleMnuItm );
    uiPopupMenu* stereoitm = new uiPopupMenu( &appl, "&Stereo viewing" );
    viewmnu->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id,idx) \
    itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id, idx ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm, "&Off", true, mStereoOffMnuItm, 0 )
    mInsertStereoItem( stereoredcyanitm, "&Red/Cyan", false,
	    		mStereoRCMnuItm, 1 )
    mInsertStereoItem( stereoquadbufitm, "&Quad buffer", false,
	    		mStereoQuadMnuItm, 2 )

    stereooffsetitm = new uiMenuItem( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm, mStereoOffsetMnuItm, 3 );
    stereooffsetitm->setEnabled( false );

    mkViewIconsMnu();

    viewmnu->insertSeparator();
    viewmnu->insertItem( &appl.createDockWindowMenu() );
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dl( GetDataFileName(0), DirList::DirsOnly, "icons.*" );
    if ( dl.size() < 2 )
	return;

    uiPopupMenu* iconsmnu = new uiPopupMenu( &appl, "&Icons" );
    viewmnu->insertItem( iconsmnu );
    mInsertItem( iconsmnu, "&Default", mViewIconsMnuItm+0 );
    int nradded = 0;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get( idx ).buf() + 6 );
	if ( nm.isEmpty() || nm == "Default" )
	    continue;

	nradded++;
	BufferString mnunm( "&" ); mnunm += nm;
	mInsertItem( iconsmnu, mnunm, mViewIconsMnuItm+nradded );
    }
}


extern const char* logMsgFileName();

void uiODMenuMgr::fillUtilMenu()
{
    settmnu = new uiPopupMenu( &appl, "&Settings" );
    utilmnu->insertItem( settmnu );
    mInsertItem( settmnu, "&Fonts ...", mSettFontsMnuItm );
    mInsertItem( settmnu, "&Mouse controls ...", mSettMouseMnuItm );
    mInsertItem( settmnu, "&Look and feel ...", mSettLkNFlMnuItm );
    mInsertItem( settmnu, "&Shortcuts ...", mSettShortcutsMnuItm );
    mInsertItem( settmnu, "&General ...", mSettGeneral );

    mInsertItem( utilmnu, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( utilmnu, "&Plugins ...", mPluginsMnuItm );
    mInsertItem( utilmnu, "&Create Devel. Env. ...", mCrDevEnvMnuItm );
    const char* lmfnm = logMsgFileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu, "Show &log file ...", mShwLogFileMnuItm );
}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, mCB(&applMgr(),uiODApplMgr,fn), txt, togg )

void uiODMenuMgr::fillDtectTB()
{
    mAddTB(dtecttb,"survey.png","Survey setup",false,manSurvCB);
    mAddTB(dtecttb,"attributes.png","Edit attributes",false,manAttrCB);
    mAddTB(dtecttb,"out_vol.png","Create seismic output",false,outVolCB);
}

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm , mCB(this,uiODMenuMgr,fn), txt, togg )


void uiODMenuMgr::fillManTB()
{
    mAddTB(mantb,"man_seis.png","Manage seismic data",false,manSeis);
    mAddTB(mantb,"man_hor.png","Manage horizons",false,manHor);
#ifdef __debug__
    mAddTB(mantb,"man_flt.png","Manage faults",false,manFlt);
#endif
    mAddTB(mantb,"man_wll.png","Manage well data",false,manWll);
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, mCB(&sceneMgr(),uiODSceneMgr,fn), txt, togg )

void uiODMenuMgr::fillCoinTB()
{
    viewid = mAddTB(cointb,"view.png","View mode",true,viewMode);
    actid = mAddTB(cointb,"pick.png","Interact mode",true,actMode);
    mAddTB(cointb,"home.png","To home position",false,toHomePos);
    mAddTB(cointb,"set_home.png","Save home position",false,saveHomePos);
    mAddTB(cointb,"view_all.png","View all",false,viewAll);
    cameraid = mAddTB(cointb,"perspective.png","Switch to orthographic camera",
	       false,switchCameraType);
    mAddTB(cointb,"cube_inl.png","view Inl",false,viewInl);
    mAddTB(cointb,"cube_crl.png","view Crl",false,viewCrl);
    mAddTB(cointb,"cube_z.png","view Z",false,viewZ);
    axisid = mAddTB(cointb,"axis.png","Display rotation axis",true,showRotAxis);
    mAddTB(cointb,"snapshot.png","Make snapshot",false,mkSnapshot);
    soloid = mAddTB(cointb,"solo.png","Display current element only",
		    true,soloMode);

    cointb->turnOn( actid, true );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    cointb->setToolTip( cameraid, perspective ? "Switch to orthographic camera"
					      : "Switch to perspective camera");
    const char* fnm = perspective ? "perspective.png" : "orthographic.png";
    cointb->setPixmap( cameraid, ioPixmap(fnm) );
}


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODMenuMgr::handleClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return; // Huh?

    const int id = itm->id();
    switch( id )
    {
    case mManSurveyMnuItm: 	applMgr().manageSurvey(); break;
    case mSessSaveMnuItm: 	appl.saveSession(); break;
    case mSessRestMnuItm: 	{ appl.restoreSession(); 
				  timer.start(200,true);  break; }
    case mImpSeisSEGY3DMnuItm:	mDoOp(Imp,Seis,0); break;
    case mImpSeisSEGY2DMnuItm:	mDoOp(Imp,Seis,1); break;
    case mImpSeisSEGYPSMnuItm:	mDoOp(Imp,Seis,2); break;
    case mImpSeisCBVSMnuItm: 	mDoOp(Imp,Seis,3); break;
    case mExpSeisSEGY3DMnuItm: 	mDoOp(Exp,Seis,0); break;
    case mExpSeisSEGY2DMnuItm: 	mDoOp(Exp,Seis,1); break;
    case mExpSeisSEGYPSMnuItm: 	mDoOp(Exp,Seis,2); break;
    case mImpHorAsciiMnuItm: 	mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm: mDoOp(Imp,Hor,1); break;
    case mExpHorAsciiMnuItm: 	mDoOp(Exp,Hor,0); break;
    case mImpWellAsciiTrackMnuItm: mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm: mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm: mDoOp(Imp,Wll,2); break;
    case mImpPickMnuItm: 	mDoOp(Imp,Pick,0); break;
    case mExpPickMnuItm: 	mDoOp(Exp,Pick,0); break;
    case mImpWvltMnuItm: 	mDoOp(Imp,Wvlt,0); break;
    case mImpLmkFaultMnuItm: 	applMgr().importLMKFault(); break;

    case mManSeisMnuItm: 	mDoOp(Man,Seis,0); break;
    case mManHorMnuItm: 	mDoOp(Man,Hor,0); break;
    case mManFaultMnuItm: 	mDoOp(Man,Flt,0); break;
    case mManWellMnuItm: 	mDoOp(Man,Wll,0); break;
    case mManPickMnuItm: 	mDoOp(Man,Pick,0); break;
    case mManWvltMnuItm: 	mDoOp(Man,Wvlt,0); break;
    case mManAttrMnuItm:	mDoOp(Man,Attr,0); break;
    case mManNLAMnuItm:		mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:	mDoOp(Man,Sess,0); break;

    case mExitMnuItm: 		appl.exit(); break;

    case mEditAttrMnuItm: 	applMgr().editAttribSet(); break;
    case mCreateVolMnuItm: 	applMgr().createVol(); break;
    case mCreateSurfMnuItm: 	applMgr().createHorOutput(0); break;
    case mCompAlongHorMnuItm: 	applMgr().createHorOutput(1); break;
    case mCompBetweenHorMnuItm:	applMgr().createHorOutput(2); break;
    case mReStartMnuItm: 	applMgr().reStartProc(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(); break;
    case mCascadeMnuItm: 	sceneMgr().cascade(); break;
    case mTileAutoMnuItm: 	sceneMgr().tile(); break;
    case mTileHorMnuItm: 	sceneMgr().tileHorizontal(); break;
    case mTileVerMnuItm: 	sceneMgr().tileVertical(); break;
    case mWorkAreaMnuItm: 	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm: 	applMgr().setZScale(); break;
    case mBatchProgMnuItm: 	applMgr().batchProgs(); break;
    case mPluginsMnuItm: 	applMgr().pluginMan(); break;
    case mCrDevEnvMnuItm: 	uiCrDevEnv::crDevEnv(&appl); break;
    case mShwLogFileMnuItm: 	showLogFile(); break;
    case mSettFontsMnuItm: 	applMgr().setFonts(); break;
    case mSettMouseMnuItm: 	sceneMgr().setKeyBindings(); break;

    case mSettLkNFlMnuItm: {
	uiLooknFeelSettings dlg( &appl, "Set Look and Feel Settings" );
	if ( dlg.go() && dlg.isChanged() )
	    uiMSG().message("Your new settings will become active\nthe next "
		    	    "time OpendTect is started.");
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl, "Set a specific User Setting" );
	dlg.go();
    } break;

    case mSettShortcutsMnuItm:	applMgr().manageShortcuts(); break;

    case mStereoOffsetMnuItm: 	applMgr().setStereoOffset(); break;
    case mStereoOffMnuItm: 
    case mStereoRCMnuItm : 
    case mStereoQuadMnuItm :
    {
	sceneMgr().setStereoType( itm->index() );
	updateStereoMenu();
    } break;

    default:
    {
	if ( id >= mViewIconsMnuItm && id < mViewIconsMnuItm+100 )
	{
	    BufferString dirnm( "icons." );
	    dirnm += itm->name().buf() + 1; // Skip the leading '&'
	    const BufferString sourcedir( GetDataFileName(dirnm) );
	    if ( !File_isDirectory(sourcedir) )
	    {
		uiMSG().error( "Icon directory seems to be invalid" );
		break;
	    }
	    const BufferString targetdir( GetDataFileName("icons.cur") );
	    File_remove( targetdir, YES );
	    File_copy( sourcedir, targetdir, YES );
	    for ( int idx=0; idx<uiToolBar::toolBars().size(); idx++ )
		uiToolBar::toolBars()[idx]->reLoadPixMaps();
	}
	if ( id > mHelpMnu )
	    helpmgr->handle( id, itm->name() );

    } break;

    }
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Seis)
mDefManCBFn(Hor)
mDefManCBFn(Flt)
mDefManCBFn(Wll)


void uiODMenuMgr::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMenuMgr::showLogFile()
{
    uiFileBrowser::Setup su( logMsgFileName() );
    su.scroll2bottom(true); su.modal(true);
    uiFileBrowser fb( &appl, su );
    fb.go();
}


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb->clear();
    fillDtectTB();
}
