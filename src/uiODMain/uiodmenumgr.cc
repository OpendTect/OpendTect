/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "ui3dviewer.h"
#include "uicrdevenv.h"
#include "uidatapackmon.h"
#include "uifiledlg.h"
#include "uifirewallprocsetterdlg.h"
#include "uiglinfo.h"
#include "uihostiddlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodfaulttoolman.h"
#include "uiodhelpmenumgr.h"
#include "uiodlangmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uiproxydlg.h"
#include "uisettings.h"
#include "uiseispartserv.h"
#include "uistrings.h"
#include "uitextfile.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uivispartserv.h"
#include "uivolprocchain.h"
#include "uiwellpartserv.h"
#include "visemobjdisplay.h"

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "measuretoolman.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsysmem.h"
#include "odver.h"
#include "settings.h"
#include "od_ostream.h"
#include "survinfo.h"
#include "texttranslator.h"
#include "thread.h"


static const char* sKeyIconSetNm = "Icon set name";
static const char* ascic = "ascii";

uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmgr_(0)
    , measuretoolman_(0)
    , inviewmode_(false)
    , langmnu_(0)
    , plugintb_(0)
    , addtimedepthsceneitm_(0)
    , faulttoolman_(0)
{
    uiMenuBar* mb = appl_.menuBar();
    surveymnu_ = mb->addMenu( new uiMenu(uiStrings::sSurvey()) );
    analmnu_ = mb->addMenu( new uiMenu(uiStrings::sAnalysis()) );
    procmnu_ = mb->addMenu( new uiMenu(uiStrings::sProcessing()) );
    scenemnu_ = mb->addMenu( new uiMenu(uiStrings::sScenes()) );
    viewmnu_ = mb->addMenu( new uiMenu(uiStrings::sView()) );
    utilmnu_ = mb->addMenu( new uiMenu(uiStrings::sUtilities()) );
    auto* tnmnu = mb->addMenu( new uiMenu(toUiString("TerraNubis")) );
    helpmnu_ = mb->addMenu( new uiMenu(uiStrings::sHelp()) );

    insertAction( tnmnu, tr("Download Free Projects"), mFreeProjects );
    insertAction( tnmnu, tr("Download Commercial Projects"), mCommProjects );

    dtecttb_ = new uiToolBar( &appl_, tr("OpendTect Tools"), uiToolBar::Top );
    viewtb_ = new uiToolBar( &appl_, tr("Graphical Tools"), uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, uiStrings::phrManage(uiStrings::sData()),
			    uiToolBar::Right );

    uiVisPartServer* visserv = appl_.applMgr().visServer();
    visserv->createToolBars();

    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
    visserv->selectionmodeChange.notify(
				mCB(this,uiODMenuMgr,selectionMode) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete appl_.removeToolBar( dtecttb_ );
    delete appl_.removeToolBar( viewtb_ );
    delete appl_.removeToolBar( mantb_ );
    if ( plugintb_ )
	delete appl_.removeToolBar( plugintb_ );

    for ( int idx=0; idx<customtbs_.size(); idx++ )
	delete appl_.removeToolBar( customtbs_[idx] );

    delete helpmgr_;
    delete langmnumgr_;
    delete faulttoolman_;
    delete measuretoolman_;
}


void uiODMenuMgr::initSceneMgrDepObjs( uiODApplMgr* appman,
				       uiODSceneMgr* sceneman )
{
    uiMenuBar* menubar = appl_.menuBar();
    fillSurveyMenu();
    fillAnalMenu();
    fillProcMenu();
    fillSceneMenu();
    fillViewMenu();
    fillUtilMenu();
    menubar->insertSeparator();
    helpmgr_ = new uiODHelpMenuMgr( this );
    langmnumgr_ = new uiODLangMenuMgr( this );

    fillDtectTB( appman );
    fillCoinTB( sceneman );
    fillManTB();
    uiSettsMgr().loadToolBarCmds( appl_ );

    measuretoolman_ = new MeasureToolMan( appl_ );
}


void uiODMenuMgr::createFaultToolMan()
{
    if ( !faulttoolman_ )
	faulttoolman_ = new uiODFaultToolMan( appl_ );
}


uiMenu* uiODMenuMgr::docMnu()
{ return helpmgr_->getDocMenu(); }


uiMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu_ :
	  (at == uiODApplMgr::Exp ? expmnu_ : manmnu_);
}


uiMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus_[(int)ot] : expmnus_[(int)ot];
}


void uiODMenuMgr::updateStereoMenu()
{
    ui3DViewer::StereoType type =
			(ui3DViewer::StereoType)sceneMgr().getStereoType();
    stereooffitm_->setChecked( type == ui3DViewer::None );
    stereoredcyanitm_->setChecked( type == ui3DViewer::RedCyan );
    stereoquadbufitm_->setChecked( type == ui3DViewer::QuadBuffer );
    stereooffsetitm_->setEnabled( type != ui3DViewer::None );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    if ( inviewmode_ == isview )
	return;
    toggViewMode( 0 );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{ viewtb_->turnOn( axisid_, shwaxis ); }

bool uiODMenuMgr::isSoloModeOn() const
{ return viewtb_->isOn( soloid_ ); }

void uiODMenuMgr::enableMenuBar( bool yn )
{ appl_.menuBar()->setSensitive( yn ); }

void uiODMenuMgr::enableActButton( bool yn )
{
    if ( yn )
	{ viewtb_->setSensitive( actviewid_, true ); return; }

    if ( !inviewmode_ )
	toggViewMode(0);
    viewtb_->setSensitive( actviewid_, false );
}


void uiODMenuMgr::insertAction( uiMenu* mnu, const uiString& txt, int id,
				const char* iconnm )
{
    if ( !mnu )
	return;

    if ( iconnm )
	mnu->insertAction(
	    new uiAction(txt,mCB(this,uiODMenuMgr,handleClick),iconnm), id );
    else
	mnu->insertAction(
	    new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), id );
}

#define mCleanUpImpExpSets(set) \
{ \
    while ( !set.isEmpty() ) \
    { \
	uiMenu* pmnu = set.removeSingle(0); \
	if ( pmnu ) delete pmnu; \
    } \
}

void uiODMenuMgr::fillSurveyMenu()
{
    insertAction( surveymnu_, m3Dots(tr("Select/Setup")), mManSurveyMnuItm,
		  "survey" );

    uiMenu* sessionitm = new uiMenu( &appl_, tr("Session") ) ;
    insertAction( sessionitm, m3Dots(uiStrings::sSave()), mSessSaveMnuItm,
		  "save" );
    insertAction( sessionitm, m3Dots(tr("Restore")), mSessRestMnuItm, "open" );
    insertAction( sessionitm, m3Dots(tr("Auto-load")), mSessAutoMnuItm );
    surveymnu_->addMenu( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiMenu( &appl_, uiStrings::sImport(), "import" );
    fillImportMenu();
    surveymnu_->addMenu( impmnu_ );

    expmnu_ = new uiMenu( &appl_, uiStrings::sExport(), "export" );
    fillExportMenu();
    surveymnu_->addMenu( expmnu_ );

    manmnu_ = new uiMenu( &appl_, tr("Manage"), "manage" );
    fillManMenu();
    surveymnu_->addMenu( manmnu_ );

    preloadmnu_ = new uiMenu( &appl_, tr("Pre-load"), "preloaded" );
    insertAction( preloadmnu_, m3Dots(uiStrings::sSeismicData()),
		 mPreLoadSeisMnuItm, "preload_seis" );
    insertAction( preloadmnu_, m3Dots(uiStrings::sHorizon(mPlural)),
		 mPreLoadHorMnuItm, "preload_horizon" );
    surveymnu_->addMenu( preloadmnu_ );

    surveymnu_->insertSeparator();
    insertAction( surveymnu_, uiStrings::sRestart(),
		       mRestartMnuItm, "restart" );
    insertAction( surveymnu_, uiStrings::sExit(), mExitMnuItm, "exit" );
}


void uiODMenuMgr::fillImportMenu()
{
    impmnu_->clear();

    uiMenu* impattr = new uiMenu( &appl_, uiStrings::sAttributeSet() );
    uiMenu* impseis = new uiMenu( &appl_, uiStrings::sSeismicData() );
    uiMenu* imphor = new uiMenu( &appl_, uiStrings::sHorizon(mPlural) );
    uiMenu* impfault = new uiMenu( &appl_, uiStrings::sFault(mPlural) );
    uiMenu* impfaultstick = new uiMenu( &appl_,
					uiStrings::sFaultStickSet(mPlural) );
    uiMenu* impfltset = new uiMenu( &appl_, uiStrings::sFaultSet( mPlural ) );
    uiMenu* impwell = new uiMenu( &appl_, uiStrings::sWell(mPlural) );
    uiMenu* imppick = new uiMenu( &appl_, tr("PointSets/Polygons") );
    uiMenu* impwvlt = new uiMenu( &appl_, tr("Wavelets") );
    uiMenu* impmute = new uiMenu( &appl_, tr("Mute Functions") );
    uiMenu* impcpd = new uiMenu( &appl_, tr("Cross-plot Data") );
    uiMenu* impvelfn = new uiMenu( &appl_, tr("Velocity Functions") );
    uiMenu* imppdf = new uiMenu( &appl_, tr("Probability Density Functions") );
    uiMenu* impgeom2d = new uiMenu( &appl_,tr("Navigation Data / 2D Geometry"));

    impmnu_->addMenu( impattr );
    insertAction( impmnu_, m3Dots(uiStrings::sColorTable()), mImpColTabMnuItm );
    impmnu_->addMenu( impcpd );
    impmnu_->addMenu( impfault );
    impmnu_->addMenu( impfaultstick );
    impmnu_->addMenu( impfltset );
    impmnu_->addMenu( imphor );
    impmnu_->addMenu( impmute );
    impmnu_->addMenu( impgeom2d );
    impmnu_->addMenu( imppick );
    impmnu_->addMenu( imppdf );
    impmnu_->addMenu( impseis );
    impmnu_->addMenu( impvelfn );
    impmnu_->addMenu( impwvlt );
    impmnu_->addMenu( impwell );
    impmnu_->insertSeparator();

    const uiString ascii = m3Dots( uiStrings::sASCII() );

    insertAction( impattr, ascii, mImpAttrMnuItm, ascic );
    insertAction( imppick, ascii, mImpPickAsciiMnuItm, ascic );
    insertAction( impwvlt, ascii, mImpWvltAsciiMnuItm, ascic );
    insertAction( impmute, ascii, mImpMuteDefAsciiMnuItm, ascic );
    insertAction( impcpd, ascii, mImpPVDSAsciiMnuItm, ascic );
    insertAction( impvelfn, ascii, mImpVelocityAsciiMnuItm, ascic );
    insertAction( imppdf, m3Dots(tr("ASCII (RokDoc)")),
		       mImpPDFAsciiMnuItm, ascic );

    const bool have2d = SI().has2D(); const bool only2d = !SI().has3D();
    uiMenu* impseissimple = new uiMenu( &appl_, tr("Simple File") );
    if ( have2d )
    {
	insertAction( impseissimple,
		only2d ? m3Dots(tr("Line")) : m3Dots(uiStrings::s2D()),
		mImpSeisSimple2DMnuItm, "seismicline2d" );
	insertAction( impseissimple, only2d
		? m3Dots(tr("Prestack Data")) : m3Dots(tr("Prestack 2D")),
		mImpSeisSimplePS2DMnuItm, "prestackdataset2d" );
    }
    if ( !only2d )
    {
	insertAction( impseissimple,
			   have2d ? m3Dots(uiStrings::s3D())
				  : m3Dots(uiStrings::sVolume()),
			   mImpSeisSimple3DMnuItm, "seismiccube" );
	insertAction( impseissimple,
			   have2d ? m3Dots(tr("Prestack 3D"))
				  : m3Dots(tr("Prestack Volume")),
			   mImpSeisSimplePS3DMnuItm, "prestackdataset" );
    }
    impseis->addMenu( impseissimple );

    uiMenu* impcbvsseis = new uiMenu( &appl_, tr("CBVS"), "od" );
    insertAction( impcbvsseis, m3Dots(tr("From File")),
		 mImpSeisCBVSMnuItm );
    insertAction( impcbvsseis, m3Dots(tr("From Other Survey")),
		 mImpSeisCBVSOtherSurvMnuItm );
    impseis->addMenu( impcbvsseis );

    if ( have2d )
    {
	uiMenu* imphor2Dasc = new uiMenu( &appl_, tr("ASCII 2D"), ascic );
	insertAction( imphor2Dasc, m3Dots(tr("Single 2D Horizon")),
	    mImpHor2DAsciiMnuItm );
	insertAction( imphor2Dasc, m3Dots(tr("Bulk 2D Horizon")),
	    mImpBulkHor2DAsciiMnuItm );
	imphor->addMenu( imphor2Dasc );
    }

    uiMenu* imphor3Dasc = new uiMenu( &appl_, tr("ASCII 3D"), ascic );
    insertAction( imphor3Dasc, m3Dots(tr("Single 3D Horizon")),
	mImpHorAsciiMnuItm );
    insertAction( imphor3Dasc, m3Dots(tr("Bulk 3D Horizon")),
	mImpBulkHorAsciiMnuIm );
    imphor->addMenu( imphor3Dasc );
    insertAction( imphor, m3Dots(tr("From ZMap")), mImpHorZMapMnuItm );
    insertAction( imphor, have2d ? m3Dots(tr("Attributes 3D")) :
		   m3Dots(tr("Attributes")), mImpHorAsciiAttribMnuItm, ascic );

    uiMenu* impfltasc = new uiMenu( &appl_, uiStrings::sASCII(), ascic );
    insertAction( impfltasc, m3Dots(tr("Single Fault")), mImpFaultMnuItm );
    insertAction( impfltasc, m3Dots(tr("Bulk Faults")), mImpFaultBulkMnuItm );
    impfault->addMenu( impfltasc );

    if ( have2d )
    {
	uiMenu* impfltss2Dasc = new uiMenu( &appl_, tr("ASCII 2D"), ascic );
	insertAction( impfltss2Dasc, m3Dots(tr("Single 2D FaultStickSet")),
	    mImpFaultSSAscii2DMnuItm );
	insertAction( impfltss2Dasc, m3Dots(tr("Bulk 2D FaultStickSets")),
	    mImpFaultSSAscii2DBulkMnuItm );
	impfaultstick->addMenu( impfltss2Dasc );
    }

    uiMenu* impfltss3Dasc = new uiMenu( &appl_, tr("ASCII 3D"), ascic );
    insertAction( impfltss3Dasc, m3Dots(tr("Single 3D FaultStickSet")),
	mImpFaultSSAscii3DMnuItm );
    insertAction( impfltss3Dasc, m3Dots(tr("Bulk 3D FaultStickSets")),
	mImpFaultSSAscii3DBulkMnuItm );
    impfaultstick->addMenu( impfltss3Dasc );

    insertAction( impfltset, m3Dots( uiStrings::sASCII() ),
		 mImpFltSetAsciiMnuItm, ascic );

    uiMenu* impwellasc = new uiMenu( &appl_, uiStrings::sASCII(), ascic );
    insertAction( impwellasc, m3Dots(uiStrings::sTrack()),
		 mImpWellAsciiTrackMnuItm );
    insertAction( impwellasc, m3Dots(uiStrings::sLogs()),
		 mImpWellAsciiLogsMnuItm );
    insertAction( impwellasc, m3Dots(uiStrings::sMarker(mPlural)),
		 mImpWellAsciiMarkersMnuItm );
    impwell->addMenu( impwellasc );
    insertAction( impwell, m3Dots(tr("Well locations")),
		 mImpWellSimpleMnuItm );

    uiMenu* impwellbulk = new uiMenu( &appl_, tr("Bulk") );
    insertAction( impwellbulk, m3Dots(uiStrings::sTrack()),
		 mImpBulkWellTrackItm );
    insertAction( impwellbulk, m3Dots(tr("Directional wells")),
		  mImpBulkDirWellItm );
    insertAction( impwellbulk, m3Dots(uiStrings::sLogs()),
		 mImpBulkWellLogsItm );
    insertAction( impwellbulk, m3Dots(uiStrings::sMarker(mPlural)),
		 mImpBulkWellMarkersItm );
    insertAction( impwellbulk, m3Dots(tr("Depth/Time Model")),
		 mImpBulkWellD2TItm );
    impwell->addMenu( impwellbulk );

    insertAction( impgeom2d, ascii, mImpGeom2DAsciiMnuItm );
#ifdef __debug__
    insertAction( impgeom2d, m3Dots(tr("SEG P1")), mImpGeom2DSEGP1MnuItm );
#endif

// Fill impmenus_
    impmnus_.erase();
    impmnus_.allowNull();
    for ( int idx=0; idx<uiODApplMgr::NrObjTypes; idx++ )
	impmnus_ += 0;

#define mAddImpMnu(tp,mnu) impmnus_.replace( (int)uiODApplMgr::tp, mnu )
    mAddImpMnu( Seis, impseis );
    mAddImpMnu( Hor, imphor );
    mAddImpMnu( Flt, impfault );
    mAddImpMnu( Fltss, impfaultstick );
	mAddImpMnu( FltSet, impfltset );
	mAddImpMnu( Wll, impwell );
    mAddImpMnu( Attr, impattr );
    mAddImpMnu( Pick, imppick );
    mAddImpMnu( Wvlt, impwvlt );
    mAddImpMnu( MDef, impmute );
    mAddImpMnu( Vel, impvelfn );
    mAddImpMnu( PDF, imppdf );
}


void uiODMenuMgr::fillExportMenu()
{
    expmnu_->clear();
    uiMenu* expseis = new uiMenu( &appl_, uiStrings::sSeismicData() );
    uiMenu* exphor = new uiMenu( &appl_, uiStrings::sHorizon(mPlural) );
    uiMenu* expflt = new uiMenu( &appl_, uiStrings::sFault(mPlural) );
    uiMenu* expfltss = new uiMenu( &appl_, uiStrings::sFaultStickSet(mPlural) );
    uiMenu* expfltset = new uiMenu( &appl_, uiStrings::sFaultSet(mPlural) );
    uiMenu* expgeom2d = new uiMenu( &appl_, tr("Geometry 2D") );
    uiMenu* exppick = new uiMenu( &appl_, tr("PointSets/Polygons") );
    uiMenu* expwvlt = new uiMenu( &appl_, tr("Wavelets") );
    uiMenu* expmute = new uiMenu( &appl_, tr("Mute Functions") );
    uiMenu* exppdf =
	new uiMenu( &appl_, tr("Probability Density Functions") );
    uiMenu* expwell = new uiMenu( &appl_, uiStrings::sWell(mPlural) );

    expmnu_->addMenu( expflt );
    expmnu_->addMenu( expfltss );
    expmnu_->addMenu( expfltset );
    expmnu_->addMenu( expgeom2d );
    expmnu_->addMenu( exphor );
    expmnu_->addMenu( expmute );
    expmnu_->addMenu( exppick );
    expmnu_->addMenu( exppdf );
    expmnu_->addMenu( expseis );
    insertAction( expmnu_, m3Dots(tr("Survey Setup")), mExpSurveySetupItm );
    expmnu_->addMenu( expwvlt );
    expmnu_->addMenu( expwell );
    expmnu_->insertSeparator();

    const bool have2d = SI().has2D(); const bool only2d = !SI().has3D();
    uiMenu* expseissimple = new uiMenu( &appl_, uiStrings::sASCII(), ascic );
    if ( have2d )
    {
	insertAction( expseissimple, only2d ? m3Dots(tr("Line"))
			: m3Dots(uiStrings::s2D()), mExpSeisSimple2DMnuItm,
			"seismicline2d"	);
	insertAction( expseissimple, only2d ? m3Dots(tr("Prestack Data"))
			: m3Dots(tr("Prestack 2D")), mExpSeisSimplePS2DMnuItm,
			"prestackdataset2d" );
    }
    if ( !only2d )
    {
	insertAction( expseissimple, have2d ? m3Dots(uiStrings::s3D())
		   : m3Dots(uiStrings::sVolume()), mExpSeisSimple3DMnuItm,
		   "seismiccube" );
	insertAction( expseissimple, have2d ? m3Dots(tr("Prestack 3D"))
		   : m3Dots(tr("Prestack Volume")), mExpSeisSimplePS3DMnuItm
		   , "prestackdataset" );
    }
    expseis->addMenu( expseissimple );
    if ( !only2d )
    {
	insertAction( expseis, m3Dots(tr("Cube Positions")),
		     mExpSeisCubePositionsMnuItm );
    }

    const uiString sascii = m3Dots(uiStrings::sASCII());

    if ( have2d )
    {
	uiMenu* exphor2Dasc = new uiMenu( &appl_, tr("ASCII 2D"), ascic );
	insertAction( exphor2Dasc, m3Dots(tr("Single 2D Horizon")),
		    mExpHorAscii2DMnuItm );
	insertAction( exphor2Dasc, m3Dots(tr("Bulk 2D Horizons")),
				mExpBulkHorAscii2DMnuItm );
	exphor->addMenu( exphor2Dasc );
    }

    uiMenu* exphor3Dasc = new uiMenu( &appl_, tr("ASCII 3D"), ascic );
    insertAction( exphor3Dasc, m3Dots(tr("Single 3D Horizon")),
				    mExpHorAscii3DMnuItm );
    insertAction( exphor3Dasc, m3Dots(tr("Bulk 3D Horizons")),
				    mExpBulkHorAscii3DMnuItm );
    exphor->addMenu( exphor3Dasc );
    uiMenu* expfltasc = new uiMenu( &appl_, uiStrings::sASCII(), ascic );
    insertAction( expfltasc, m3Dots(tr("Single Fault")), mExpFltAsciiMnuItm );
    insertAction( expfltasc, m3Dots(tr("Bulk Faults")),
		  mExpBulkFltAsciiMnuItm );
    expflt->addMenu( expfltasc );
    uiMenu* expfltssasc = new uiMenu( &appl_, uiStrings::sASCII(), ascic );
    insertAction( expfltssasc, m3Dots(tr("Single FaultStickSet")),
	mExpFltSSAsciiMnuItm );
    insertAction( expfltssasc, m3Dots(tr("Bulk FaultStickSets")),
	mExpBulkFltSSAsciiMnuItm );
    expfltss->addMenu( expfltssasc );
    insertAction( expfltset, m3Dots(uiStrings::sASCII()),
		  mExpFltSetAsciiMnuItm, ascic );
    insertAction( expgeom2d, sascii, mExpGeom2DMnuItm, ascic );
    insertAction( exppick, sascii, mExpPickAsciiMnuItm, ascic );
    insertAction( expwvlt, sascii, mExpWvltAsciiMnuItm, ascic );
    insertAction( expmute, sascii, mExpMuteDefAsciiMnuItm, ascic );
    insertAction( exppdf, m3Dots(tr("ASCII (RokDoc)")),
			 mExpPDFAsciiMnuItm, ascic );
    insertAction( expwell, m3Dots(tr("Logs")), mExpLogLAS, ascic );

// Fill expmenus_
    expmnus_.erase();
    expmnus_.allowNull();
    for ( int idx=0; idx<uiODApplMgr::NrObjTypes; idx++ )
	expmnus_ += 0;

#define mAddExpMnu(tp,mnu) expmnus_.replace( (int)uiODApplMgr::tp, mnu )
    mAddExpMnu( Seis, expseis );
    mAddExpMnu( Hor, exphor );
    mAddExpMnu( Flt, expflt );
    mAddExpMnu( Fltss, expfltss );
    mAddExpMnu( FltSet, expfltset );
    mAddExpMnu( Pick, exppick );
    mAddExpMnu( Wvlt, expwvlt );
    mAddExpMnu( MDef, expmute );
    mAddExpMnu( PDF, exppdf );
    mAddExpMnu( Geom, expgeom2d );
    mAddExpMnu( Wll, expwell );
}


void uiODMenuMgr::fillManMenu()
{
    manmnu_->clear();
    add2D3DMenuItem( *manmnu_, "man_attrs", tr("Attribute Sets"),
			mManAttr2DMnuItm, mManAttr3DMnuItm );
    insertAction( manmnu_, m3Dots(tr("Bodies")),
		  mManBodyMnuItm, "man_body" );
    insertAction( manmnu_, m3Dots(tr("Color Tables")),
		  mManColTabMnuItm, "empty" );
    insertAction( manmnu_, m3Dots(tr("Cross-plot Data")),
		  mManCrossPlotItm, "manxplot" );
    insertAction( manmnu_, m3Dots(uiStrings::sFault(mPlural)),
		  mManFaultMnuItm, "man_flt" );
    insertAction( manmnu_, m3Dots(uiStrings::sFaultStickSet(mPlural)),
		  mManFaultStickMnuItm, "man_fltss" );
    if ( SI().has3D() )
	insertAction( manmnu_, m3Dots(uiStrings::sFaultSet(mPlural)),
		      mManFaultSetMnuItm, "man_fltset" );

    insertAction( manmnu_, m3Dots(tr("Geometry 2D")), mManGeomItm, "man2dgeom");
    if ( SI().survDataType() == SurveyInfo::No2D )
	insertAction( manmnu_, m3Dots(uiStrings::sHorizon(mPlural)),
		      mManHor3DMnuItm, "man_hor" );
    else
    {
	uiMenu* mnu = new uiMenu( &appl_,
				  uiStrings::sHorizon(mPlural),"man_hor");
	insertAction( mnu, m3Dots(uiStrings::s2D()),
		      mManHor2DMnuItm );
	insertAction( mnu, m3Dots(uiStrings::s3D()),
		      mManHor3DMnuItm );
	manmnu_->addMenu( mnu );
    }

    insertAction( manmnu_, m3Dots(tr("Layer Properties")), mManPropsMnuItm,
			"man_props" );
    insertAction( manmnu_, m3Dots(tr("PointSets/Polygons")),mManPickMnuItm,
			"man_picks" );
    insertAction( manmnu_, m3Dots(tr("Probability Density Functions")),
		 mManPDFMnuItm, "man_prdfs" );
    insertAction( manmnu_, m3Dots(tr("Random Lines")), mManRanLMnuItm,
			"empty" );
    add2D3DMenuItem( *manmnu_, "man_seis", uiStrings::sSeismicData(),
			mManSeis2DMnuItm, mManSeis3DMnuItm );
    add2D3DMenuItem( *manmnu_, "man_ps", tr("Prestack Seismic Data"),
			mManSeisPS2DMnuItm, mManSeisPS3DMnuItm );
    insertAction( manmnu_, m3Dots(tr("Sessions")),
			mManSessMnuItm, "empty" );
    insertAction( manmnu_,
			m3Dots(uiStrings::sStratigraphy()),
			mManStratMnuItm, "man_strat" );
    insertAction( manmnu_, m3Dots(uiStrings::sWavelet(mPlural)),
			mManWvltMnuItm, "man_wvlt" );
    insertAction( manmnu_, m3Dots(uiStrings::sWell(mPlural)),
			mManWellMnuItm, "man_wll" );
    manmnu_->insertSeparator();
}


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

    csoitm_ = new uiMenu( &appl_, tr("Create Seismic Output") );

// Attributes
    uiMenu* attritm = new uiMenu( uiStrings::sAttribute(mPlural) );
    csoitm_->addMenu( attritm );

    add2D3DMenuItem( *attritm, "seisout", tr("Single Attribute"),
		     mSeisOut2DMnuItm, mSeisOut3DMnuItm );
    if ( SI().has3D() )
    {
	attritm->insertAction(
	    new uiAction( m3Dots(tr("Multi Attribute Volume")),
			mCB(&applMgr(),uiODApplMgr,createMultiAttribVol)) );
	attritm->insertAction(
	    new uiAction( m3Dots(tr("MultiCube DataStore")),
			mCB(&applMgr(),uiODApplMgr,createMultiCubeDS)) );
    }

    add2D3DMenuItem( *attritm, "alonghor", tr("Along Horizon"),
		     mCompAlongHor2DMnuItm, mCompAlongHor3DMnuItm );
    add2D3DMenuItem( *attritm, "betweenhors", tr("Between Horizons"),
		     mCompBetweenHor2DMnuItm, mCompBetweenHor3DMnuItm );


// 2D <-> 3D
    uiMenu* itm2d3d = 0;
    const uiString menutext = tr("2D <=> 3D");
    if ( SI().has3D() )
    {
	itm2d3d = new uiMenu( menutext );
	csoitm_->addMenu( itm2d3d );
	insertAction( itm2d3d, m3Dots(tr("Create 2D from 3D")),
		     mCreate2DFrom3DMnuItm );
	insertAction( itm2d3d, m3Dots(tr("Extract 2D from 3D")),
		     m2DFrom3DMnuItm );
    }
    const bool show3dfrom2d =
		GetEnvVarYN( "OD_CREATE_3D_FROM_2D" ) && SI().has2D();
    if ( show3dfrom2d )
    {
	if ( !itm2d3d )
	{
	    itm2d3d = new uiMenu( menutext );
	    csoitm_->addMenu( itm2d3d );
	}
	insertAction( itm2d3d, m3Dots(tr("Create 3D from 2D")),
							    m3DFrom2DMnuItm );
	insertAction( itm2d3d, m3Dots(tr("Interpolate 3D from 2D")),
						    m3DFrom2DInterPolMnuItm);
    }


    if ( SI().has3D() )
    {
// Other 3D items
	csoitm_->insertAction(
	    new uiAction(m3Dots(tr("Angle Mute Function")),
			mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) ));
	csoitm_->insertAction(
	    new uiAction(m3Dots(tr("Bayesian Classification")),
			mCB(&applMgr(),uiODApplMgr,bayesClass3D), "bayes"));
	csoitm_->insertAction(
	    new uiAction(m3Dots(tr("From Well Logs")),
			mCB(&applMgr(),uiODApplMgr,createCubeFromWells) ));
    }

    add2D3DMenuItem( *csoitm_, "empty", tr("Prestack Processing"),
		     mPSProc2DMnuItm, mPSProc3DMnuItm );

// Velocity
    uiMenu* velitm = new uiMenu( tr("Velocity") );
    csoitm_->addMenu( velitm );
    add2D3DMenuItem( *velitm, "empty", tr("Time - Depth Conversion"),
		     mT2DConv2DMnuItm, mT2DConv3DMnuItm );
    if ( SI().has3D() )
    {
	velitm->insertAction(
	    new uiAction(m3Dots(tr("Velocity Conversion")),
			 mCB(&applMgr(),uiODApplMgr,processVelConv)) );
    }

    add2D3DMenuItem( *csoitm_, "empty", tr("Volume Builder"),
		     mVolProc2DMnuItm, mVolProc3DMnuItm );

    procmnu_->addMenu( csoitm_ );

    uiMenu* grditm = new uiMenu( &appl_, tr("Create Horizon Output") );
    add2D3DMenuItem( *grditm, "", uiStrings::sAttribute(mPlural),
		     mCreateSurf2DMnuItm, mCreateSurf3DMnuItm );
    procmnu_->addMenu( grditm );

    procwellmnu_ = new uiMenu( &appl_, uiStrings::sWells(), "well" );
    procwellmnu_->insertAction( new uiAction(m3Dots(uiStrings::sRockPhy()),
		mCB(&applMgr(),uiODApplMgr,launchRockPhysics),"rockphys") );
    procmnu_->addMenu( procwellmnu_ );

    insertAction( procmnu_, m3Dots(tr("(Re-)Start Batch Job")),
		 mStartBatchJobMnuItm );

    procmnu_->insertSeparator();
}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();
    SurveyInfo::Pol2D survtype = SI().survDataType();
    const char* attrpm = "attributes";
    if ( survtype == SurveyInfo::Both2DAnd3D )
    {
	uiMenu* aitm = new uiMenu( &appl_, uiStrings::sAttribute(mPlural),
				   attrpm );
	insertAction( aitm, m3Dots(uiStrings::s2D()), mEdit2DAttrMnuItm,
			   "attributes_2d" );
	insertAction( aitm, m3Dots(uiStrings::s3D()), mEdit3DAttrMnuItm,
			   "attributes_3d" );

	analmnu_->addMenu( aitm );
	analmnu_->insertSeparator();
    }
    else
    {
	insertAction( analmnu_, m3Dots(uiStrings::sAttribute(mPlural)),
			   mEditAttrMnuItm, attrpm );
	analmnu_->insertSeparator();
    }

    add2D3DMenuItem( *analmnu_, VolProc::uiChain::pixmapFileName(),
		     tr("Volume Builder"),
		     mCB(&applMgr(),uiODApplMgr,doVolProc2DCB),
		     mCB(&applMgr(),uiODApplMgr,doVolProcCB) );

    uiMenu* xplotmnu = new uiMenu( &appl_, tr("Cross-plot Data"),
				   "xplot");
    insertAction( xplotmnu, m3Dots(tr("Well logs vs Attributes")),
		       mXplotMnuItm, "xplot_wells" );
    insertAction( xplotmnu, m3Dots(tr("Attributes vs Attributes")),
		       mAXplotMnuItm, "xplot_attribs" );
    insertAction( xplotmnu, m3Dots(tr("Open Saved Cross-plot")),
		  mOpenXplotMnuItm );
    analmnu_->addMenu( xplotmnu );

    analwellmnu_ = new uiMenu( &appl_, uiStrings::sWells(), "well" );
    analwellmnu_->insertAction( new uiAction( m3Dots(tr("Edit Logs")),
	mCB(&applMgr(),uiODApplMgr,doWellLogTools), "well_props" ) );
    if (  SI().zIsTime() )
	analwellmnu_->insertAction(
	    new uiAction( m3Dots(tr("Tie Well to Seismic")),
		    mCB(&applMgr(),uiODApplMgr,tieWellToSeismic), "well_tie" ));
    analmnu_->addMenu( analwellmnu_ );

    layermodelmnu_ = new uiMenu( &appl_, tr("Layer Modeling"),
				 "stratlayermodeling" );
    layermodelmnu_->insertAction( new uiAction( m3Dots(tr("Basic")),
	mCB(&applMgr(),uiODApplMgr,doLayerModeling), "empty") );
    analmnu_->addMenu( layermodelmnu_ );
    analmnu_->insertSeparator();
}


void uiODMenuMgr::fillSceneMenu()
{
    scenemnu_->clear();
    insertAction( scenemnu_, uiStrings::sNew(), mAddSceneMnuItm );
    insertAction( scenemnu_, tr("New Map View"), mAddMapSceneMnuItm );

    uiString itmtxt = tr( "New [%1]" )
	      .arg( SI().zIsTime() ? uiStrings::sDepth() : uiStrings::sTime() );
    add2D3DMenuItem( *scenemnu_, "empty", itmtxt, mAddTimeDepth2DMnuItm,
						  mAddTimeDepth3DMnuItm );

    add2D3DMenuItem( *scenemnu_, "empty", tr("New [Horizon Flattened]"),
		     mAddHorFlat2DMnuItm, mAddHorFlat3DMnuItm );
    lastsceneitm_ = scenemnu_->insertSeparator();

    insertAction( scenemnu_, tr("Cascade"), mCascadeMnuItm );
    uiMenu* tileitm = new uiMenu( &appl_, uiStrings::sTile() );
    scenemnu_->addMenu( tileitm );

    insertAction( tileitm, tr("Auto"), mTileAutoMnuItm );
    insertAction( tileitm, uiStrings::sHorizontal(), mTileHorMnuItm );
    insertAction( tileitm, uiStrings::sVertical(), mTileVerMnuItm );

    insertAction( scenemnu_, m3Dots(uiStrings::sProperties()),
		 mScenePropMnuItm );
    scenemnu_->insertSeparator();

    updateSceneMenu();
}


void uiODMenuMgr::insertNewSceneItem( uiAction* action, int id )
{
    scenemnu_->insertAction( action, id, lastsceneitm_ );
    lastsceneitm_ = action;
}


void uiODMenuMgr::insertNewSceneItem( uiMenu* mnu )
{
    if ( !mnu )
	return;

    uiAction* submenuitem = new uiAction( mnu->text() );
    submenuitem->setMenu( mnu );
    scenemnu_->insertAction( submenuitem, -1, lastsceneitm_ );
    lastsceneitm_ = submenuitem;
}


void uiODMenuMgr::updateSceneMenu()
{
    uiStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int idx=0; idx<=scenenms.size(); idx++ )
    {
	const int id = mSceneSelMnuItm + idx;
	const uiAction* itm = scenemnu_->findAction( id );

	if ( idx >= scenenms.size() )
	{
	    if ( itm )
		scenemnu_->removeItem( id );
	    continue;
	}

	if ( !itm )
	{
	    auto* newitm = new uiAction( uiString::emptyString(),
				mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertAction( newitm, id );
	    newitm->setCheckable( true );
	    itm = newitm;
	}

	uiAction* edititm = const_cast<uiAction*>( itm );
	edititm->setText( scenenms[idx]);
	edititm->setChecked( idx==activescene );
    }
}


void uiODMenuMgr::fillViewMenu()
{
    viewmnu_->clear();
    insertAction( viewmnu_, m3Dots(tr("Work Area")), mWorkAreaMnuItm );
    insertAction( viewmnu_, m3Dots(tr("Z-Scale")), mZScaleMnuItm );
    insertAction( viewmnu_, m3Dots(tr("Viewer 2D")), m2DViewMnuItm );
    uiMenu* stereoitm = new uiMenu( &appl_, tr("Stereo Viewing") );
    viewmnu_->addMenu( stereoitm );

#define mInsertStereoItem(itm,txt,icn,docheck,id) \
    itm = new uiAction( txt, mCB(this,uiODMenuMgr,handleClick),icn ); \
    stereoitm->insertAction( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, tr("Off"), "off", true, mStereoOffMnuItm)
    mInsertStereoItem( stereoredcyanitm_, tr("Red/Cyan"), "redcyan", false,
			mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, tr("Quad Buffer"), "quadbuff", false,
			mStereoQuadMnuItm )

    stereooffsetitm_ = new uiAction( m3Dots(tr("Stereo Offset")),
				     mCB(this,uiODMenuMgr,handleClick),
				     "stereooffset" );
    stereoitm->insertAction( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.setName("Toolbars");
    viewmnu_->addMenu( &toolbarsmnu );

    showtreeitm_ = new uiAction( tr("Show tree"),
				mCB(this,uiODMenuMgr,toggleTreeMode) );
    viewmnu_->insertAction( showtreeitm_ );
    showtreeitm_->setCheckable( true );
    showtreeitm_->setChecked( true );
    showtreeitm_->setShortcut( "F11" );
}


void uiODMenuMgr::toggleTreeMode( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb)
    const bool ison = action ? action->isChecked() : true;
    sceneMgr().showTree( ison );
}


void uiODMenuMgr::addIconMnuItems( const DirList& dl, uiMenu* iconsmnu,
				   BufferStringSet& nms )
{
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get( idx ).buf() + 6 );
	if ( nm.isEmpty() || nms.isPresent(nm) )
	    continue;

	BufferString mnunm( "&" ); mnunm += nm;
    insertAction( iconsmnu, toUiString(mnunm), mViewIconsMnuItm+nms.size() );
	nms.add( nm );
    }
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dlsett( GetSettingsDir(), File::DirsInDir, "icons.*" );
    DirList dlsite( mGetApplSetupDataDir(), File::DirsInDir, "icons.*" );
    DirList dlrel( mGetSWDirDataDir(), File::DirsInDir, "icons.*" );
    if ( dlsett.size() + dlsite.size() + dlrel.size() < 2 )
	return;

    uiMenu* iconsmnu = new uiMenu( &appl_, tr("Icons") );
    viewmnu_->addMenu( iconsmnu );
    insertAction( iconsmnu, tr("Default"), mViewIconsMnuItm+0 );
    BufferStringSet nms; nms.add( "Default" );
    addIconMnuItems( dlsett, iconsmnu, nms );
    addIconMnuItems( dlsite, iconsmnu, nms );
    addIconMnuItems( dlrel, iconsmnu, nms );
}


void uiODMenuMgr::fillUtilMenu()
{
#ifdef __mac__
    // Qt disables the 'Settings' menu on Mac, hence a different text
    settmnu_ = new uiMenu( &appl_, tr("User Settings") );
#else
    settmnu_ = new uiMenu( &appl_, uiStrings::sSettings() );
#endif

    utilmnu_->addMenu( settmnu_ );

    insertAction( settmnu_, m3Dots(tr("Look and Feel")), mSettLkNFlMnuItm );
    insertAction( settmnu_, m3Dots(tr("Keyboard Shortcuts")),
		 mSettShortcutsMnuItm);

#ifdef __debug__
    const bool defenabadvsettings = true;
#else
    const bool defenabadvsettings = false;
#endif
    const bool enabadvsettings = GetEnvVarYN( "OD_ENABLE_ADVANCED_SETTINGS",
		defenabadvsettings );
    if ( enabadvsettings )
    {
	uiMenu* advmnu = new uiMenu( &appl_, uiStrings::sAdvanced() );
	insertAction( advmnu, m3Dots(tr("Personal Settings")), mSettGeneral );
	insertAction( advmnu, m3Dots(tr("Survey Defaults")), mSettSurvey );
	settmnu_->addMenu( advmnu );
    }

    toolsmnu_ = new uiMenu( &appl_, uiStrings::sTools() );
    utilmnu_->addMenu( toolsmnu_ );

    insertAction( toolsmnu_, m3Dots(tr("Batch Programs")), mBatchProgMnuItm );
    insertAction( toolsmnu_, m3Dots(tr("Position Conversion")),
		  mPosconvMnuItm, "crs" );

    installmnu_ = new uiMenu( &appl_, tr("Installation") );
    utilmnu_->addMenu( installmnu_ );
    FilePath installerdir( ODInst::GetInstallerDir() );
    const bool hasinstaller = File::isDirectory( installerdir.fullPath() );
    if ( hasinstaller )
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	const bool aitfixed = ODInst::autoInstTypeIsFixed();
	if ( !aitfixed || ait == ODInst::UseManager || ait == ODInst::FullAuto )
	    insertAction( installmnu_, m3Dots(tr("Update")),
			 mInstMgrMnuItem );
	if ( !aitfixed )
	    insertAction( installmnu_, m3Dots(tr("Auto-update Policy")),
			 mInstAutoUpdPolMnuItm );
	installmnu_->insertSeparator();
    }

    insertAction( installmnu_, m3Dots(tr("Python Settings")), mSettPython,
		  "python" );
    insertAction( installmnu_, m3Dots(tr("Connection Settings")),
		 mInstConnSettsMnuItm );
    insertAction( installmnu_, m3Dots(tr("Plugins")), mPluginsMnuItm );
    insertAction( installmnu_, m3Dots(tr("Setup Distributed Computing")),
		 mSetupBatchItm);
    insertAction( installmnu_, tr("Graphics Information"), mGraphicsInfoItm );
    insertAction( installmnu_, tr("Show HostID"), mHostIDInfoItm );

    if ( __iswin__ )
	insertAction( installmnu_, m3Dots(tr("Firewall Management")),
			mFirewallProcItm );

    BufferString develverstr;
    GetSpecificODVersion( "devel", develverstr );
    if ( !develverstr.isEmpty() )
	insertAction( utilmnu_, m3Dots(tr("Plugin Development")),
		     mCrDevEnvMnuItm );

    const char* lmfnm = od_ostream::logStream().fileName();
    if ( lmfnm && *lmfnm )
	insertAction( utilmnu_, m3Dots(tr("Show Log File")),mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	insertAction( toolsmnu_, m3Dots(tr("Display DataPack Info")),
		     mDumpDataPacksMnuItm);
	insertAction( toolsmnu_, m3Dots(tr("Display Memory Info")),
		     mDisplayMemoryMnuItm);
	insertAction( toolsmnu_, m3Dots(tr("Display Well::MGR Info")),
		      mDisplayWellMgrMnuItm);
    }
}


int uiODMenuMgr::add2D3DToolButton( uiToolBar& tb, const char* iconnm,
				    const uiString& tt,
				    const CallBack& cb2d, const CallBack& cb3d,
				    int itmid2d, int itmid3d )
{
    if ( !SI().has2D() )
	return tb.addButton( iconnm, tt, cb3d, false, itmid3d );

    if ( !SI().has3D() )
	return tb.addButton( iconnm, tt, cb2d, false, itmid2d );

    const int butid = tb.addButton( iconnm, tt );
    uiMenu* popmnu = new uiMenu( tb.parent(), uiStrings::sAction() );
    popmnu->insertAction( new uiAction(m3Dots(uiStrings::s2D()),cb2d),
			itmid2d );
    popmnu->insertAction( new uiAction(m3Dots(uiStrings::s3D()),cb3d),
			itmid3d );
    tb.setButtonMenu( butid, popmnu, uiToolButton::InstantPopup );
    return butid;
}


void uiODMenuMgr::add2D3DMenuItem( uiMenu& menu, const char* iconnm,
				   const uiString& itmtxt,
				   const CallBack& cb2d, const CallBack& cb3d,
				   int itmid2d, int itmid3d )
{
    if ( SI().has2D() && SI().has3D() )
    {
	uiMenu* mnu = new uiMenu( itmtxt, iconnm );
	mnu->insertAction( new uiAction(m3Dots(uiStrings::s2D()),cb2d),itmid2d);
	mnu->insertAction( new uiAction(m3Dots(uiStrings::s3D()),cb3d),itmid3d);
	menu.addMenu( mnu );
    }
    else
    {
	uiString titledots( itmtxt );
	titledots.append( " ..." );
	if ( SI().has2D() )
	    menu.insertAction( new uiAction(titledots,cb2d,iconnm), itmid2d );
	else if ( SI().has3D() )
	    menu.insertAction( new uiAction(titledots,cb3d,iconnm), itmid3d );
    }
}


void uiODMenuMgr::add2D3DMenuItem( uiMenu& menu, const char* iconnm,
				   const uiString& itmtxt,
				   int itmid2d, int itmid3d )
{
    const CallBack cb = mCB(this,uiODMenuMgr,handleClick);
    add2D3DMenuItem( menu, iconnm, itmtxt, cb, cb, itmid2d, itmid3d );
}



#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(appman,uiODApplMgr,fn), togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey",tr("Survey Setup"),false,manSurvCB);
    add2D3DToolButton( *dtecttb_, "attributes", tr("Edit Attributes"),
		       mCB(appman,uiODApplMgr,editAttr2DCB),
		       mCB(appman,uiODApplMgr,editAttr3DCB) );
    add2D3DToolButton( *dtecttb_, "seisout", tr("Create Seismic Output"),
		       mCB(appman,uiODApplMgr,seisOut2DCB),
		       mCB(appman,uiODApplMgr,seisOut3DCB) );

    add2D3DToolButton( *dtecttb_,VolProc::uiChain::pixmapFileName(),
		tr("Volume Builder"),
		mCB(appman,uiODApplMgr,doVolProc2DCB),
		mCB(appman,uiODApplMgr,doVolProcCB) );

    const int xplotid = dtecttb_->addButton( "xplot", uiStrings::sCrossPlot() );
    uiMenu* mnu = new uiMenu();
    mnu->insertAction(
	new uiAction(m3Dots(tr("Cross-plot Attribute vs Attribute Data")),
		     mCB(appman,uiODApplMgr,doAttribXPlot),"xplot_attribs") );
    mnu->insertAction(
	new uiAction(m3Dots(tr("Cross-plot Attribute vs Well Data")),
		     mCB(appman,uiODApplMgr,doWellXPlot),"xplot_wells") );
    dtecttb_->setButtonMenu( xplotid, mnu, uiToolButton::InstantPopup );

    mAddTB(dtecttb_,"rockphys",tr("Create New Well Logs Using Rock Physics"),
			false,launchRockPhysics);
/* Not yet
    mAddTB(dtecttb_,"2dlaunch",tr("Launch 2D Viewer"),
			false,launch2DViewer);
*/

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(this,uiODMenuMgr,fn), togg )

#define mAddPopUp( nm, txt1, txt2, itm1, itm2, mnuid ) { \
    uiMenu* popmnu = new uiMenu( &appl_, nm ); \
    popmnu->insertAction( new uiAction(txt1, \
		       mCB(this,uiODMenuMgr,handleClick)), itm1 ); \
    popmnu->insertAction( new uiAction(txt2, \
		       mCB(this,uiODMenuMgr,handleClick)), itm2 ); \
    mantb_ ->setButtonMenu( mnuid, popmnu, uiToolButton::InstantPopup ); }

#define mAddPopupMnu( mnu, txt, itm ) \
    mnu->insertAction( new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), \
									 itm );

void uiODMenuMgr::fillManTB()
{
    uiString seisstr = uiStrings::sVolDataName(true,true,false);
    manids_.seisid_ = mAddTB(mantb_,"man_seis",
	uiStrings::phrManage(seisstr),false,manSeis);
    manids_.horid_ = mAddTB(mantb_,"man_hor",
	uiStrings::phrManage(uiStrings::sHorizon(mPlural)),false,manHor);
    manids_.fltid_ = mAddTB(mantb_,"man_flt",
	uiStrings::phrManage(uiStrings::sFault(mPlural)),false,manFlt);
    manids_.wllid_ = mAddTB(mantb_,"man_wll",
	uiStrings::phrManage(uiStrings::sWells()),false,manWll);
    manids_.pickid_ = mAddTB(mantb_,"man_picks", uiStrings::phrManage(
	toUiString("%1/%2").arg(uiStrings::sPointSet(mPlural))
			   .arg(uiStrings::sPolygon(mPlural))),false,manPick);
    manids_.bodyid_ = mAddTB(mantb_,"man_body",
	uiStrings::phrManage(tr("Bodies/Regions")),false,manBody);
    manids_.wvltid_ = mAddTB(mantb_,"man_wvlt",
	uiStrings::phrManage(uiStrings::sWavelet(mPlural)),false,manWvlt);
    manids_.stratid_ = mAddTB(mantb_,"man_strat",
	uiStrings::phrManage(uiStrings::sStratigraphy()),false,manStrat);

    uiMenu* seispopmnu = new uiMenu( &appl_, tr("Seismics Menu") );
    if ( SI().has2D() )
    {
	mAddPopupMnu( seispopmnu, tr("2D Seismics"), mManSeis2DMnuItm )
	mAddPopupMnu( seispopmnu, tr("2D Prestack Seismics"),
		      mManSeisPS2DMnuItm )
    }
    if ( SI().has3D() )
    {
	mAddPopupMnu( seispopmnu, tr("3D Seismics"), mManSeis3DMnuItm )
	mAddPopupMnu( seispopmnu, tr("3D Prestack Seismics"),
		      mManSeisPS3DMnuItm )
    }
    mantb_->setButtonMenu( manids_.seisid_, seispopmnu,
			   uiToolButton::InstantPopup );

    if ( SI().survDataType() != SurveyInfo::No2D )
	mAddPopUp( tr("Horizon Menu"), tr("2D Horizons"),
		   tr("3D Horizons"),
		   mManHor2DMnuItm, mManHor3DMnuItm, manids_.horid_ );

    uiMenu* fltpopmnu = new uiMenu( &appl_, tr("Faults Menu") );
    mAddPopupMnu( fltpopmnu, uiStrings::sFault(mPlural), mManFaultMnuItm )
    mAddPopupMnu( fltpopmnu, uiStrings::sFaultStickSet(mPlural),
		  mManFaultStickMnuItm )
    if ( SI().has3D() )
	mAddPopupMnu( fltpopmnu, uiStrings::sFaultSet(mPlural),
		      mManFaultSetMnuItm )
    mantb_->setButtonMenu( manids_.fltid_, fltpopmnu,
			   uiToolButton::InstantPopup );
}


int uiODMenuMgr::manActionID( uiODApplMgr::ObjType tp ) const
{
    int id = -1;
    switch ( tp )
    {
    case uiODApplMgr::Seis: id = manids_.seisid_; break;
    case uiODApplMgr::Hor: id = manids_.horid_; break;
    case uiODApplMgr::Flt: id = manids_.fltid_; break;
    case uiODApplMgr::Wll: id = manids_.wllid_; break;
    case uiODApplMgr::Pick: id = manids_.pickid_; break;
    case uiODApplMgr::Body: id = manids_.bodyid_; break;
    case uiODApplMgr::Wvlt: id = manids_.wvltid_; break;
    case uiODApplMgr::Strat: id = manids_.stratid_; break;
    default: id = -1;
    }

    return id;
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiODMenuMgr,fn), fnm ); \
    mnu->insertAction( itm, idx ); }

void uiODMenuMgr::fillCoinTB( uiODSceneMgr* scenemgr )
{
    actviewid_ = viewtb_->addButton( "altpick", tr("Switch to View Mode"),
			mCB(this,uiODMenuMgr,toggViewMode), false );
    mAddTB(viewtb_,"home",tr("To home position"),false,toHomePos);
    mAddTB(viewtb_,"set_home",tr("Save Home Position"),false,saveHomePos);
    mAddTB(viewtb_,"view_all",tr("View All"),false,viewAll);
    cameraid_ = mAddTB(viewtb_,"perspective",
		       tr("Switch to Orthographic Camera"),
		       false,switchCameraType);

    curviewmode_ = ui3DViewer::Inl;
    bool separateviewbuttons = false;
    Settings::common().getYN( "dTect.SeparateViewButtons", separateviewbuttons);
    if ( !separateviewbuttons )
    {
	viewselectid_ = viewtb_->addButton( "cube_inl",tr("View In-line"),
				mCB(this,uiODMenuMgr,handleViewClick), false );

	uiMenu* vwmnu = new uiMenu( &appl_, tr("View Menu") );
	mAddMnuItm( vwmnu, tr("View In-line"),
		    handleViewClick, "cube_inl", 0 );
	mAddMnuItm( vwmnu, tr("View Cross-line"), handleViewClick,
		    "cube_crl", 1 );
	mAddMnuItm( vwmnu, tr("View Z"), handleViewClick, "cube_z", 2 );
	mAddMnuItm( vwmnu, tr("View North"), handleViewClick, "view_N", 3 );
	mAddMnuItm( vwmnu, tr("View North Z"), handleViewClick, "view_NZ", 4);
	viewtb_->setButtonMenu( viewselectid_, vwmnu );
	viewinlid_ = viewcrlid_ = viewzid_ = viewnid_ = viewnzid_ = -1;
    }
    else
    {
#define mAddVB(img,txt) viewtb_->addButton( img, txt, \
			mCB(this,uiODMenuMgr,handleViewClick), false );
	viewinlid_ = mAddVB( "cube_inl", tr("View In-line") );
	viewcrlid_ = mAddVB( "cube_crl", tr("View Cross-line") );
	viewzid_ = mAddVB( "cube_z", tr("View Z") );
	viewnid_ = mAddVB( "view_N", tr("View North") );
	viewnzid_ = mAddVB( "view_NZ", tr("View North Z") );
	viewselectid_ = -1;
    }

    mAddTB( viewtb_, "dir-light", tr("Set light options"), false,
	    doDirectionalLight);

    axisid_ = mAddTB(viewtb_,"axis",tr("Display Orientation Axis"),
		     true,showRotAxis);
    coltabid_ = viewtb_->addButton( "colorbar", tr("Display Color Bar"),
			    mCB(this,uiODMenuMgr,dispColorBar), true );
    uiMenu* colbarmnu = new uiMenu( &appl_, tr("ColorBar Menu") );
    mAddMnuItm( colbarmnu, m3Dots(uiStrings::sSettings()), dispColorBar,
		"disppars", 0 );
    viewtb_->setButtonMenu( coltabid_, colbarmnu );

    mAddTB(viewtb_,"snapshot",uiStrings::sTakeSnapshot(),false,mkSnapshot);
    polyselectid_ = viewtb_->addButton( "polygonselect",
		tr("Polygon Selection mode"),
		mCB(this,uiODMenuMgr,selectionMode), true );
    uiMenu* mnu = new uiMenu( &appl_, tr("Menu") );
    mAddMnuItm( mnu, uiStrings::sPolygon(),
		handleToolClick, "polygonselect", 0 );
    mAddMnuItm( mnu, uiStrings::sRectangle(),
		handleToolClick, "rectangleselect", 1 );
    viewtb_->setButtonMenu( polyselectid_, mnu );

    removeselectionid_ = viewtb_->addButton( "trashcan", tr("Remove selection"),
		    mCB(this,uiODMenuMgr,removeSelection), false );

    soloid_ = mAddTB(viewtb_,"solo",tr("Display current element only"),
		     true,soloMode);
}


void uiODMenuMgr::handleViewClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    const int itmid = itm->getID();
    if ( itmid==viewselectid_ )
    {
	sceneMgr().setViewSelectMode( curviewmode_ );
	return;
    }

    BufferString pm( "cube_inl" );
    uiString dir = uiStrings::sInline();
    curviewmode_ = ui3DViewer::Inl;
    switch( itmid )
    {
    case 1: pm = "cube_crl"; dir = uiStrings::sCrossline();
		curviewmode_ = ui3DViewer::Crl; break;
    case 2: pm = "cube_z"; dir = uiStrings::sZ();
		curviewmode_ = ui3DViewer::Z; break;
    case 3: pm = "view_N"; dir = uiStrings::sNorth(false);
		curviewmode_ = ui3DViewer::Y; break;
    case 4: pm = "view_NZ"; dir = mJoinUiStrs(sNorth(false),sZ());
		curviewmode_ = ui3DViewer::YZ; break;
    }

    viewtb_->setIcon( viewselectid_, pm );
    viewtb_->setToolTip( viewselectid_, tr("View %1").arg( dir ));
    sceneMgr().setViewSelectMode( curviewmode_ );
}


void uiODMenuMgr::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    sIsPolySelect = itm->getID()==0;
    selectionMode( 0 );
}


void uiODMenuMgr::selectionMode( CallBacker* cb )
{
    uiVisPartServer* visserv = appl_.applMgr().visServer();
    if ( cb == visserv )
    {
	const bool ison = visserv->isSelectionModeOn();
	viewtb_->turnOn( polyselectid_, ison );
	viewtb_->setSensitive( removeselectionid_, ison );
	sIsPolySelect = visserv->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	const bool ison = viewtb_->isOn( polyselectid_ );
	if ( inviewmode_ && ison )
	    updateViewMode( false );

	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn( !inviewmode_ && ison );
	visserv->setSelectionMode( mode );
    }

    viewtb_->setIcon( polyselectid_, sIsPolySelect ?
			"polygonselect" : "rectangleselect" );
    viewtb_->setToolTip( polyselectid_,
			 sIsPolySelect ? tr("Polygon Selection Mode")
				       : tr("Rectangle Selection Mode") );
}


void uiODMenuMgr::removeSelection( CallBacker* )
{
    uiVisPartServer& visserv = *appl_.applMgr().visServer();
    visserv.removeSelection();
}


void uiODMenuMgr::dispColorBar( CallBacker* cb )
{
    uiVisPartServer& visserv = *appl_.applMgr().visServer();

    mDynamicCastGet(uiAction*,itm,cb)
    if ( itm && itm->getID()==0 )
    {
	visserv.manageSceneColorbar( sceneMgr().askSelectScene() );
	return;
    }

    visserv.displaySceneColorbar( viewtb_->isOn(coltabid_) );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    viewtb_->setToolTip( cameraid_,
			 perspective ? tr("Switch to Orthographic Camera")
				     : tr("Switch to Perspective Camera"));
    viewtb_->setIcon( cameraid_, perspective ? "perspective"
					     : "orthographic" );
}


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODMenuMgr::handleClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return; // Huh?

    const int id = itm->getID();
    switch( id )
    {
    case mManSurveyMnuItm:		applMgr().selectSurvey(0); break;
    case mSessSaveMnuItm:		appl_.saveSession(); break;
    case mSessRestMnuItm:		appl_.restoreSession(); break;
    case mSessAutoMnuItm:		appl_.autoSession(); break;
    case mImpAttrMnuItm:		mDoOp(Imp,Attr,0); break;
    case mImpAttrOthSurvMnuItm:		mDoOp(Imp,Attr,1); break;
    case mImpColTabMnuItm:		mDoOp(Imp,ColTab,0); break;
    case mImpSeisCBVSMnuItm:		mDoOp(Imp,Seis,0); break;
    case mImpSeisSimple3DMnuItm:	mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm:	mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm:	mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm:	mDoOp(Imp,Seis,8); break;
    case mImpSeisCBVSOtherSurvMnuItm:	mDoOp(Imp,Seis,9); break;
    case mExpSeisSimple3DMnuItm:	mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm:	mDoOp(Exp,Seis,6); break;
    case mExpSeisSimplePS3DMnuItm:	mDoOp(Exp,Seis,7); break;
    case mExpSeisSimplePS2DMnuItm:	mDoOp(Exp,Seis,8); break;
    case mExpSeisCubePositionsMnuItm:	mDoOp(Exp,Seis,9); break;
    case mImpHorAsciiMnuItm:		mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm:	mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm:		mDoOp(Imp,Hor,2); break;
    case mImpBulkHorAsciiMnuIm:		mDoOp(Imp,Hor,3); break;
    case mImpBulkHor2DAsciiMnuItm:	mDoOp(Imp,Hor,4); break;
    case mImpHorZMapMnuItm:		mDoOp(Imp,Hor,5); break;
    case mExpHorAscii3DMnuItm:		mDoOp(Exp,Hor,0); break;
    case mExpHorAscii2DMnuItm:		mDoOp(Exp,Hor,1); break;
    case mExpBulkHorAscii3DMnuItm:	mDoOp(Exp,Hor,2); break;
    case mExpBulkHorAscii2DMnuItm:	mDoOp(Exp,Hor,3); break;
    case mExpFltAsciiMnuItm:		mDoOp(Exp,Flt,0); break;
    case mExpBulkFltAsciiMnuItm:	mDoOp(Exp,Flt,1); break;
    case mExpFltSSAsciiMnuItm:		mDoOp(Exp,Fltss,0); break;
    case mExpBulkFltSSAsciiMnuItm:	mDoOp(Exp,Fltss,1); break;
    case mExpFltSetAsciiMnuItm:		mDoOp(Exp,FltSet,0); break;
    case mImpWellAsciiTrackMnuItm:	mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:	mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:	mDoOp(Imp,Wll,2); break;
    case mImpWellSimpleMnuItm:		mDoOp(Imp,Wll,4); break;
    case mImpBulkWellTrackItm:		mDoOp(Imp,Wll,5); break;
    case mImpBulkWellLogsItm:		mDoOp(Imp,Wll,6); break;
    case mImpBulkWellMarkersItm:	mDoOp(Imp,Wll,7); break;
    case mImpBulkWellD2TItm:		mDoOp(Imp,Wll,8); break;
    case mImpBulkDirWellItm:		mDoOp(Imp,Wll,9); break;
    case mImpPickAsciiMnuItm:		mDoOp(Imp,Pick,0); break;
    case mExpPickAsciiMnuItm:		mDoOp(Exp,Pick,0); break;
    case mExpWvltAsciiMnuItm:		mDoOp(Exp,Wvlt,0); break;
    case mImpWvltAsciiMnuItm:		mDoOp(Imp,Wvlt,0); break;
    case mImpFaultMnuItm:		mDoOp(Imp,Flt,0); break;
    case mImpFltSetAsciiMnuItm:		mDoOp(Imp,FltSet,0); break;
    case mImpFaultBulkMnuItm:		mDoOp(Imp,Flt,1); break;
    case mImpFaultSSAscii3DMnuItm:	mDoOp(Imp,Fltss,0); break;
    case mImpFaultSSAscii2DMnuItm:	mDoOp(Imp,Fltss,1); break;
    case mImpFaultSSAscii3DBulkMnuItm:	mDoOp(Imp,Fltss,2); break;
    case mImpFaultSSAscii2DBulkMnuItm:	mDoOp(Imp,Fltss,3); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpPVDSAsciiMnuItm:		mDoOp(Imp,PVDS,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
    case mExpWellACII:			mDoOp(Exp,Wll,0); break;
    case mExpLogLAS:			mDoOp(Exp,Wll,1); break;
    case mImpGeom2DAsciiMnuItm:		mDoOp(Imp,Geom,0); break;
    case mImpGeom2DSEGP1MnuItm:		mDoOp(Imp,Geom,1); break;
    case mExpGeom2DMnuItm:		mDoOp(Exp,Geom,0); break;
    case mExpSurveySetupItm:		applMgr().exportSurveySetup(); break;
    case mManColTabMnuItm:		mDoOp(Man,ColTab,0); break;
    case mManSeis3DMnuItm:		mDoOp(Man,Seis,0); break;
    case mManSeis2DMnuItm:		mDoOp(Man,Seis,1); break;
    case mManSeisPS3DMnuItm:		mDoOp(Man,Seis,2); break;
    case mManSeisPS2DMnuItm:		mDoOp(Man,Seis,3); break;
    case mManHor3DMnuItm:		mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm:		mDoOp(Man,Hor,1); break;
    case mManFaultStickMnuItm:		mDoOp(Man,Flt,1); break;
    case mManFaultMnuItm:		mDoOp(Man,Flt,2); break;
    case mManFaultSetMnuItm:		mDoOp(Man,FltSet,0); break;
    case mManBodyMnuItm:		mDoOp(Man,Body,0); break;
    case mManPropsMnuItm:		mDoOp(Man,Props,0); break;
    case mManWellMnuItm:		mDoOp(Man,Wll,0); break;
    case mManPickMnuItm:		mDoOp(Man,Pick,0); break;
    case mManRanLMnuItm:		mDoOp(Man,RanL,0); break;
    case mManWvltMnuItm:		mDoOp(Man,Wvlt,0); break;
    case mManAttr3DMnuItm:		mDoOp(Man,Attr,0); break;
    case mManAttr2DMnuItm:		mDoOp(Man,Attr,1); break;
    case mManNLAMnuItm:			mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:		mDoOp(Man,Sess,0); break;
    case mManStratMnuItm:		mDoOp(Man,Strat,0); break;
    case mManPDFMnuItm:			mDoOp(Man,PDF,0); break;
    case mManGeomItm:			mDoOp(Man,Geom,0); break;
    case mManCrossPlotItm:		mDoOp(Man,PVDS,0); break;

    case mPreLoadSeisMnuItm:	applMgr().manPreLoad(uiODApplMgr::Seis); break;
    case mPreLoadHorMnuItm:	applMgr().manPreLoad(uiODApplMgr::Hor); break;
    case mRestartMnuItm:	appl_.restart(); break;
    case mExitMnuItm:		appl_.exit(); break;
    case mEditAttrMnuItm:	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm:	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm:	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol(SI().has2D(),false); break;
    case mSeisOut2DMnuItm:	applMgr().createVol(true,false); break;
    case mSeisOut3DMnuItm:	applMgr().createVol(false,false); break;
    case mPSProc2DMnuItm:	applMgr().processPreStack(true); break;
    case mPSProc3DMnuItm:	applMgr().processPreStack(false); break;
    case mVolProc2DMnuItm:	applMgr().createVolProcOutput(true); break;
    case mVolProc3DMnuItm:	applMgr().createVolProcOutput(false); break;
    case mT2DConv2DMnuItm:	applMgr().processTime2Depth(true); break;
    case mT2DConv3DMnuItm:	applMgr().processTime2Depth(false); break;
    case mCreateSurf2DMnuItm:	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm:	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mCreate2DFrom3DMnuItm:	applMgr().create2DGrid(); break;
    case m2DFrom3DMnuItm:	applMgr().create2DFrom3D(); break;
    case m3DFrom2DMnuItm:	applMgr().create3DFrom2D(); break;
    case m3DFrom2DInterPolMnuItm: applMgr().interpol3DFrom2D(); break;
    case mStartBatchJobMnuItm:	applMgr().startBatchJob(); break;
    case mXplotMnuItm:		applMgr().doWellXPlot(); break;
    case mAXplotMnuItm:		applMgr().doAttribXPlot(); break;
    case mOpenXplotMnuItm:	applMgr().openCrossPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mAddTimeDepth2DMnuItm: applMgr().addTimeDepthScene(true); break;
    case mAddTimeDepth3DMnuItm: applMgr().addTimeDepthScene(false); break;
    case mAddHorFlat2DMnuItm:	applMgr().addHorFlatScene(true); break;
    case mAddHorFlat3DMnuItm:	applMgr().addHorFlatScene(false); break;
    case mCascadeMnuItm:	sceneMgr().cascade(); break;
    case mTileAutoMnuItm:	sceneMgr().tile(); break;
    case mTileHorMnuItm:	sceneMgr().tileHorizontal(); break;
    case mTileVerMnuItm:	sceneMgr().tileVertical(); break;
    case mScenePropMnuItm:	sceneMgr().setSceneProperties(); break;
    case mWorkAreaMnuItm:	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm:		applMgr().setZStretch(); break;
    case m2DViewMnuItm:		applMgr().show2DViewer(); break;
    case mBatchProgMnuItm:	applMgr().batchProgs(); break;
    case mPluginsMnuItm:	applMgr().pluginMan(); break;
    case mSetupBatchItm:	applMgr().setupBatchHosts(); break;
    case mGraphicsInfoItm:	uiGLI().createAndShowMessage( true ); break;
    case mHostIDInfoItm:	showHostID(); break;
    case mPosconvMnuItm:	applMgr().posConversion(); break;
    case mCRSPosconvMnuItm:	applMgr().crsPosConversion(); break;
    case mInstMgrMnuItem:	applMgr().startInstMgr(); break;
    case mInstAutoUpdPolMnuItm:	applMgr().setAutoUpdatePol(); break;
    case mCrDevEnvMnuItm:	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm:	showLogFile(); break;
    case mFirewallProcItm:	showFirewallProcDlg(); break;

    case mAddMapSceneMnuItm: {
	sceneMgr().tile();
    const int sceneid = sceneMgr().addScene( true, 0, tr("Map View") );
	ui3DViewer* vwr = sceneMgr().get3DViewer( sceneid );
	if ( vwr ) vwr->setMapView( true );
    } break;
    case mInstConnSettsMnuItm: {
	uiProxyDlg dlg( &appl_ ); dlg.go(); } break;

    case mSettLkNFlMnuItm: {
	uiSettingsDlg dlg( &appl_ );
	dlg.go();
    } break;

    case mDumpDataPacksMnuItm: {
	uiDataPackMonitor::launchFrom( &appl_, 10 );
    } break;
    case mDisplayMemoryMnuItm: {
	IOPar iopar;
	OD::dumpMemInfo( iopar );
	BufferString text;
	iopar.dumpPretty( text );
	uiDialog dlg( &appl_,
	     uiDialog::Setup(tr("Memory Information"),mNoDlgTitle,mNoHelpKey) );
	auto* browser = new uiTextBrowser( &dlg );
	browser->setText( text.buf() );
	dlg.setCancelText( uiString::empty() );
	dlg.go();
    } break;
    case mDisplayWellMgrMnuItm: {
	applMgr().wellServer()->showWellMgrInfo();
    } break;
    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set Personal Settings" );
	dlg.go();
    } break;
    case mSettPython: {
	uiDialog* dlg = uiSettings::getPythonDlg(&appl_,"Set Python Settings");
	dlg->go();
    } break;
    case mSettSurvey: {
	uiSettings dlg( &appl_, "Set Survey Default Settings",
				uiSettings::sKeySurveyDefs() );
	dlg.go();
    } break;

    case mSettShortcutsMnuItm:	applMgr().manageShortcuts(); break;

    case mStereoOffsetMnuItm:	applMgr().setStereoOffset(); break;
    case mStereoOffMnuItm:
    case mStereoRCMnuItm :
    case mStereoQuadMnuItm :
    {
	const int type = id == mStereoRCMnuItm ? 1
					: (id == mStereoQuadMnuItm ? 2 : 0 );
	sceneMgr().setStereoType( type );
	updateStereoMenu();
    } break;

    case mFreeProjects :
    case mCommProjects :
    {
	helpmgr_->handle( id );
    } break;

    default:
    {
    if ( id>=mSceneSelMnuItm && id<=mSceneSelMnuItm +100 )
	{
	    sceneMgr().setActiveScene( id-mSceneSelMnuItm );
	    itm->setChecked( true );
	}

	if ( id >= mViewIconsMnuItm && id < mViewIconsMnuItm+100 )
	{
	    Settings::common().set( sKeyIconSetNm,
				    itm->text().getFullString() + 1 );
	    for ( int idx=0; idx<uiToolBar::toolBars().size(); idx++ )
		uiToolBar::toolBars()[idx]->reloadIcons();
	    Settings::common().write();
	}
	if ( id > mHelpMnu )
	    helpmgr_->handle( id );

    } break;

    }
}


int uiODMenuMgr::ask2D3D( const uiString& txt, int res2d, int res3d,
			  int rescncl )
{
    int res = rescncl;
    if ( !SI().has2D() )
	res = res3d;
    else if ( !SI().has3D() )
	res = res2d;
    else
    {
	const int msg = uiMSG().ask2D3D( txt, true );
	res = msg == -1 ? rescncl : ( msg == 1 ? res2d : res3d );
    }

    return res;
}


void uiODMenuMgr::manHor( CallBacker* )
{
    const int opt =
	ask2D3D( uiStrings::phrManage( tr("2D or 3D Horizons")), 1, 2, 0 );
    if ( opt == 0 ) return;

    mDoOp(Man,Hor,opt);
}


void uiODMenuMgr::manSeis( CallBacker* )
{
    appl_.applMgr().seisServer()->manageSeismics( !SI().has2D() ? 0 : 2 );
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Flt)
mDefManCBFn(Wll)
mDefManCBFn(Pick)
mDefManCBFn(Body)
mDefManCBFn(Props)
mDefManCBFn(Wvlt)
mDefManCBFn(Strat)
mDefManCBFn(PDF)


void uiODMenuMgr::showLogFile()
{
    uiTextFileDlg* dlg = new uiTextFileDlg( &appl_,
				od_ostream::logStream().fileName(), true );
    dlg->setDeleteOnClose( true );
    dlg->show();
}


void uiODMenuMgr::showFirewallProcDlg()
{
    if (__iswin__)
    {
	uiFirewallProcSetter dlg( &appl_, ePDD().getActionType() );
	dlg.go();
    }
}


uiToolBar* uiODMenuMgr::pluginTB()
{
    if ( !plugintb_ )
	plugintb_ = new uiToolBar( &appl_, tr("Third-party Plugins") );

    return plugintb_;
}


uiToolBar* uiODMenuMgr::customTB( const char* nm )
{
    uiToolBar* tb = appl_.findToolBar( nm );
    if ( !tb )
    {
	tb = new uiToolBar( &appl_, toUiString(nm) );
	customtbs_.add( tb );
    }

    return tb;
}


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb_->clear();
    mantb_->clear();
    if ( plugintb_ )
	plugintb_->clear();

    for ( int idx=0; idx<customtbs_.size(); idx++ )
	customtbs_[idx]->clear();

    fillDtectTB( &applMgr() );
    fillManTB();
    uiSettsMgr().updateUserCmdToolBar();
}


void uiODMenuMgr::toggViewMode( CallBacker* cb )
{
    inviewmode_ = !inviewmode_;
    viewtb_->setIcon( actviewid_, inviewmode_ ? "altview" : "altpick" );
    viewtb_->setToolTip( actviewid_,
			 inviewmode_ ? tr("Switch to Interact Mode")
				     : tr("Switch to View Mode") );
    if ( inviewmode_ )
	sceneMgr().viewMode( cb );
    else
	sceneMgr().actMode( cb );

    if ( inviewmode_ )
	applMgr().visServer()->turnSelectionModeOn( false );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    fillImportMenu();
    fillExportMenu();
    fillManMenu();

    fillSceneMenu();
    fillAnalMenu();
    fillProcMenu();

    showtreeitm_->setChecked( true );
    dTectMnuChanged.trigger();
}


void uiODMenuMgr::showHostID()
{
    uiHostIDDlg dlg( &appl_ );
    dlg.go();
}
