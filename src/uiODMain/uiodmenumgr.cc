/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.cc,v 1.125 2008-06-16 06:05:26 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "uicrdevenv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodhelpmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uisettings.h"
#include "uisoviewer.h"
#include "uitextfile.h"
#include "uitoolbar.h"
#include "uivispartserv.h"

#include "dirlist.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "pixmap.h"
#include "strmprov.h"
#include "survinfo.h"
#include "thread.h"


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmgr_(0)
{
    surveymnu_ = new uiPopupMenu( &appl_, "&Survey" );
    procmnu_ = new uiPopupMenu( &appl_, "&Processing" );
    winmnu_ = new uiPopupMenu( &appl_, "&Windows" );
    viewmnu_ = new uiPopupMenu( &appl_, "&View" );
    utilmnu_ = new uiPopupMenu( &appl_, "&Utilities" );
    helpmnu_ = new uiPopupMenu( &appl_, "&Help" );

    dtecttb_ = new uiToolBar( &appl_, "OpendTect tools", uiToolBar::Top );
    cointb_ = new uiToolBar( &appl_, "Graphical tools", uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, "Manage data", uiToolBar::Right );

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

#define mCleanUpImpExpSets(set) \
{ \
    while ( !set.isEmpty() ) \
    { \
	uiPopupMenu* pmnu = set.remove(0); \
	if ( pmnu ) delete pmnu; \
    } \
}

void uiODMenuMgr::fillSurveyMenu()
{
    mInsertItem( surveymnu_, "&Select/Setup ...", mManSurveyMnuItm );

    uiPopupMenu* sessionitm = new uiPopupMenu( &appl_, "S&ession");
    mInsertItem( sessionitm, "&Save ...", mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    mInsertItem( sessionitm, "&Auto ...", mSessAutoMnuItm );
    surveymnu_->insertItem( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiPopupMenu( &appl_, "&Import" );
    fillImportMenu();
    surveymnu_->insertItem( impmnu_ );

    expmnu_ = new uiPopupMenu( &appl_, "Ex&port" );
    fillExportMenu();
    surveymnu_->insertItem( expmnu_ );

    manmnu_ = new uiPopupMenu( &appl_, "&Manage");
    fillManMenu();
    surveymnu_->insertItem( manmnu_ );

    surveymnu_->insertSeparator();
    mInsertItem( surveymnu_, "E&xit", mExitMnuItm );
}



void uiODMenuMgr::fillImportMenu()
{
    impmnu_->clear();
    uiPopupMenu* impseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* imphor = new uiPopupMenu( &appl_, "&Horizons" );
    uiPopupMenu* impfault = new uiPopupMenu( &appl_, "&Faults" );
    uiPopupMenu* impwell = new uiPopupMenu( &appl_, "&Wells" );
    impmnu_->insertItem( impseis );
    impmnu_->insertItem( imphor );
    impmnu_->insertItem( impfault );
    impmnu_->insertItem( impwell );
    mInsertItem( impmnu_, "&Picksets ...", mImpPickMnuItm );
    mInsertItem( impmnu_, "&Wavelets ...", mImpWvltMnuItm );

    uiPopupMenu* impseissgy = new uiPopupMenu( &appl_, "&SEG-Y" );
    mInsertItem( impseissgy, "&3D ...", mImpSeisSEGY3DMnuItm );
    mInsertItem( impseissgy, "&2D ...", mImpSeisSEGY2DMnuItm );
    mInsertItem( impseissgy, "&Pre-Stack 3D ...", mImpSeisSEGYPS3DMnuItm );
    mInsertItem( impseissgy, "Pre-&Stack 2D ...", mImpSeisSEGYPS2DMnuItm );
    impseis->insertItem( impseissgy );
    uiPopupMenu* impseissimple = new uiPopupMenu( &appl_, "S&imple file" );
    mInsertItem( impseissimple, "&3D ...", mImpSeisSimple3DMnuItm );
    mInsertItem( impseissimple, "&2D ...", mImpSeisSimple2DMnuItm );
    mInsertItem( impseissimple, "&Pre-Stack 3D ...", mImpSeisSimplePS3DMnuItm );
    mInsertItem( impseissimple, "&Pre-Stack 2D ...", mImpSeisSimplePS2DMnuItm );
    impseis->insertItem( impseissimple );
    mInsertItem( impseis, "&CBVS ...", mImpSeisCBVSMnuItm );

    uiPopupMenu* imphorasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( imphorasc, "&Geometry 3D ...", mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, "&Attributes 3D ...", mImpHorAsciiAttribMnuItm );
    mInsertItem( imphorasc, "&Geometry 2D ...", mImpHor2DAsciiMnuItm );
    imphor->insertItem( imphorasc );

    mInsertItem( impfault, "&Ascii 3D ...", mImpFaultMnuItm );

    uiPopupMenu* impwellasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );

    impmnus_.erase();
    impmnus_.allowNull();
    impmnus_ += impseis; impmnus_ += imphor; impmnus_ += impfault; 
    impmnus_ += impwell; impmnus_ += 0;
}


void uiODMenuMgr::fillExportMenu()
{
    expmnu_->clear();
    uiPopupMenu* expseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* expseissgy = new uiPopupMenu( &appl_, "&SEG-Y" );
    mInsertItem( expseissgy, "&3D ...", mExpSeisSEGY3DMnuItm );
    mInsertItem( expseissgy, "&2D ...", mExpSeisSEGY2DMnuItm );
    expseis->insertItem( expseissgy );
    uiPopupMenu* expseissimple = new uiPopupMenu( &appl_, "S&imple file" );
    mInsertItem( expseissimple, "&3D ...", mExpSeisSimple3DMnuItm );
    mInsertItem( expseissimple, "&2D ...", mExpSeisSimple2DMnuItm );
    expseis->insertItem( expseissimple );
    expmnu_->insertItem( expseis );

    uiPopupMenu* exphor = new uiPopupMenu( &appl_, "&Horizons" );
    mInsertItem( exphor, "&Ascii ...", mExpHorAsciiMnuItm );
    expmnu_->insertItem( exphor );

    uiPopupMenu* expflt = new uiPopupMenu( &appl_, "&Faults" );
    mInsertItem( expflt, "&Ascii ...", mExpFltAsciiMnuItm );
    expmnu_->insertItem( expflt );

    mInsertItem( expmnu_, "&Picksets ...", mExpPickMnuItm );

    expmnus_.erase();
    expmnus_.allowNull();
    expmnus_ += expseis; expmnus_ += exphor;
    expmnus_+= 0; expmnus_+=0; expmnus_+= 0;
}


void uiODMenuMgr::fillManMenu()
{
    manmnu_->clear();
    mInsertItem( manmnu_, "&AttributeSets ...", mManAttrMnuItm );
    mInsertItem( manmnu_, "&Faults ...", mManFaultMnuItm );
    if ( SI().getSurvDataType() == SurveyInfo::No2D )
	mInsertItem( manmnu_, "&Horizons ...", mManHor3DMnuItm );
    else
    {
	mInsertItem( manmnu_, "&Horizons 3D ...", mManHor3DMnuItm );
	mInsertItem( manmnu_, "Horizons &2D ...", mManHor2DMnuItm );
    }
    mInsertItem( manmnu_, "&PickSets ...", mManPickMnuItm );
    mInsertItem( manmnu_, "&Seismics ...", mManSeisMnuItm );
    mInsertItem( manmnu_, "Strati&graphy ...", mManStratMnuItm );
    mInsertItem( manmnu_, "Wa&velets ...", mManWvltMnuItm );
    mInsertItem( manmnu_, "&Wells ...", mManWellMnuItm );
}


#define mCreateHorMnu( mnu, nm, dim )\
if ( SI().has##dim() )\
{\
    uiPopupMenu* mnu = hasboth ? new uiPopupMenu( &appl_, nm ) :horitm;\
    mInsertItem( mnu, "&Horizon grid...", mCreateSurf##dim##MnuItm );\
    mInsertItem( mnu, "&Between horizons ...", mCompBetweenHor##dim##MnuItm );\
    mInsertItem( mnu, "&Horizon slice...", mCompAlongHor##dim##MnuItm );\
    if ( hasboth )\
	horitm->insertItem( mnu );\
}

void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();
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
		new uiPopupMenu( &appl_, "Create output using &Horizon");
    bool hasboth = SI().has2D() && SI().has3D();
    mCreateHorMnu( mnu2d, "&2D", 2D );
    mCreateHorMnu( mnu3d, "&3D", 3D );
    procmnu_->insertItem( horitm );
    
    mInsertItem( procmnu_, "&Cross-plot ...", mXplotMnuItm );
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
    winmnu_->insertSeparator();
}


void uiODMenuMgr::updateWindowsMenu()
{
    BufferStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int id=mSceneSelMnuItm; id<mSceneSelMnuItm+scenenms.size()+1; id++ )
	winmnu_->removeItem( id );

#define mInsertSceneItem(txt,docheck,id) \
    uiMenuItem* itm = new uiMenuItem( txt, \
	    			      mCB(this,uiODMenuMgr,handleClick) ); \
    winmnu_->insertItem( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    for ( int idx=0; idx<scenenms.size(); idx++ )
    {
	mInsertSceneItem( scenenms.get(idx), idx==activescene,
			  mSceneSelMnuItm+idx );    
    }
}


void uiODMenuMgr::fillViewMenu()
{
    if ( !viewmnu_ ) return;

    viewmnu_->clear();
    mInsertItem( viewmnu_, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu_, "&Z-scale ...", mZScaleMnuItm );
    uiPopupMenu* stereoitm = new uiPopupMenu( &appl_, "&Stereo viewing" );
    viewmnu_->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id) \
    itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, "&Off", true, mStereoOffMnuItm )
    mInsertStereoItem( stereoredcyanitm_, "&Red/Cyan", false,
	    		mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, "&Quad buffer", false,
	    		mStereoQuadMnuItm )

    stereooffsetitm_ = new uiMenuItem( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiPopupMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.item().setName("&Toolbars");
    viewmnu_->insertItem( &toolbarsmnu );
}


void uiODMenuMgr::addIconMnuItems( const DirList& dl, uiPopupMenu* iconsmnu,
				   int& nradded )
{
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get( idx ).buf() + 6 );
	if ( nm.isEmpty() || nm == "Default" || nm == "cur" )
	    continue;

	nradded++;
	BufferString mnunm( "&" ); mnunm += nm;
	mInsertItem( iconsmnu, mnunm, mViewIconsMnuItm+nradded );
    }
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dlsite( mGetApplSetupDataDir(), DirList::DirsOnly, "icons.*" );
    DirList dlrel( mGetSWDirDataDir(), DirList::DirsOnly, "icons.*" );
    if ( dlsite.size() + dlrel.size() < 2 )
	return;

    uiPopupMenu* iconsmnu = new uiPopupMenu( &appl_, "&Icons" );
    viewmnu_->insertItem( iconsmnu );
    mInsertItem( iconsmnu, "&Default", mViewIconsMnuItm+0 );
    int nradded = 0;
    addIconMnuItems( dlsite, iconsmnu, nradded );
    addIconMnuItems( dlrel, iconsmnu, nradded );
}


extern const char* logMsgFileName();

void uiODMenuMgr::fillUtilMenu()
{
    settmnu_ = new uiPopupMenu( &appl_, "&Settings" );
    utilmnu_->insertItem( settmnu_ );
    mInsertItem( settmnu_, "&Fonts ...", mSettFontsMnuItm );
    mInsertItem( settmnu_, "&Look and feel ...", mSettLkNFlMnuItm );
    mInsertItem( settmnu_, "&Mouse controls ...", mSettMouseMnuItm );
    mInsertItem( settmnu_, "&Keyboard shortcuts ...", mSettShortcutsMnuItm );
    mInsertItem( settmnu_, "&General ...", mSettGeneral );

    mInsertItem( utilmnu_, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( utilmnu_, "&Plugins ...", mPluginsMnuItm );
    mInsertItem( utilmnu_, "&Position conversion ...", mPosconvMnuItm );
    mInsertItem( utilmnu_, "&Create Devel. Env. ...", mCrDevEnvMnuItm );
    const char* lmfnm = logMsgFileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, "Show &log file ...", mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
	mInsertItem( utilmnu_, "Data pack dump ...", mDumpDataPacksMnuItm );
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
    mAddTB(dtecttb_,"xplot.png","Crossplot Attribute vs Well data",false,xPlot);

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm , mCB(this,uiODMenuMgr,fn), txt, togg )

void uiODMenuMgr::fillManTB()
{
    mAddTB(mantb_,"man_seis.png","Manage seismic data",false,manSeis);
    mAddTB(mantb_,"man_hor.png","Manage horizons",false,manHor);
    mAddTB(mantb_,"man_flt.png","Manage faults",false,manFlt);
    mAddTB(mantb_,"man_wll.png","Manage well data",false,manWll);
    mAddTB(mantb_,"man_picks.png","Manage Pick Sets",false,manPick);
    mAddTB(mantb_,"man_wvlt.png","Manage Wavelets",false,manWvlt);
    mAddTB(mantb_,"man_strat.png","Manage Stratigraphy",false,manStrat);
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
    cameraid_ = mAddTB(cointb_,"orthographic.png",
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
    const char* fnm = perspective ? "orthographic.png" : "perspective.png";
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
    case mSessRestMnuItm: 	appl_.restoreSession(); break;
    case mSessAutoMnuItm: 	appl_.autoSession(); break;
    case mImpSeisSEGY3DMnuItm:	mDoOp(Imp,Seis,0); break;
    case mImpSeisSEGY2DMnuItm:	mDoOp(Imp,Seis,1); break;
    case mImpSeisSEGYPS3DMnuItm: mDoOp(Imp,Seis,2); break;
    case mImpSeisSEGYPS2DMnuItm: mDoOp(Imp,Seis,3); break;
    case mImpSeisCBVSMnuItm: 	mDoOp(Imp,Seis,4); break;
    case mImpSeisSimple3DMnuItm: mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm: mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm: mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm: mDoOp(Imp,Seis,8); break;
    case mExpSeisSEGY3DMnuItm: 	mDoOp(Exp,Seis,0); break;
    case mExpSeisSEGY2DMnuItm: 	mDoOp(Exp,Seis,1); break;
    case mExpSeisSimple3DMnuItm: mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm: mDoOp(Exp,Seis,6); break;
    case mImpHorAsciiMnuItm: mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm: mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm: mDoOp(Imp,Hor,2); break;
    case mExpHorAsciiMnuItm: 	mDoOp(Exp,Hor,0); break;
    case mExpFltAsciiMnuItm: 	mDoOp(Exp,Flt,0); break;
    case mImpWellAsciiTrackMnuItm: mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm: mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm: mDoOp(Imp,Wll,2); break;
    case mImpPickMnuItm: 	mDoOp(Imp,Pick,0); break;
    case mExpPickMnuItm: 	mDoOp(Exp,Pick,0); break;
    case mImpWvltMnuItm: 	mDoOp(Imp,Wvlt,0); break;
    case mImpFaultMnuItm: 	mDoOp(Imp,Flt,0); break;

    case mManSeisMnuItm: 	mDoOp(Man,Seis,0); break;
    case mManHor3DMnuItm: 	mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm: 	mDoOp(Man,Hor,1); break;
    case mManFaultMnuItm: 	mDoOp(Man,Flt,0); break;
    case mManWellMnuItm: 	mDoOp(Man,Wll,0); break;
    case mManPickMnuItm: 	mDoOp(Man,Pick,0); break;
    case mManWvltMnuItm: 	mDoOp(Man,Wvlt,0); break;
    case mManAttrMnuItm:	mDoOp(Man,Attr,0); break;
    case mManNLAMnuItm:		mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:	mDoOp(Man,Sess,0); break;
    case mManStratMnuItm:	mDoOp(Man,Strat,0); break;

    case mExitMnuItm: 		appl_.exit(); break;

    case mEditAttrMnuItm: 	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm: 	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm: 	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol( SI().has2D() ); break;
    case mSeisOut2DMnuItm: 	applMgr().createVol(true); break;
    case mSeisOut3DMnuItm: 	applMgr().createVol(false); break;
    case mCreateSurf2DMnuItm: 	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm: 	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mReStartMnuItm: 	applMgr().reStartProc(); break;
    case mXplotMnuItm:		applMgr().doXPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mCascadeMnuItm: 	sceneMgr().cascade(); break;
    case mTileAutoMnuItm: 	sceneMgr().tile(); break;
    case mTileHorMnuItm: 	sceneMgr().tileHorizontal(); break;
    case mTileVerMnuItm: 	sceneMgr().tileVertical(); break;
    case mWorkAreaMnuItm: 	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm: 	applMgr().setZScale(); break;
    case mBatchProgMnuItm: 	applMgr().batchProgs(); break;
    case mPluginsMnuItm: 	applMgr().pluginMan(); break;
    case mPosconvMnuItm:	applMgr().posConversion(); break;	
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

    case mDumpDataPacksMnuItm: {
	uiFileDialog dlg( &appl_, false, "/tmp/dpacks.txt",
			  "*.txt", "Data pack dump" );
	dlg.setAllowAllExts( true );
	if ( dlg.go() )
	{
	    StreamData sd( StreamProvider(dlg.fileName()).makeOStream() );
	    if ( sd.usable() ) DataPackMgr::dumpDPMs( *sd.ostrm );
	}
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
	const int type = id == mStereoRCMnuItm ? 1 
	    				: (id == mStereoQuadMnuItm ? 2 : 0 );
	sceneMgr().setStereoType( type );
	updateStereoMenu();
    } break;

    default:
    {
	if ( id>=mSceneSelMnuItm && id<=mSceneSelMnuItm +100 )
	{
	    const char* scenenm = itm->name();
	    sceneMgr().setActiveScene( scenenm );
	}

	if ( id >= mViewIconsMnuItm && id < mViewIconsMnuItm+100 )
	{
	    BufferString dirnm( "icons." );
	    dirnm += itm->name().buf() + 1; // Skip the leading '&'
	    const BufferString sourcedir( mGetSetupFileName(dirnm) );
	    if ( !File_isDirectory(sourcedir) )
	    {
		uiMSG().error( "Icon directory seems to be invalid" );
		break;
	    }

	    const BufferString targetdir( GetSettingsFileName("icons") );
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
mDefManCBFn(Strat)


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
    fillImportMenu();
    fillExportMenu();
    fillManMenu();

    fillProcMenu();
    dTectMnuChanged.trigger();
}
