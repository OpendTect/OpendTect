/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.cc,v 1.69 2007-01-11 15:48:45 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "uicrdevenv.h"
#include "uitextfile.h"
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
#include "survinfo.h"


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
	: appl_(*a)
    	, timer_(*new Timer("popup timer"))
    	, helpmgr_(0)
    	, dTectTBChanged(this)
    	, dTectMnuChanged(this)
	, surveymnu_(0)
	, procmnu_(0)
	, winmnu_(0)
	, viewmnu_(0)
	, utilmnu_(0)
	, helpmnu_(0)
	, dtecttb_(0)
	, cointb_(0)
	, mantb_(0)
{
    surveymnu_ = new uiPopupMenu( &appl_, "&Survey" );
    procmnu_ = new uiPopupMenu( &appl_, "&Processing" );
    winmnu_ = new uiPopupMenu( &appl_, "&Windows" );
    viewmnu_ = new uiPopupMenu( &appl_, "&View" );
    utilmnu_ = new uiPopupMenu( &appl_, "&Utilities" );
    helpmnu_ = new uiPopupMenu( &appl_, "&Help" );

    dtecttb_ = new uiToolBar( &appl_, "OpendTect tools" );
    dtecttb_->setVerticallyStretchable( true );
    cointb_ = new uiToolBar( &appl_, "Graphical tools" );
    cointb_->setVerticallyStretchable( true );
    mantb_ = new uiToolBar( &appl_, "Manage data" );
    mantb_->setVerticallyStretchable( true );

    appl_.applMgr().visServer()->createToolBars();
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete dtecttb_;
    delete cointb_;
    delete mantb_;
    delete helpmgr_;
}


void uiODMenuMgr::initSceneMgrDepObjs( uiODApplMgr* appman,
				       uiODSceneMgr* sceneman )
{
    uiMenuBar* menubar = appl_.menuBar();
    fillSurveyMenu();	menubar->insertItem( surveymnu_ );
    fillProcMenu();	menubar->insertItem( procmnu_ );
    fillWinMenu();	menubar->insertItem( winmnu_ );
    fillViewMenu();	menubar->insertItem( viewmnu_ );
    fillUtilMenu();	menubar->insertItem( utilmnu_ );
    menubar->insertSeparator();
    helpmgr_ = new uiODHelpMenuMgr( this );
    menubar->insertItem( helpmnu_ );

    fillDtectTB( appman );
    fillCoinTB( sceneman );
    fillManTB();

    timer_.tick.notify( mCB(this,uiODMenuMgr,timerCB) );
}


uiPopupMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu_ :
	  (at == uiODApplMgr::Exp ? expmnu_ : manmnu_);
}


uiPopupMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus_[(int)ot] : expmnus_[(int)ot];
}


void uiODMenuMgr::storePositions()
{
    dtecttb_->storePosition();
    cointb_->storePosition();
    mantb_->storePosition();
}


void uiODMenuMgr::updateStereoMenu()
{
    uiSoViewer::StereoType type = 
			(uiSoViewer::StereoType)sceneMgr().getStereoType();
    stereooffitm_->setChecked( type == uiSoViewer::None );
    stereoredcyanitm_->setChecked( type == uiSoViewer::RedCyan );
    stereoquadbufitm_->setChecked( type == uiSoViewer::QuadBuffer );
    stereooffsetitm_->setEnabled( type != uiSoViewer::None );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    cointb_->turnOn( viewid_, isview );
    cointb_->turnOn( actid_, !isview );
    cointb_->setSensitive( axisid_, isview );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{ cointb_->turnOn( axisid_, shwaxis ); }

bool uiODMenuMgr::isSoloModeOn() const
{ return cointb_->isOn( soloid_ ); }

void uiODMenuMgr::enableMenuBar( bool yn )
{ appl_.menuBar()->setSensitive( yn ); }

void uiODMenuMgr::enableActButton( bool yn )
{ cointb_->setSensitive( actid_, yn ); }


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick)), id )

void uiODMenuMgr::fillSurveyMenu()
{
    mInsertItem( surveymnu_, "&Edit/New ...", mManSurveyMnuItm );

    uiPopupMenu* sessionitm = new uiPopupMenu( &appl_, "S&ession");
    mInsertItem( sessionitm, "&Save ...", mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    surveymnu_->insertItem( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiPopupMenu( &appl_, "&Import" );
    uiPopupMenu* impseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* imphor = new uiPopupMenu( &appl_, "&Horizons" );
    uiPopupMenu* impfault = new uiPopupMenu( &appl_, "&Faults" );
    uiPopupMenu* impwell = new uiPopupMenu( &appl_, "&Wells" );
    impmnu_->insertItem( impseis );
    impmnu_->insertItem( imphor );
#ifdef __debug__
    impmnu_->insertItem( impfault );
    mInsertItem( impfault, "&Landmark ...", mImpLmkFaultMnuItm );
#endif
    impmnu_->insertItem( impwell );
    mInsertItem( impmnu_, "&Picksets ...", mImpPickMnuItm );
    mInsertItem( impmnu_, "&Wavelets ...", mImpWvltMnuItm );

    uiPopupMenu* impseissgy = new uiPopupMenu( &appl_, "&SEG-Y" );
    mInsertItem( impseissgy, "&3D ...", mImpSeisSEGY3DMnuItm );
    mInsertItem( impseissgy, "&2D ...", mImpSeisSEGY2DMnuItm );
    if ( GetEnvVarYN("OD_SHOW_PRESTACK_SEGY_IMP") )
	mInsertItem( impseissgy, "&Pre-Stack ...", mImpSeisSEGYPSMnuItm );
    impseis->insertItem( impseissgy );
    mInsertItem( impseis, "&CBVS ...", mImpSeisCBVSMnuItm );

    uiPopupMenu* imphorasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( imphorasc, "&Geometry ...", mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, "&Attributes ...", mImpHorAsciiAttribMnuItm );
    imphor->insertItem( imphorasc );

    uiPopupMenu* impwellasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );

    expmnu_ = new uiPopupMenu( &appl_, "E&xport" );
    uiPopupMenu* expseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* expseissgy = new uiPopupMenu( &appl_, "&SEG-Y" );
    mInsertItem( expseissgy, "&3D ...", mExpSeisSEGY3DMnuItm );
    mInsertItem( expseissgy, "&2D ...", mExpSeisSEGY2DMnuItm );
    expseis->insertItem( expseissgy );
    uiPopupMenu* exphor = new uiPopupMenu( &appl_, "&Horizons" );
    expmnu_->insertItem( expseis );
    expmnu_->insertItem( exphor );
    mInsertItem( exphor, "&Ascii ...", mExpHorAsciiMnuItm );
    mInsertItem( expmnu_, "&Picksets ...", mExpPickMnuItm );

    surveymnu_->insertItem( impmnu_ );
    surveymnu_->insertItem( expmnu_ );
    manmnu_ = new uiPopupMenu( &appl_, "&Manage");

    mInsertItem( manmnu_, "&AttributeSets ...", mManAttrMnuItm );
#ifdef __debug__
    mInsertItem( manmnu_, "&Faults ...", mManFaultMnuItm );
#endif
    mInsertItem( manmnu_, "&Horizons ...", mManHorMnuItm );
    mInsertItem( manmnu_, "&PickSets ...", mManPickMnuItm );
    mInsertItem( manmnu_, "&Seismics ...", mManSeisMnuItm );
    mInsertItem( manmnu_, "Wa&velets ...", mManWvltMnuItm );
    mInsertItem( manmnu_, "&Wells ...", mManWellMnuItm );
    surveymnu_->insertItem( manmnu_ );

    surveymnu_->insertSeparator();
    mInsertItem( surveymnu_, "E&xit", mExitMnuItm );

    impmnus_.allowNull();
    impmnus_ += impseis; impmnus_ += imphor; impmnus_ += impfault; 
    impmnus_ += impwell; impmnus_ += 0;
    expmnus_.allowNull();
    expmnus_ += expseis; expmnus_ += exphor;
    expmnus_+= 0; expmnus_+=0; expmnus_+= 0;
}


void uiODMenuMgr::fillProcMenu()
{
    if ( SI().getSurvDataType() == SurveyInfo::Both2DAnd3D )
    {
	uiPopupMenu* aitm = new uiPopupMenu( &appl_, "Attributes" );
	mInsertItem( aitm, "&2D ...", mEdit2DAttrMnuItm );
	mInsertItem( aitm, "&3D ...", mEdit3DAttrMnuItm );
	procmnu_->insertItem( aitm );
	procmnu_->insertSeparator();
	uiPopupMenu* sitm = new uiPopupMenu( &appl_, "&Create Seismic output" );
	mInsertItem( sitm, "&2D ...", mSeisOut2DMnuItm );
	mInsertItem( sitm, "&3D ...", mSeisOut3DMnuItm );
	procmnu_->insertItem( sitm );
    }
    else
    {
	mInsertItem( procmnu_, "&Attributes ...", mEditAttrMnuItm );
	procmnu_->insertSeparator();
	mInsertItem( procmnu_, "&Create Seismic output ...", mSeisOutMnuItm );
    }

    uiPopupMenu* horitm = 
		new uiPopupMenu( &appl_, "Create output using &Horizon ...");
    mInsertItem( horitm, "&Horizon grid...", mCreateSurfMnuItm );
    mInsertItem( horitm, "&Between horizons ...", mCompBetweenHorMnuItm );
    mInsertItem( horitm, "&Horizon slice...", mCompAlongHorMnuItm );
    procmnu_->insertItem( horitm );
    
    mInsertItem( procmnu_, "&Re-Start ...", mReStartMnuItm );
}


void uiODMenuMgr::fillWinMenu()
{
    mInsertItem( winmnu_, "&New", mAddSceneMnuItm );
    mInsertItem( winmnu_, "&Cascade", mCascadeMnuItm );
    uiPopupMenu* tileitm = new uiPopupMenu( &appl_, "&Tile" );
    winmnu_->insertItem( tileitm );

    mInsertItem( tileitm, "&Auto", mTileAutoMnuItm );
    mInsertItem( tileitm, "&Horizontal", mTileHorMnuItm );
    mInsertItem( tileitm, "&Vertical", mTileVerMnuItm );
}


void uiODMenuMgr::fillViewMenu()
{
    mInsertItem( viewmnu_, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu_, "&Z-scale ...", mZScaleMnuItm );
    uiPopupMenu* stereoitm = new uiPopupMenu( &appl_, "&Stereo viewing" );
    viewmnu_->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id,idx) \
    itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id, idx ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, "&Off", true, mStereoOffMnuItm, 0 )
    mInsertStereoItem( stereoredcyanitm_, "&Red/Cyan", false,
	    		mStereoRCMnuItm, 1 )
    mInsertStereoItem( stereoquadbufitm_, "&Quad buffer", false,
	    		mStereoQuadMnuItm, 2 )

    stereooffsetitm_ = new uiMenuItem( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm_, mStereoOffsetMnuItm, 3 );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();

    viewmnu_->insertSeparator();
    viewmnu_->insertItem( &appl_.createDockWindowMenu() );
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dl( GetDataFileName(0), DirList::DirsOnly, "icons.*" );
    if ( dl.size() < 2 )
	return;

    uiPopupMenu* iconsmnu = new uiPopupMenu( &appl_, "&Icons" );
    viewmnu_->insertItem( iconsmnu );
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
    settmnu_ = new uiPopupMenu( &appl_, "&Settings" );
    utilmnu_->insertItem( settmnu_ );
    mInsertItem( settmnu_, "&Fonts ...", mSettFontsMnuItm );
    mInsertItem( settmnu_, "&Mouse controls ...", mSettMouseMnuItm );
    mInsertItem( settmnu_, "&Look and feel ...", mSettLkNFlMnuItm );
    mInsertItem( settmnu_, "&Shortcuts ...", mSettShortcutsMnuItm );
    mInsertItem( settmnu_, "&General ...", mSettGeneral );

    mInsertItem( utilmnu_, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( utilmnu_, "&Plugins ...", mPluginsMnuItm );
    mInsertItem( utilmnu_, "&Create Devel. Env. ...", mCrDevEnvMnuItm );
    const char* lmfnm = logMsgFileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, "Show &log file ...", mShwLogFileMnuItm );
}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, mCB(appman,uiODApplMgr,fn), txt, togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey.png","Survey setup",false,manSurvCB);

    SurveyInfo::Pol2D survtype = SI().getSurvDataType();
    if ( survtype == SurveyInfo::Only2D )
    {
	mAddTB(dtecttb_,"attributes.png","Edit attributes",false,editAttr2DCB);
	mAddTB(dtecttb_,"out_2dlines.png","Create seismic output",
	       false,seisOut2DCB);
    }
    else if ( survtype == SurveyInfo::No2D )
    {
	mAddTB(dtecttb_,"attributes.png","Edit attributes",false,editAttr3DCB);
	mAddTB(dtecttb_,"out_vol.png","Create seismic output",false,seisOut3DCB);
    }
    else
    {
	mAddTB(dtecttb_,"attributes_2d.png","Edit 2D attributes",false,
	       editAttr2DCB);
	mAddTB(dtecttb_,"attributes_3d.png","Edit 3D attributes",false,
	       editAttr3DCB);
	mAddTB(dtecttb_,"out_2dlines.png","Create 2D seismic output",false,
	       seisOut2DCB);
	mAddTB(dtecttb_,"out_vol.png","Create 3D seismic output",false,
	       seisOut3DCB);
    }

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm , mCB(this,uiODMenuMgr,fn), txt, togg )

void uiODMenuMgr::fillManTB()
{
    mAddTB(mantb_,"man_seis.png","Manage seismic data",false,manSeis);
    mAddTB(mantb_,"man_hor.png","Manage horizons",false,manHor);
#ifdef __debug__
    mAddTB(mantb_,"man_flt.png","Manage faults",false,manFlt);
#endif
    mAddTB(mantb_,"man_wll.png","Manage well data",false,manWll);
    mAddTB(mantb_,"man_picks.png","Manage Pick Sets",false,manPick);
    mAddTB(mantb_,"man_wvlt.png","Manage Wavelets",false,manWvlt);
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, mCB(scenemgr,uiODSceneMgr,fn), txt, togg )

void uiODMenuMgr::fillCoinTB( uiODSceneMgr* scenemgr )
{
    viewid_ = mAddTB(cointb_,"view.png","View mode",true,viewMode);
    actid_ = mAddTB(cointb_,"pick.png","Interact mode",true,actMode);
    mAddTB(cointb_,"home.png","To home position",false,toHomePos);
    mAddTB(cointb_,"set_home.png","Save home position",false,saveHomePos);
    mAddTB(cointb_,"view_all.png","View all",false,viewAll);
    cameraid_ = mAddTB(cointb_,"perspective.png",
	    	       "Switch to orthographic camera",false,switchCameraType);
    mAddTB(cointb_,"cube_inl.png","View Inl",false,viewInl);
    mAddTB(cointb_,"cube_crl.png","View Crl",false,viewCrl);
    mAddTB(cointb_,"cube_z.png","View Z",false,viewZ);
    axisid_ = mAddTB(cointb_,"axis.png","Display rotation axis",
	    	     true,showRotAxis);
    mAddTB(cointb_,"snapshot.png","Make snapshot",false,mkSnapshot);
    soloid_ = mAddTB(cointb_,"solo.png","Display current element only",
		     true,soloMode);

    cointb_->turnOn( actid_, true );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    cointb_->setToolTip(cameraid_,perspective ? "Switch to orthographic camera"
					      : "Switch to perspective camera");
    const char* fnm = perspective ? "perspective.png" : "orthographic.png";
    cointb_->setPixmap( cameraid_, ioPixmap(fnm) );
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
    case mSessSaveMnuItm: 	appl_.saveSession(); break;
    case mSessRestMnuItm: 	{ appl_.restoreSession(); 
				  timer_.start(200,true);  break; }
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

    case mExitMnuItm: 		appl_.exit(); break;

    case mEditAttrMnuItm: 	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm: 	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm: 	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol( SI().has2D() ); break;
    case mSeisOut2DMnuItm: 	applMgr().createVol(true); break;
    case mSeisOut3DMnuItm: 	applMgr().createVol(false); break;
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
    case mCrDevEnvMnuItm: 	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm: 	showLogFile(); break;
    case mSettFontsMnuItm: 	applMgr().setFonts(); break;
    case mSettMouseMnuItm: 	sceneMgr().setKeyBindings(); break;

    case mSettLkNFlMnuItm: {
	uiLooknFeelSettings dlg( &appl_, "Set Look and Feel Settings" );
	if ( dlg.go() && dlg.isChanged() )
	    uiMSG().message("Your new settings will become active\nthe next "
		    	    "time OpendTect is started.");
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set a specific User Setting" );
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
	    helpmgr_->handle( id, itm->name() );

    } break;

    }
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Seis)
mDefManCBFn(Hor)
mDefManCBFn(Flt)
mDefManCBFn(Wll)
mDefManCBFn(Pick)
mDefManCBFn(Wvlt)


void uiODMenuMgr::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMenuMgr::showLogFile()
{
    uiTextFileDlg::Setup su( logMsgFileName() );
    su.modal( true );
    uiTextFileDlg dlg( &appl_, su );
    dlg.go();
}


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb_->clear();
    fillDtectTB( &applMgr() );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    surveymnu_->clear();
    procmnu_->clear();
    fillSurveyMenu();
    fillProcMenu();
    dTectMnuChanged.trigger();
}


