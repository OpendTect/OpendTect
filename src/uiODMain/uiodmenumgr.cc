/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "ui3dviewer.h"
#include "uicrdevenv.h"
#include "uifileselector.h"
#include "uifirewallprocsetterdlg.h"
#include "uiglinfo.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodfaulttoolman.h"
#include "uiodhelpmenumgr.h"
#include "uiodhostiddlg.h"
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

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "measuretoolman.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsysmem.h"
#include "odver.h"
#include "settings.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "survinfo.h"
#include "texttranslation.h"
#include "thread.h"
#include "visemobjdisplay.h"

static const char* ascic = "ascii";
static const char* singic = "single";
static const char* multic = "multiple";
static const uiString singstr = uiStrings::sSingle();
static const uiString multstr = uiStrings::sMultiple();
#define mGet2D3D() \
    const bool mUnusedVar have2d = SI().has2D(); \
    const bool mUnusedVar have3d = SI().has3D()
#define mGet2D3DWithOneChoice() \
    mGet2D3D(); \
    bool haveonechoice = !have2d || !have3d; \
    if ( always3d && !have3d ) \
	haveonechoice = false
#define mClickCB mCB(this,uiODMenuMgr,handleClick)


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmnumgr_(0)
    , langmnumgr_(0)
    , measuretoolman_(0)
    , inviewmode_(false)
    , impmnu_(0)
    , expmnu_(0)
    , preloadmnu_(0)
    , manmnu_(0)
    , plugintb_(0)
    , faulttoolman_(0)
{
    impmnus_.setNullAllowed( true ); expmnus_.setNullAllowed( true );

    surveymnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sSurvey()) );
    analmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sAnalysis()) );
    procmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sProcessing()) );
    scenemnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sScenes()) );
    viewmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sView()) );
    utilmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sUtilities()) );
    helpmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sHelp()) );

    dtecttb_ = new uiToolBar( &appl_, tr("OpendTect Tools"), uiToolBar::Top );
    viewtb_ = new uiToolBar( &appl_, tr("Graphical Tools"), uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, uiStrings::phrManage( uiStrings::sData()),
			    uiToolBar::Right );

    uiVisPartServer* visserv = appl_.applMgr().visServer();
    visserv->createToolBars();

    DBM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    DBM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
    visserv->selectionmodeChange.notify(
				mCB(this,uiODMenuMgr,polySelectionModeCB) );
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

    delete helpmnumgr_;
    delete langmnumgr_;
    delete faulttoolman_;
    delete measuretoolman_;
}


uiMenu* uiODMenuMgr::docMnu()
{
    return helpmnumgr_->getDocMenu();
}


uiMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp   ? impmnu_ :
	  (at == uiODApplMgr::Exp   ? expmnu_ :
	  (at == uiODApplMgr::PL    ? preloadmnu_
				    : manmnu_));
}


void uiODMenuMgr::createFaultToolMan()
{
    if ( !faulttoolman_ )
	faulttoolman_ = new uiODFaultToolMan( appl_ );
}


uiMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus_[ot] : expmnus_[ot];
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
{
    viewtb_->turnOn( axisid_, shwaxis );
}

bool uiODMenuMgr::isSoloModeOn() const
{
    return viewtb_->isOn( soloid_ );
}

void uiODMenuMgr::enableMenuBar( bool yn )
{
    appl_.menuBar()->setSensitive( yn );
}

void uiODMenuMgr::enableActButton( bool yn )
{
    if ( yn )
	{ viewtb_->setSensitive( actviewid_, true ); return; }

    if ( !inviewmode_ )
	toggViewMode(0);
    viewtb_->setSensitive( actviewid_, false );
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
    helpmnumgr_ = new uiODHelpMenuMgr( *this );

    fillDtectTB( appman );
    fillVisTB( sceneman );
    fillManTB();
    uiSettsMgr().loadToolBarCmds( appl_ );

    measuretoolman_ = new MeasureToolMan( appl_ );
}


uiMenu* uiODMenuMgr::addSubMenu( uiMenu* parmnu, const uiString& nm,
				 const char* icnm )
{
    uiMenu* mnu = parmnu->addSubMenu( &appl_, nm, icnm );
    if ( parmnu == impmnu_ )
	impmnus_ += mnu;
    else if ( parmnu == expmnu_ )
	expmnus_ += mnu;
    return mnu;
}


uiAction* uiODMenuMgr::addDirectAction( uiMenu* mnu, const uiString& nm,
				   const char* icnm, int id )
{
    uiAction* uiact = new uiAction( nm, mClickCB, icnm );
    mnu->insertAction( uiact, id );
    return uiact;
}


uiAction* uiODMenuMgr::addCheckableAction( uiMenu* mnu, const uiString& nm,
					   const char* icnm, int id )
{
    uiAction* uiact = new uiAction( nm, mClickCB, icnm );
    uiact->setCheckable( true );
    mnu->insertAction( uiact, id );
    return uiact;
}


uiAction* uiODMenuMgr::addAction( uiMenu* mnu, const uiString& nm,
				const char* icnm, int id )
{
    return addDirectAction( mnu, m3Dots(nm), icnm, id );
}


uiAction* uiODMenuMgr::addAction( uiMenu* mnu, const uiString& nm,
				const char* icnm, const CallBack& cb,
				int id )
{
    uiAction* uiact = new uiAction( m3Dots(nm), cb, icnm );
    mnu->insertAction( uiact, id );
    return uiact;
}


uiMenu* uiODMenuMgr::addAsciiSubMenu( uiMenu* mnu, const uiString& nm,
					const char* icnm )
{
    uiMenu* submnu = addSubMenu( mnu, nm, icnm );
    uiMenu* ascmnu = addSubMenu( submnu, uiStrings::sASCII(), ascic );
    return ascmnu;
}


uiMenu* uiODMenuMgr::addAsciiActionSubMenu( uiMenu* mnu, const uiString& nm,
				    const char* icnm, int id,
				    const uiString* altascnm )
{
    uiMenu* ascmnu = addSubMenu( mnu, nm, icnm );
    addAction( ascmnu, altascnm ? *altascnm : uiStrings::sASCII(), ascic, id );
    return ascmnu;
}


uiMenu* uiODMenuMgr::add2D3DAsciiSubMenu( uiMenu* mnu, const uiString& nm,
					const char* icnm, int id2d, int id3d,
					const uiString* altascnm,
					bool always3d )
{
    return addDualAsciiSubMenu( mnu, nm, icnm, id2d, id3d, true,
				altascnm, always3d );
}


uiMenu* uiODMenuMgr::addSingMultAsciiSubMenu( uiMenu* mnu, const uiString& nm,
				    const char* icnm, int idsing, int idmult,
				    const uiString* altascnm )
{
    return addDualAsciiSubMenu( mnu, nm, icnm, idsing, idmult, false,
				altascnm, false );
}


uiMenu* uiODMenuMgr::add2D3DActions( uiMenu* mnu, const uiString& nm,
				     const char* icnm, int id1, int id2,
				     bool always3d )
{
    mGet2D3DWithOneChoice();

    uiMenu* ret = mnu;
    if ( haveonechoice )
	addAction( ret, nm, icnm, have2d ? id1 : id2 );
    else
    {
	ret = addSubMenu( mnu, nm, icnm );
	addAction( ret, uiStrings::s2D(), "2d", id1 );
	addAction( ret, uiStrings::s3D(), "3d", id2 );
    }

    return ret;
}


uiMenu* uiODMenuMgr::add2D3DActions( uiMenu* mnu, const uiString& nm,
				     const char* icnm,
				     const CallBack& cb1, const CallBack& cb2,
				     bool always3d,
				     int itmid2d, int itmid3d )
{
    mGet2D3DWithOneChoice();

    uiMenu* ret = mnu;
    if ( haveonechoice )
	addAction( ret, nm, icnm, have2d ? cb1 : cb2,
				  have2d ? itmid2d : itmid3d );
    else
    {
	ret = addSubMenu( mnu, nm, icnm );
	addAction( ret, uiStrings::s2D(), "2d", cb1, itmid2d );
	addAction( ret, uiStrings::s3D(), "3d", cb2, itmid3d );
    }

    return ret;
}


uiMenu* uiODMenuMgr::addDualAsciiSubMenu( uiMenu* mnu, const uiString& nm,
					const char* icnm, int id1, int id2,
					bool is2d3d, const uiString* altascnm,
					bool always3d )
{
    mGet2D3DWithOneChoice();
    if ( !is2d3d )
	haveonechoice = false;

    uiMenu* ascmnu;
    if ( haveonechoice )
	ascmnu = addAsciiActionSubMenu( mnu, nm, icnm, have2d ? id1 : id2,
					altascnm );
    else
    {
	const uiString ascnm = altascnm ? *altascnm : uiStrings::sASCII();
	uiMenu* submnu = addSubMenu( mnu, nm, icnm );
	ascmnu = addSubMenu( submnu, ascnm, ascic );
	if ( is2d3d )
	{
	    addAction( ascmnu, uiStrings::s2D(), "2d", id1 );
	    addAction( ascmnu, uiStrings::s3D(), "3d", id2 );
	}
	else
	{
	    addAction( ascmnu, uiStrings::sSingle(), singic, id1 );
	    addAction( ascmnu, uiStrings::sMultiple(), multic, id2 );
	}
    }
    return ascmnu;
}


uiMenu* uiODMenuMgr::add2D3DSingMultAsciiSubMenu( uiMenu* mnu,
	    const uiString& nm, const char* icnm,
	    int id2dsing, int id3dsing, int id2dmult, int id3dmult,
	    bool always3d )
{
    mGet2D3DWithOneChoice();

    uiMenu* ascmnu;
    if ( haveonechoice )
	ascmnu = addSingMultAsciiSubMenu( mnu, nm, icnm,
		have2d ? id2dsing : id3dsing, have2d ? id2dmult : id3dmult );
    else
    {
	ascmnu = addAsciiSubMenu( mnu, nm, icnm );
	uiMenu* mnu2d = addSubMenu( ascmnu, uiStrings::s2D(), "2d" );
	addAction( mnu2d, singstr, singic, id2dsing );
	addAction( mnu2d, multstr, multic, id2dmult );
	uiMenu* mnu3d = addSubMenu( ascmnu, uiStrings::s3D(), "3d" );
	addAction( mnu3d, singstr, singic, id3dsing );
	addAction( mnu3d, multstr, multic, id3dmult );
    }
    return ascmnu;
}


void uiODMenuMgr::fillSurveyMenu()
{
    addAction( surveymnu_, tr("Select/Setup"), "survey", mManSurveyMnuItm );

    uiMenu* sessionmnu = addSubMenu( surveymnu_, uiStrings::sSession(),
				    "session" ) ;
    addAction( sessionmnu, uiStrings::sSave(), "save", mSessSaveMnuItm );
    addAction( sessionmnu, tr("Restore"), "spiral_in", mSessRestMnuItm );
    addAction( sessionmnu, tr("Auto-Load"), "autoload", mSessAutoMnuItm );

    surveymnu_->insertSeparator();

    impmnu_ = addSubMenu( surveymnu_, uiStrings::sImport(), "import" );
    expmnu_ = addSubMenu( surveymnu_, uiStrings::sExport(), "export" );
    manmnu_ = addSubMenu( surveymnu_, uiStrings::sManage(), "manage" );
    preloadmnu_ = addSubMenu( surveymnu_, tr("Pre-load"), "preloaded" );

    setSurveySubMenus();

    surveymnu_->insertSeparator();

    addDirectAction( surveymnu_, tr("Restart"), "restart", mRestartMnuItm );
    addDirectAction( surveymnu_, uiStrings::sExit(), "exit", mExitMnuItm );
}


static uiString getPointSetsPolygonsStr()
{
    return toUiString("%1/%2").arg( uiStrings::sPointSet(mPlural) )
			      .arg( uiStrings::sPolygon(mPlural) );
}


void uiODMenuMgr::setSurveySubMenus()
{
    impmnu_->clear(); expmnu_->clear(); manmnu_->clear(); preloadmnu_->clear();
    impmnus_.setEmpty(); expmnus_.setEmpty();
    mGet2D3D();

    uiMenu* ascmnu;
    uiString mnunm; const char* iconnm;

    mnunm = uiStrings::sAttributeSet(mPlural); iconnm = "attributes";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpAttrAsciiMnuItm );
    expmnus_ += 0;
    add2D3DActions( manmnu_, mnunm, iconnm, mManAttr2DMnuItm, mManAttr3DMnuItm);

    mnunm = uiStrings::sBody(mPlural); iconnm = "tree-body";
    impmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManBodyMnuItm );
    expmnus_ += 0;

    mnunm = uiStrings::sColorTable(mPlural); iconnm = "colorbar";
    impmnus_ += 0;
    addAction( impmnu_, mnunm, iconnm, mImpColTabMnuItm );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManColTabMnuItm );

    mnunm = tr("Cross-Plot Data"); iconnm = "xplot";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpXPlotAsciiMnuItm );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManXPlotMnuItm );

    mnunm = uiStrings::sFault(mPlural); iconnm = "tree-flt";
    addSingMultAsciiSubMenu( impmnu_, mnunm, iconnm,
			     mImpFltAsciiMnuItm, mImpBulkFltAsciiMnuItm );
    addSingMultAsciiSubMenu( expmnu_, mnunm, iconnm,
			     mExpFltAsciiMnuItm, mExpBulkFltAsciiMnuItm );

    mnunm = uiStrings::sFaultStickSet(mPlural); iconnm = "tree-fltss";
    add2D3DSingMultAsciiSubMenu( impmnu_, mnunm, iconnm,
	    mImpFltSSAscii2DMnuItm, mImpFltSSAscii3DMnuItm,
	    mImpFltSSAscii2DBulkMnuItm, mImpFltSSAscii3DBulkMnuItm );
    addSingMultAsciiSubMenu( expmnu_, mnunm, iconnm,
			     mExpFltSSAsciiMnuItm, mExpBulkFltSSAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManFltSSMnuItm );

    mnunm = uiStrings::sFaultSet(mPlural); iconnm = "faultplanes";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( impmnu_, mnunm, iconnm, mImpFltSetAsciiMnuItm );
    addAction( expmnu_, mnunm, iconnm, mExpFltSetAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManFaultSetMnuItm );

    mnunm = tr("Navigation Data / 2D Geometry"); iconnm = "tree-geom2d";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpGeom2DAsciiMnuItm );
    if ( !have2d )
	expmnus_ += 0;
    else
    {
	mnunm = tr("2D Geometries"); iconnm = "tree-geom2d";
	addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpGeom2DAsciiMnuItm );
	addAction( manmnu_, mnunm, iconnm, mManGeom2DMnuItm );
    }

    mnunm = uiStrings::sHorizon(mPlural);
    iconnm = have2d ? "horizons" : "tree-horizon3d";
    ascmnu = add2D3DSingMultAsciiSubMenu( impmnu_, mnunm, iconnm,
	    mImpHor2DAsciiMnuItm, mImpHor3DAsciiMnuItm,
	    mImpBulkHor2DAsciiMnuItm, mImpBulkHor3DAsciiMnuItm, true );
    ascmnu = add2D3DSingMultAsciiSubMenu( expmnu_, mnunm, iconnm,
	    mExpHor2DAsciiMnuItm, mExpHor3DAsciiMnuItm,
	    mExpBulkHor2DAsciiMnuItm, mExpBulkHor3DAsciiMnuItm, true );
    addAction( ascmnu, tr("Horizon Attribute"), "horattrib",
			mImpHor3DAsciiAttribMnuItm );
    add2D3DActions( manmnu_, mnunm, iconnm,
		     mManHor2DMnuItm, mManHor3DMnuItm, true );
    addAction( preloadmnu_, mnunm, iconnm, mPreLoadHorMnuItm );

    mnunm = tr("Layer Properties"); iconnm = "defprops";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManPropsMnuItm );

    mnunm = tr("Mute Functions"); iconnm = "mute";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpMuteDefAsciiMnuItm );
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpMuteDefAsciiMnuItm );

    // Nothing for NLA
    impmnus_ += 0;
    expmnus_ += 0;

    mnunm = getPointSetsPolygonsStr();
    iconnm = "pointspolygons";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpPickAsciiMnuItm );
    impmnus_ += impmnus_.last(); // because both Pick and Poly in enum
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpPickAsciiMnuItm );
    expmnus_ += expmnus_.last();
    addAction( manmnu_, mnunm, iconnm, mManPickMnuItm );

    mnunm = uiStrings::sProbDensFunc(false,mPlural); iconnm = "prdfs";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpPDFAsciiMnuItm );
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpPDFAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManPDFMnuItm );

    mnunm = uiStrings::sRandomLine(mPlural); iconnm = "tree-randomline";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManRanLMnuItm );

    createSeisSubMenus();

    mnunm = uiStrings::sSession(mPlural); iconnm = "session";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManSessMnuItm );

    mnunm = uiStrings::sStratigraphy(); iconnm = "strat_tree";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManStratMnuItm );

    mnunm = uiStrings::sVelocity(mPlural); iconnm = "velocity_cube";
    impmnus_ += 0;
    expmnus_ += 0;
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpVelocityAsciiMnuItm );

    mnunm = uiStrings::sWavelet(mPlural); iconnm = "wavelet";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpWvltAsciiMnuItm );
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpWvltAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManWvltMnuItm );

    mnunm = uiStrings::sWell(mPlural); iconnm = "well";
    fillWellImpSubMenu( addSubMenu(impmnu_,mnunm,iconnm) );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManWellMnuItm );

    impmnu_->insertSeparator(); expmnu_->insertSeparator();
    manmnu_->insertSeparator(); preloadmnu_->insertSeparator();
}


void uiODMenuMgr::fillWellImpSubMenu( uiMenu* mnu )
{
    imptrackmnu_ = addSubMenu( mnu, tr("Track/Time-Depth"), "welltrack" );
    implogsmnu_ = addSubMenu( mnu, uiStrings::sLogs(), "welllog" );
    impmarkersmnu_ = addSubMenu( mnu, uiStrings::sMarker(mPlural),
				 "wellmarkers" );
    addAction( mnu, tr("Simple Multi-Well"), "multiwell",
		mImpWellSimpleMultiMnuItm );

    uiMenu* ascmnu = addSubMenu( imptrackmnu_, uiStrings::sASCII(), ascic );
    addAction( ascmnu, singstr, singic, mImpWellAsciiTrackMnuItm );
    addAction( ascmnu, tr("Multiple Tracks"), multic, mImpBulkWellTrackItm );
    addAction( ascmnu, tr("Multiple Depth-Time Models"), multic,
					mImpBulkWellD2TItm );

    ascmnu = addSubMenu( implogsmnu_, toUiString("%1 (LAS)")
			    .arg(uiStrings::sASCII()), ascic );
    addAction( ascmnu, singstr, singic, mImpWellAsciiLogsMnuItm );
    addAction( ascmnu, multstr, multic, mImpBulkWellLogsItm );

    ascmnu = addSubMenu( impmarkersmnu_, uiStrings::sASCII(), ascic );
    addAction( ascmnu, tr("Single-Well"), singic, mImpWellAsciiMarkersMnuItm );
    addAction( ascmnu, tr("Multi-Well"), multic, mImpBulkWellMarkersItm );
}


uiMenu* uiODMenuMgr::addFullSeisSubMenu( uiMenu* parmnu, const uiString& mnunm,
			const char* icnm, const CallBack& cb, int menustartid )
{
    uiMenu* mnu = addSubMenu( parmnu, mnunm, icnm );
    fillFullSeisSubMenu( mnu, cb, menustartid );
    return mnu;
}


void uiODMenuMgr::fillFullSeisSubMenu( uiMenu* mnu, const CallBack& cb,
					int menustartid )
{
    mGet2D3D();
    if ( have2d )
    {
	uiString linestr = have3d ? uiStrings::s2D() : tr("Line(s)");
	uiString linepsstr = have3d ? tr("Prestack 2D") : tr("Prestack Data");
	addAction( mnu, linestr, "seismicline2d", cb, menustartid );
	addAction( mnu, linepsstr, "prestackdataset2d", cb, menustartid+1 );
    }
    if ( have3d )
    {
	uiString volstr = have2d ? uiStrings::s3D() : uiStrings::sVolume();
	uiString volpsstr = have2d ? tr("Prestack 3D") : tr("Prestack Volume");
	addAction( mnu, volstr, "seismiccube", cb, menustartid+2 );
	addAction( mnu, volpsstr, "prestackdataset", cb, menustartid+3 );
    }
}


void uiODMenuMgr::createSeisSubMenus()
{
    uiString mnunm = uiStrings::sSeismicData();
    const char* iconnm = "seis";
    addAction( preloadmnu_, mnunm, iconnm, mPreLoadSeisMnuItm );
    addFullSeisSubMenu( manmnu_, mnunm, iconnm, mClickCB, mManSeisMnu );

    uiMenu* impseis = addSubMenu( impmnu_, mnunm, iconnm );
    uiMenu* expseis = addSubMenu( expmnu_, mnunm, iconnm );
    mnunm = tr("Simple File");
    addFullSeisSubMenu( impseis, mnunm, ascic, mClickCB, mImpSeisSimpleMnu );
    addFullSeisSubMenu( expseis, mnunm, ascic, mClickCB, mExpSeisSimpleMnu );

    mGet2D3D();
    if ( have3d )
    {
	mnunm = uiStrings::sOpendTect(); iconnm = "od";
	uiMenu* odimpmnu = addSubMenu( impseis, mnunm, iconnm );
	addAction( odimpmnu, tr("From File"), "singlefile",
		   mImpSeisODCubeMnuItm );
	addAction( odimpmnu, tr("From Other Survey"), "impfromothsurv",
		   mImpSeisODCubeOtherSurvMnuItm );
	addAction( expseis, tr("Cube Positions"), iconnm,
		   mExpSeisCubePositionsMnuItm );
    }
}


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

// SEISMIC OUTPUT

    csomnu_ = addSubMenu( procmnu_, tr("Create Seismic Output"), "out_seis" );

    uiMenu* attrmnu = addSubMenu( csomnu_, uiStrings::sAttribute(mPlural),
				  "attributes" );
    add2D3DActions( attrmnu, tr("Single Attribute"), "single",
		    mSeisOut2DMnuItm, mSeisOut3DMnuItm);

    mGet2D3D();
    if ( have3d )
    {
	addAction( attrmnu, tr("Multi Attribute"), "multiple",
		    mCB(&applMgr(),uiODApplMgr,createMultiAttribVol) );
    }

    add2D3DActions( attrmnu, tr("Along Horizon"), "alonghor",
		    mCompAlongHor2DMnuItm, mCompAlongHor3DMnuItm );
    add2D3DActions( attrmnu, tr("Between Horizons"), "betweenhors",
		    mCompBetweenHor2DMnuItm, mCompBetweenHor3DMnuItm );


    if ( have2d && have3d )
    {
	uiMenu* mnu2d3d = addSubMenu( csomnu_, tr("2D <=> 3D"), "2d3d" );
	addAction( mnu2d3d, tr("Create 2D Line Grid from 3D Cube"),
			"cr2dfrom3d", mCreate2DFrom3DMnuItm );
	addAction( mnu2d3d, tr("Extract 3D Cube to Line Data"),
			"extr3dinto2d", mExtract2DFrom3DMnuItm );
	addAction( mnu2d3d, tr("Create 3D Cube from 2D Lines"),
			"cr3dfrom2d", mCreate3DFrom2DMnuItm );
    }

    if ( have3d )
    {
	addAction( csomnu_, tr("Angle Mute Function"), "anglemute",
		mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) );
	addAction( csomnu_, tr("Bayesian Classification"), "bayes",
		mCB(&applMgr(),uiODApplMgr,bayesClass3D) );
	addAction( csomnu_, tr("From Well Logs"), "log2cube",
		mCB(&applMgr(),uiODApplMgr,createCubeFromWells) );
    }

    uiMenu* psmnu = add2D3DActions( csomnu_, tr("Prestack Processing"),
			"prestackdataset", mPSProc2DMnuItm, mPSProc3DMnuItm );
    if ( have3d )
	addAction( psmnu, tr("Create MultiCube DataStore"), "multicubeps",
			    mCB(&applMgr(),uiODApplMgr,createMultiCubeDS) );

    uiMenu* velmnu = addSubMenu( csomnu_, uiStrings::sVelocity(),
				 "velocity_cube" );
    add2D3DActions( velmnu, tr("Time - Depth Conversion"), "time2depth",
		    mT2DConv2DMnuItm, mT2DConv3DMnuItm );
    if ( have3d )
	addAction( velmnu, tr("Velocity Conversion"), "velconv",
			 mCB(&applMgr(),uiODApplMgr,processVelConv) );

    add2D3DActions( csomnu_, tr("Volume Builder"), "volproc",
		     mVolProc2DMnuItm, mVolProc3DMnuItm );

// OTHER

    chomnu_ = addSubMenu( procmnu_, tr("Create Horizon Output"), "out_hor" );
    add2D3DActions( chomnu_, uiStrings::sAttribute(mPlural), "attributes",
		    mCreateSurf2DMnuItm, mCreateSurf3DMnuItm );

    procwellmnu_ = addSubMenu( procmnu_, uiStrings::sWells(), "well" );
    addAction( procwellmnu_, uiStrings::sRockPhy(), "rockphys",
		     mCB(&applMgr(),uiODApplMgr,launchRockPhysics) );

    addAction( procmnu_, tr("(Re-)Start Batch Job"), "work",
	       mStartBatchJobMnuItm );
}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();

    add2D3DActions( analmnu_, uiStrings::sAttribute(mPlural), "attributes",
		    mEdit2DAttrMnuItm, mEdit3DAttrMnuItm );
    add2D3DActions( analmnu_, tr("Volume Builder"),
		    VolProc::uiChain::pixmapFileName(),
		    mVolProc2DMnuItm, mVolProc3DMnuItm );

    uiMenu* xplotmnu = addSubMenu( analmnu_, tr("Cross-plot Data"), "xplot" );
    addAction( xplotmnu, tr("Well logs vs Attributes"), "xplot_wells",
			mXPlotMnuItm );
    addAction( xplotmnu, tr("Attributes vs Attributes"), "xplot_attribs",
			mAttrXPlotMnuItm );
    addAction( xplotmnu, tr("Open Cross-plot from File"), "singlefile",
			mOpenXPlotMnuItm );

    analwellmnu_ = addSubMenu( analmnu_, uiStrings::sWells(), "well" );
    addAction( analwellmnu_, tr("Edit Logs"), "well_props",
		mCB(&applMgr(),uiODApplMgr,doWellLogTools) );
    if ( SI().zIsTime() )
	addAction( analwellmnu_, tr("Tie Well to Seismic"), "well_tie",
		    mCB(&applMgr(),uiODApplMgr,tieWellToSeismic) );

    layermodelmnu_ = addSubMenu( analmnu_, tr("Layer Modeling"),
				 "stratlayermodeling" );
    addAction( layermodelmnu_, uiStrings::sBasic(), "basiclayermodeling",
		mCB(&applMgr(),uiODApplMgr,doLayerModeling) );

    analmnu_->insertSeparator();
}


void uiODMenuMgr::fillSceneMenu()
{
    scenemnu_->clear();
    addAction( scenemnu_, uiStrings::sNew(), "addnew", mAddSceneMnuItm );
    addAction( scenemnu_, tr("New Map View"), "survey", mAddMapSceneMnuItm );

    uiString tdmnutxt = toUiString( "%1 [%2]" ).arg( uiStrings::sNew() )
	 .arg( SI().zIsTime() ? uiStrings::sDepth() : uiStrings::sTime() );
    const char* iconnm = SI().zIsTime() ? "depth" : "time";
    add2D3DActions( scenemnu_, tdmnutxt, iconnm, mAddTimeDepth2DMnuItm,
						 mAddTimeDepth3DMnuItm );

    tdmnutxt = toUiString( "%1 [%2]" ).arg( uiStrings::sNew() )
			.arg( tr("Horizon Flattened") );
    add2D3DActions( scenemnu_, tdmnutxt, "horizons",
		    mAddHorFlat2DMnuItm, mAddHorFlat3DMnuItm );
    lastsceneitm_ = scenemnu_->insertSeparator();

    addAction( scenemnu_, tr("Cascade"), "cascade", mCascadeMnuItm );

    uiMenu* tilemnu = addSubMenu( scenemnu_, uiStrings::sTile(), "tile" );
    addAction( tilemnu, uiStrings::sAuto(), "auto", mTileAutoMnuItm );
    addAction( tilemnu, uiStrings::sHorizontal(), "hortile", mTileHorMnuItm );
    addAction( tilemnu, uiStrings::sVertical(), "vertile", mTileVerMnuItm );

    addAction( scenemnu_, uiStrings::sProperties(), "settings",
		    mScenePropMnuItm );

    updateSceneMenu();
}


void uiODMenuMgr::insertNewSceneItem( uiAction* action, int id )
{
    scenemnu_->insertAction( action, id, lastsceneitm_ );
    lastsceneitm_ = action;
}


void uiODMenuMgr::updateSceneMenu()
{
    uiStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int idx=0; idx<=scenenms.size(); idx++ )
    {
	const int id = mSceneSelMnuItm + idx;
	uiAction* itm = scenemnu_->findAction( id );

	if ( idx >= scenenms.size() )
	{
	    if ( itm )
		scenemnu_->removeAction( id );
	    continue;
	}

	if ( !itm )
	{
	    itm = new uiAction( uiString::empty(),
				mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertAction( itm, id );
	    itm->setCheckable( true );
	}

	itm->setText( scenenms[idx] );
	itm->setChecked( idx==activescene );
    }
}


void uiODMenuMgr::fillViewMenu()
{
    viewmnu_->clear();
    addAction( viewmnu_, tr("Work Area"), "workarea", mWorkAreaMnuItm );
    addAction( viewmnu_, tr("Z-Scale"), "zscale", mZScaleMnuItm );
    addAction( viewmnu_, tr("Seismics [2D Viewer]"), "2dlaunch", m2DViewMnuItm);

#define mAddStereoAction(itm,txt,icnm,id,docheck) \
    itm = addCheckableAction( stereomnu, txt, icnm, id ); \
    itm->setChecked( docheck )
    uiMenu* stereomnu = addSubMenu( viewmnu_, tr("Stereo Viewing"), "stereo" );
    mAddStereoAction( stereooffitm_, uiStrings::sOff(), "off",
			mStereoOffMnuItm, true );
    mAddStereoAction( stereoredcyanitm_, tr("Red/Cyan"), "redcyan",
			mStereoRCMnuItm, false );
    mAddStereoAction( stereoquadbufitm_, tr("Quad Buffered"), "quadbuff",
			mStereoQuadMnuItm, false );

    stereooffsetitm_ = addAction( stereomnu, tr("Stereo Offset"),
				  "stereooffset", mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    viewmnu_->insertSeparator();

    uiMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.setName( "Toolbars" );
    toolbarsmnu.setIcon( "toolbar" );
    viewmnu_->addMenu( &toolbarsmnu );
}


void uiODMenuMgr::fillUtilMenu()
{
#ifdef __mac__
    // Qt disables the 'Settings' menu on Mac, hence a different text
    settmnu_ = addSubMenu( utilmnu_, tr("User Settings"), "settings" );
#else
    settmnu_ = addSubMenu( utilmnu_, uiStrings::sSettings(), "settings" );
#endif

    langmnumgr_ = new uiODLangMenuMgr( *this );
    addAction( settmnu_, uiStrings::sUserSettings(), "settings",
				mSettingsMnuItm );

#ifdef __debug__
    const bool defenabadvsettings = true;
#else
    const bool defenabadvsettings = false;
#endif
    const bool enabadvsettings = GetEnvVarYN( "OD_ENABLE_ADVANCED_SETTINGS",
		    defenabadvsettings );
    if ( enabadvsettings )
    {
	uiMenu* advmnu = addSubMenu( settmnu_, uiStrings::sAdvanced(),
				     "advanced" );
	addAction( advmnu, tr("Personal Settings"), "unknownperson",
						mSettAdvPersonal );
	addAction( advmnu, tr("Survey Defaults"), "survey", mSettAdvSurvey );
    }

    toolsmnu_ = addSubMenu( utilmnu_, uiStrings::sTools(), "tools" );
    addAction( toolsmnu_, tr("Batch Programs"), "batchprogs", mBatchProgMnuItm);
    addAction( toolsmnu_, tr("Position Conversion"), "xy2ic", mPosconvMnuItm );
    addAction( toolsmnu_, tr("CRS Position Conversion"), "crs",
	       mCRSPosconvMnuItm );

    BufferString develverstr;
    GetSpecificODVersion( "devel", develverstr );
    if ( !develverstr.isEmpty() )
	addAction( toolsmnu_, tr("Create Plugin Devel. Env."), "plugin",
				mCrDevEnvMnuItm );

    installmnu_ = addSubMenu( utilmnu_, tr("Installation"), "od" );
    File::Path installerdir( ODInst::GetInstallerDir() );
    const bool hasinstaller = File::isDirectory( installerdir.fullPath() );
    if ( hasinstaller && !__ismac__ )
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	const bool aitfixed = ODInst::autoInstTypeIsFixed();
	if ( !aitfixed || ait == ODInst::UseManager || ait == ODInst::FullAuto )
	    addAction( installmnu_, uiStrings::sUpdate(), "update",
				    mInstMgrMnuItem );
	if ( !aitfixed )
	    addAction( installmnu_, tr("Auto-update Policy"), "auto",
				    mInstAutoUpdPolMnuItm );
	installmnu_->insertSeparator();
    }

    addAction( installmnu_, tr("Python Settings"), "python", mSettAdvPython );
    addAction( installmnu_, tr("Internet Connection Settings"),
			    "internet_connection", mInstConnSettsMnuItm );
    addAction( installmnu_, uiStrings::sPlugin(mPlural), "plugin",
			    mPluginsMnuItm );
    addAction( installmnu_, tr("Graphics Information"), "info",
			    mGraphicsInfoItm );
    addAction( installmnu_, tr("Show HostID"), "hostid",
			    mHostIDInfoItm );
    if ( __iswin__ )
	addAction( installmnu_, tr("Firewall Add/Remove Process"), "",
							    mFirewallProcItm );

    mmmnu_ = addSubMenu( utilmnu_, tr("Distributed Processing"), "mmproc" );
    addAction( mmmnu_, uiStrings::sSetup(), "settings", mSetupBatchItm );

    const char* lmfnm = od_ostream::logStream().fileName();
    if ( lmfnm && *lmfnm )
	addAction( toolsmnu_, tr("Show Log File"), "logfile",
			      mShwLogFileMnuItm );

#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	addAction( toolsmnu_, tr("DataPack Dump"), "dump",
			      mDumpDataPacksMnuItm );
	addAction( toolsmnu_, tr("Memory Info"), "memory",
			      mDisplayMemoryMnuItm );
    }
}


int uiODMenuMgr::add2D3DToolButton( uiToolBar& tb, const char* iconnm,
				    const uiString& tt,
				    const CallBack& cb2d, const CallBack& cb3d,
				    int itmid2d, int itmid3d )
{
    mGet2D3D();

    if ( !have2d )
	return tb.addButton( iconnm, tt, cb3d, false, itmid3d );
    if ( !have3d )
	return tb.addButton( iconnm, tt, cb2d, false, itmid2d );

    const int butid = tb.addButton( iconnm, tt );
    uiMenu* popmnu = tb.addButtonMenu( butid, uiToolButton::InstantPopup );
    popmnu->insertAction( new uiAction(m3Dots(uiStrings::s2D()),cb2d,"2d"),
			itmid2d );
    popmnu->insertAction( new uiAction(m3Dots(uiStrings::s3D()),cb3d,"3d"),
			itmid3d );
    return butid;
}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(appman,uiODApplMgr,fn), togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey",tr("Survey Setup"),false,manSurvCB);
    add2D3DToolButton( *dtecttb_, "attributes", tr("Edit Attributes"),
		       mCB(appman,uiODApplMgr,editAttr2DCB),
		       mCB(appman,uiODApplMgr,editAttr3DCB) );
    add2D3DToolButton( *dtecttb_, "out_seis", tr("Create Seismic Output"),
		       mCB(appman,uiODApplMgr,seisOut2DCB),
		       mCB(appman,uiODApplMgr,seisOut3DCB) );

    add2D3DToolButton( *dtecttb_,VolProc::uiChain::pixmapFileName(),
		tr("Volume Builder"),
		mCB(appman,uiODApplMgr,doVolProc2DCB),
		mCB(appman,uiODApplMgr,doVolProc3DCB) );

    const int xplotid = dtecttb_->addButton( "xplot", tr("Cross-plot") );
    uiMenu* mnu = dtecttb_->addButtonMenu( xplotid, uiToolButton::InstantPopup);
    mnu->insertAction(
	new uiAction(m3Dots(tr("Cross-plot Attribute vs Attribute Data")),
		     mCB(appman,uiODApplMgr,doAttribXPlot),"xplot_attribs") );
    mnu->insertAction(
	new uiAction(m3Dots(tr("Cross-plot Attribute vs Well Data")),
		     mCB(appman,uiODApplMgr,doWellXPlot),"xplot_wells") );

    mAddTB(dtecttb_,"rockphys",tr("Create New Well Logs Using Rock Physics"),
			false,launchRockPhysics);
    mAddTB(dtecttb_,"2dlaunch",tr("Launch 2D Viewer"),
			false,launch2DViewer);
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(this,uiODMenuMgr,fn), togg )

#define mAddPopUp( txt1, txt2, itm1, itm2, mnuid ) \
{ \
    uiMenu* popmnu = mantb_ ->addButtonMenu( mnuid, \
					uiToolButton::InstantPopup ); \
    popmnu->insertAction( new uiAction(txt1, \
		       mCB(this,uiODMenuMgr,handleClick)), itm1 ); \
    popmnu->insertAction( new uiAction(txt2, \
		       mCB(this,uiODMenuMgr,handleClick)), itm2 ); \
}

#define mAddPopupMnu( mnu, txt, itm ) \
    mnu->insertAction(new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)),itm);

void uiODMenuMgr::fillManTB()
{
    const int seisid =
	mAddTB(mantb_,"man_seis",
	       uiStrings::phrManage(uiStrings::sSeisObjName(true, true, false)),
				    false,manSeis);
    const int horid = mAddTB(mantb_,"man_hor",
	      uiStrings::phrManage( uiStrings::sHorizon(mPlural)),false,manHor);
    const int fltid = mAddTB(mantb_,"man_flt",
	      uiStrings::phrManage( uiStrings::sFault(mPlural)),false,manFlt);
    mAddTB(mantb_,"man_wll",
	   uiStrings::phrManage( uiStrings::sWells()),false,manWll);
    mAddTB(mantb_,"man_picks", uiStrings::phrManage(getPointSetsPolygonsStr()),
			    false, manPick );
    mAddTB(mantb_,"man_body", uiStrings::phrManage(uiStrings::sBody(mPlural)),
			    false, manBody );
    mAddTB(mantb_,"man_wvlt",
	   uiStrings::phrManage(uiStrings::sWavelet(mPlural)),false,manWvlt);
    mAddTB(mantb_,"man_strat",uiStrings::phrManage( uiStrings::sStratigraphy()),
	   false,manStrat);

    mGet2D3D();

    uiMenu* seispopmnu = mantb_->addButtonMenu( seisid,
						uiToolButton::InstantPopup );
    fillFullSeisSubMenu( seispopmnu, mClickCB, mManSeisMnu );

    if ( have2d )
	mAddPopUp( tr("2D Horizons"), tr("3D Horizons"),
		   mManHor2DMnuItm, mManHor3DMnuItm, horid );

    uiMenu* fltpopmnu = mantb_->addButtonMenu( fltid,
					       uiToolButton::InstantPopup );
    mAddPopupMnu( fltpopmnu, uiStrings::sFault(mPlural), mManFltMnuItm );
    mAddPopupMnu( fltpopmnu, uiStrings::sFaultStickSet(mPlural),
		  mManFltSSMnuItm );
    if ( have3d )
	mAddPopupMnu( fltpopmnu, uiStrings::sFaultSet(mPlural),
		      mManFaultSetMnuItm );
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiODMenuMgr,fn), fnm ); \
    mnu->insertAction( itm, idx ); }

#define mAddSMMnuItm(mnu,txt,fn,fnm,idx) { \
    uiAction* itm = new uiAction( txt, mCB(scenemgr,uiODSceneMgr,fn), fnm ); \
    mnu->insertAction( itm, idx ); }

void uiODMenuMgr::fillVisTB( uiODSceneMgr* scenemgr )
{
    actviewid_ = viewtb_->addButton( "altpick", tr("Switch to View Mode"),
			mCB(this,uiODMenuMgr,toggViewMode), false );

    int homeid = mAddTB(viewtb_,"home",tr("To home position"),false,toHomePos);
    uiMenu* homemnu = viewtb_->addButtonMenu( homeid );
    mAddSMMnuItm( homemnu, tr("To home position"), toHomePos, "home", 0 );
    mAddSMMnuItm( homemnu, tr("Save position as Home position"),
		  saveHomePos, "set_home", 1 );

    mAddTB( viewtb_, "view_all", tr("View All"), false, viewAll );
    cameraid_ = mAddTB( viewtb_, "perspective",
			tr("Switch to Orthographic Camera"),
			false, switchCameraType );

    curviewmode_ = ui3DViewer::Inl;
    bool separateviewbuttons = false;
    Settings::common().getYN( "dTect.SeparateViewButtons", separateviewbuttons);
    if ( !separateviewbuttons )
    {
	viewselectid_ = viewtb_->addButton( "cube_inl",tr("View In-line"),
				mCB(this,uiODMenuMgr,handleViewClick), false );
	uiMenu* vwmnu = viewtb_->addButtonMenu( viewselectid_ );
	mAddMnuItm( vwmnu, tr("View In-line"),
		    handleViewClick, "cube_inl", 0 );
	mAddMnuItm( vwmnu, tr("View Cross-line"), handleViewClick,
		    "cube_crl", 1 );
	mAddMnuItm( vwmnu, tr("View Z"), handleViewClick, "cube_z", 2 );
	mAddMnuItm( vwmnu, tr("View North"), handleViewClick, "view_N", 3 );
	mAddMnuItm( vwmnu, tr("View North Z"), handleViewClick, "view_NZ", 4);
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
    uiMenu* colbarmnu = viewtb_->addButtonMenu( coltabid_ );
    mAddMnuItm( colbarmnu, m3Dots(uiStrings::sSettings()), dispColorBar,
		"disppars", 0 );

    mAddTB(viewtb_,"snapshot",uiStrings::sTakeSnapshot(),false,mkSnapshot);
    polyselectid_ = viewtb_->addButton( "polygonselect",
		tr("Polygon Selection mode"),
		mCB(this,uiODMenuMgr,polySelectionModeCB), true );
    uiMenu* mnu = viewtb_->addButtonMenu( polyselectid_ );
    mAddMnuItm( mnu, uiStrings::sPolygon(),
		handleToolClick, "polygonselect", 0 );
    mAddMnuItm( mnu, uiStrings::sRectangle(),
		handleToolClick, "rectangleselect", 1 );

    removeselectionid_ = viewtb_->addButton( "remove", tr("Remove selection"),
		    mCB(this,uiODMenuMgr,removeSelectionCB), false );

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
    case 4: pm = "view_NZ";
	dir = uiStrings::sNorth(false).appendPhrase(uiStrings::sZ(),
				    uiString::Space, uiString::OnSameLine);
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
    polySelectionModeCB( 0 );
}


void uiODMenuMgr::polySelectionModeCB( CallBacker* cb )
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


void uiODMenuMgr::removeSelectionCB( CallBacker* )
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
    if ( !itm )
	{ pErrMsg("Huh?"); return; }

    const int id = itm->getID();
    switch( id )
    {
    case mManSurveyMnuItm:		applMgr().selectSurvey(0); break;
    case mSessSaveMnuItm:		appl_.saveSession(); break;
    case mSessRestMnuItm:		appl_.restoreSession(); break;
    case mSessAutoMnuItm:		appl_.autoSession(); break;
    case mImpAttrAsciiMnuItm:		mDoOp(Imp,Attr,0); break;
    case mImpAttrOthSurvMnuItm:		mDoOp(Imp,Attr,1); break;
    case mImpColTabMnuItm:		mDoOp(Imp,ColTab,0); break;
    case mImpSeisODCubeMnuItm:		mDoOp(Imp,Seis,0); break;
    case mImpSeisSimple3DMnuItm:	mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm:	mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm:	mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm:	mDoOp(Imp,Seis,8); break;
    case mImpSeisODCubeOtherSurvMnuItm:	mDoOp(Imp,Seis,9); break;
    case mExpSeisSimple3DMnuItm:	mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm:	mDoOp(Exp,Seis,6); break;
    case mExpSeisSimplePS3DMnuItm:	mDoOp(Exp,Seis,7); break;
    case mExpSeisSimplePS2DMnuItm:	mDoOp(Exp,Seis,8); break;
    case mExpSeisCubePositionsMnuItm:	mDoOp(Exp,Seis,9); break;
    case mImpHor3DAsciiMnuItm:		mDoOp(Imp,Hor,0); break;
    case mImpHor3DAsciiAttribMnuItm:	mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm:		mDoOp(Imp,Hor,2); break;
    case mImpBulkHor3DAsciiMnuItm:	mDoOp(Imp,Hor,3); break;
    case mImpBulkHor2DAsciiMnuItm:	mDoOp(Imp,Hor,4); break;
    case mExpHor3DAsciiMnuItm:		mDoOp(Exp,Hor,0); break;
    case mExpHor2DAsciiMnuItm:		mDoOp(Exp,Hor,1); break;
    case mExpBulkHor3DAsciiMnuItm:	mDoOp(Exp,Hor,2); break;
    case mExpBulkHor2DAsciiMnuItm:	mDoOp(Exp,Hor,3); break;
    case mExpFltAsciiMnuItm:		mDoOp(Exp,Flt,0); break;
    case mExpBulkFltAsciiMnuItm:	mDoOp(Exp,Flt,1); break;
    case mExpFltSSAsciiMnuItm:		mDoOp(Exp,Fltss,0); break;
    case mExpBulkFltSSAsciiMnuItm:	mDoOp(Exp,Fltss,1); break;
    case mExpFltSetAsciiMnuItm:		mDoOp(Exp,FltSet,0); break;
    case mImpWellAsciiTrackMnuItm:	mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:	mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:	mDoOp(Imp,Wll,2); break;
    case mImpWellSimpleMultiMnuItm:	mDoOp(Imp,Wll,4); break;
    case mImpBulkWellTrackItm:		mDoOp(Imp,Wll,5); break;
    case mImpBulkWellLogsItm:		mDoOp(Imp,Wll,6); break;
    case mImpBulkWellMarkersItm:	mDoOp(Imp,Wll,7); break;
    case mImpBulkWellD2TItm:		mDoOp(Imp,Wll,8); break;
    case mImpPickAsciiMnuItm:		mDoOp(Imp,Pick,0); break;
    case mExpPickAsciiMnuItm:		mDoOp(Exp,Pick,0); break;
    case mExpWvltAsciiMnuItm:		mDoOp(Exp,Wvlt,0); break;
    case mImpWvltAsciiMnuItm:		mDoOp(Imp,Wvlt,0); break;
    case mImpFltAsciiMnuItm:		mDoOp(Imp,Flt,0); break;
    case mImpBulkFltAsciiMnuItm:	mDoOp(Imp,Flt,1); break;
    case mImpFltSSAscii3DMnuItm:	mDoOp(Imp,Fltss,0); break;
    case mImpFltSSAscii2DMnuItm:	mDoOp(Imp,Fltss,1); break;
    case mImpFltSetAsciiMnuItm:		mDoOp(Imp,FltSet,0); break;
    case mImpFltSSAscii3DBulkMnuItm:	mDoOp(Imp,Fltss,2); break;
    case mImpFltSSAscii2DBulkMnuItm:	mDoOp(Imp,Fltss,3); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpXPlotAsciiMnuItm:		mDoOp(Imp,XPlot,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
    case mImpGeom2DAsciiMnuItm:		mDoOp(Imp,Geom2D,0); break;
    case mExpGeom2DAsciiMnuItm:		mDoOp(Exp,Geom2D,0); break;
    case mManColTabMnuItm:		mDoOp(Man,ColTab,0); break;
    case mManSeis3DMnuItm:		mDoOp(Man,Seis,0); break;
    case mManSeis2DMnuItm:		mDoOp(Man,Seis,1); break;
    case mManSeisPS3DMnuItm:		mDoOp(Man,Seis,2); break;
    case mManSeisPS2DMnuItm:		mDoOp(Man,Seis,3); break;
    case mManHor3DMnuItm:		mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm:		mDoOp(Man,Hor,1); break;
    case mManFltSSMnuItm:		mDoOp(Man,Flt,1); break;
    case mManFltMnuItm:			mDoOp(Man,Flt,2); break;
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
    case mManGeom2DMnuItm:		mDoOp(Man,Geom2D,0); break;
    case mManXPlotMnuItm:		mDoOp(Man,XPlot,0); break;

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
    case mT2DConv2DMnuItm:	applMgr().processTime2Depth( true ); break;
    case mT2DConv3DMnuItm:	applMgr().processTime2Depth( false ); break;
    case mCreateSurf2DMnuItm:	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm:	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mCreate2DFrom3DMnuItm:	applMgr().create2DGrid(); break;
    case mExtract2DFrom3DMnuItm: applMgr().create2DFrom3D(); break;
    case mCreate3DFrom2DMnuItm:	applMgr().create3DFrom2D(); break;
    case mStartBatchJobMnuItm:	applMgr().startBatchJob(); break;
    case mXPlotMnuItm:		applMgr().doWellXPlot(); break;
    case mAttrXPlotMnuItm:	applMgr().doAttribXPlot(); break;
    case mOpenXPlotMnuItm:	applMgr().openCrossPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mAddTimeDepth2DMnuItm:	applMgr().addTimeDepthScene(true); break;
    case mAddTimeDepth3DMnuItm:	applMgr().addTimeDepthScene(false); break;
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

    case mSettingsMnuItm: {
	uiSettingsDlg dlg( &appl_ );
	dlg.go();
    } break;

    case mDumpDataPacksMnuItm: {
	uiFileSelector::Setup fssu( "/tmp/dpacks.txt" );
	fssu.setForWrite().setFormat( File::Format::textFiles() );
	fssu.confirmoverwrite( false );
	uiFileSelector uifs( &appl_, fssu );
	uifs.caption() = tr("Data pack dump");
	if ( uifs.go() )
	{
	    od_ostream strm( uifs.fileName() );
	    if ( strm.isOK() )
		DataPackMgr::dumpDPMs( strm );
	}
    } break;
    case mDisplayMemoryMnuItm: {
	IOPar iopar;
	OD::dumpMemInfo( iopar );
	BufferString text;
	iopar.dumpPretty( text );
	uiDialog dlg( &appl_,
	     uiDialog::Setup(tr("Memory Information"),mNoDlgTitle,mNoHelpKey) );
	uiTextBrowser* browser = new uiTextBrowser( &dlg );
	browser->setText( text.buf() );
	dlg.setCancelText( uiString::empty() );
	dlg.go();
    } break;

    case mSettAdvPython: {
	uiDialog* dlg = uiAdvSettings::getPythonDlg( &appl_ );
	dlg->go();
    } break;

    case mSettAdvPersonal:
    case mSettAdvSurvey: {
	uiAdvSettings dlg( &appl_, tr("Browse/Edit Settings"),
		id == mSettAdvPersonal ? 0 : uiAdvSettings::sKeySurveyDefs() );
	dlg.go();
    } break;

    case mStereoOffsetMnuItm:
	applMgr().setStereoOffset();
    break;
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
	    sceneMgr().setActiveScene( id-mSceneSelMnuItm );
	    itm->setChecked( true );
	}
	else if ( id > mHelpMnu )
	    helpmnumgr_->handle( id );
    } break;

    }
}


int uiODMenuMgr::ask2D3D( const uiString& txt, int res2d, int res3d,
			  int rescncl )
{
    mGet2D3D();

    int res = rescncl;
    if ( !have2d )
	res = res3d;
    else if ( !have3d )
	res = res2d;
    else
    {
	const int msg = gUiMsg().ask2D3D( txt, true );
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
    mGet2D3D();

    appl_.applMgr().seisServer()->manageSeismics( !have2d ? 0 : 2 );
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
    dlg->go();
}


void uiODMenuMgr::showFirewallProcDlg()
{
    if ( __iswin__ )
    {
	uiFirewallProcSetter* dlg = new uiFirewallProcSetter( &appl_ );
	dlg->go();
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
    setSurveySubMenus();
    fillAnalMenu();
    fillProcMenu();
    fillSceneMenu();
    dTectMnuChanged.trigger();
}


void uiODMenuMgr::showHostID()
{
    uiHostIDDlg dlg( &appl_ );
    dlg.go();
}
