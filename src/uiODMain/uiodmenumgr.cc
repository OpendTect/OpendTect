/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmenumgr.h"

#include "ui3dviewer.h"
#include "uiautosaverdlg.h"
#include "uicrdevenv.h"
#include "uifileselector.h"
#include "uiglinfo.h"
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
#include "visemobjdisplay.h"

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
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
static const char* sTODOIcon = "icontodo";
static const char* ascic = "ascii";
static const char* singic = "single";
static const char* multic = "multiple";
static const uiString sAsciiDots = m3Dots( uiStrings::sASCII() );
static const uiString singstr = uiStrings::sSingle();
static const uiString multstr = uiStrings::sMultiple();
#define mGet2D3D() \
    const bool mUnusedVar have2d = DBM().isBad() || SI().has2D(); \
    const bool mUnusedVar have3d = DBM().isBad() || SI().has3D()
#define mGet2D3DWithOneChoice() \
    mGet2D3D(); \
    bool haveonechoice = !have2d || !have3d; \
    if ( always3d && !have3d ) \
	haveonechoice = false


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmnumgr_(0)
    , langmnumgr_(0)
    , measuretoolman_(0)
    , inviewmode_(false)
    , plugintb_(0)
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

    faulttoolman_ = new uiODFaultToolMan( appl_ );

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
    helpmnumgr_ = new uiODHelpMenuMgr( this );
    langmnumgr_ = new uiODLangMenuMgr( this );

    fillDtectTB( appman );
    fillVisTB( sceneman );
    fillManTB();

    measuretoolman_ = new MeasureToolMan( appl_ );
}


uiMenu* uiODMenuMgr::addSubMenu( uiMenu* parmnu, const uiString& nm,
				 const char* icnm )
{
    uiMenu* mnu = new uiMenu( &appl_, nm );
    mnu->setIcon( icnm );

    if ( parmnu == impmnu_ )
	impmnus_ += mnu;
    else if ( parmnu == expmnu_ )
	expmnus_ += mnu;

    parmnu->addMenu( mnu );
    return mnu;
}


void uiODMenuMgr::addDirectAction( uiMenu* mnu, const uiString& nm,
				   const char* icnm, int id )
{
    uiAction* uiact = new uiAction( nm, mCB(this,uiODMenuMgr,handleClick),
				    icnm );
    mnu->insertAction( uiact, id );
}


void uiODMenuMgr::addAction( uiMenu* mnu, const uiString& nm,
				const char* icnm, int id )
{
    addDirectAction( mnu, m3Dots(nm), icnm, id );
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
    addAction( ascmnu, altascnm ? *altascnm : sAsciiDots, ascic, id );
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


void uiODMenuMgr::add2D3DActions( uiMenu* mnu, const uiString& nm,
				  const char* icnm, int id1, int id2,
				  bool always3d )
{
    mGet2D3DWithOneChoice();

    if ( haveonechoice )
	addAction( mnu, nm, icnm, have2d ? id1 : id2 );
    else
    {
	uiMenu* submnu = addSubMenu( mnu, nm, icnm );
	addAction( submnu, uiStrings::s2D(), "2d", id1 );
	addAction( submnu, uiStrings::s3D(), "3d", id2 );
    }
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
	addAction( mnu2d, multstr, multic, id3dsing );
	uiMenu* mnu3d = addSubMenu( ascmnu, uiStrings::s3D(), "3d" );
	addAction( mnu3d, singstr, singic, id3dsing );
	addAction( mnu3d, multstr, multic, id3dmult );
    }
    return ascmnu;
}


void uiODMenuMgr::fillSurveyMenu()
{
    addAction( surveymnu_, tr("Select/Setup"), "survey", mManSurveyMnuItm );

    uiMenu* sessionmnu = addSubMenu( surveymnu_, tr("Session"), "session" ) ;
    addAction( sessionmnu, uiStrings::sSave(), "save", mSessSaveMnuItm );
    addAction( sessionmnu, tr("Restore"), "spiral_in", mSessRestMnuItm );
    addAction( sessionmnu, tr("Auto-Load"), "autoload", mSessAutoMnuItm );

    surveymnu_->insertSeparator();

    impmnu_ = addSubMenu( surveymnu_, uiStrings::sImport(), "import" );
    expmnu_ = addSubMenu( surveymnu_, uiStrings::sExport(), "export" );
    manmnu_ = addSubMenu( surveymnu_, tr("Manage"), "manage" );
    preloadmnu_ = addSubMenu( surveymnu_, tr("Pre-Load"), "preloaded" );

    setSurveySubMenus();

    surveymnu_->insertSeparator();

    addDirectAction( surveymnu_, uiStrings::sExit(), "exit", mExitMnuItm );
}


void uiODMenuMgr::setSurveySubMenus()
{
    impmnu_->clear(); expmnu_->clear(); manmnu_->clear(); preloadmnu_->clear();
    impmnus_.setEmpty(); expmnus_.setEmpty();
    mGet2D3D();

    uiMenu* ascmnu;
    uiString mnunm; const char* iconnm;

    mnunm = tr( "Attribute Sets" ); iconnm = "attributes";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpAttrAsciiMnuItm );
    expmnus_ += 0;
    add2D3DActions( manmnu_, mnunm, iconnm, mManAttr2DMnuItm, mManAttr3DMnuItm);

    mnunm = tr("Bodies"); iconnm = "tree-body";
    impmnus_ += 0;
    addAction( impmnu_, mnunm, iconnm, mManBodyMnuItm );
    expmnus_ += 0;

    mnunm = tr("Color Tables"); iconnm = "colorbar";
    impmnus_ += 0;
    addAction( impmnu_, mnunm, iconnm, mImpColTabMnuItm );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManColTabMnuItm );

    mnunm = tr("Cross-Plot Data"); iconnm = "xplot";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpXPlotAsciiMnuItm );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManXPlotMnuItm );

    mnunm = tr("Faults"); iconnm = "tree-flt";
    addSingMultAsciiSubMenu( impmnu_, mnunm, iconnm,
			     mImpFltAsciiMnuItm, mImpBulkFltAsciiMnuItm );
    addSingMultAsciiSubMenu( expmnu_, mnunm, iconnm,
			     mExpFltAsciiMnuItm, mExpBulkFltAsciiMnuItm );

    mnunm = tr("FaultStick Sets"); iconnm = "tree-fltss";
    add2D3DSingMultAsciiSubMenu( impmnu_, mnunm, iconnm,
	    mImpFltSSAscii2DMnuItm, mImpFltSSAscii3DMnuItm,
	    mImpFltSSAscii2DBulkMnuItm, mImpFltSSAscii3DBulkMnuItm );
    addSingMultAsciiSubMenu( expmnu_, mnunm, iconnm,
			     mExpFltSSAsciiMnuItm, mExpBulkFltSSAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManFltSSMnuItm );

    impmnus_ += 0;
    if ( !have2d )
	expmnus_ += 0;
    else
    {
	mnunm = tr("2D Geometries"); iconnm = "tree-geom2d";
	addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpGeom2DAsciiMnuItm );
	addAction( manmnu_, mnunm, iconnm, mManGeom2DMnuItm );
    }

    mnunm = tr("Horizons"); iconnm = have2d ? "horizons" : "tree-horizon3d";
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

    mnunm = tr("PointSets/Polygons"); iconnm = "pointspolygons";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpPickAsciiMnuItm );
    impmnus_ += impmnus_.last(); // because both Pick and Poly in enum
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpPickAsciiMnuItm );
    expmnus_ += expmnus_.last();
    addAction( manmnu_, mnunm, iconnm, mManPickMnuItm );

    mnunm = tr("Probability Density Functions"); iconnm = "prdfs";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpPDFAsciiMnuItm );
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpPDFAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManPDFMnuItm );

    mnunm = tr("Random Lines"); iconnm = "tree-randomline";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManRanLMnuItm );

    createSeisSubMenus();

    mnunm = tr("Sessions"); iconnm = "session";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManSessMnuItm );

    mnunm = tr("Stratigraphy"); iconnm = "strat_tree";
    impmnus_ += 0;
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManStratMnuItm );

    mnunm = tr("Velocities"); iconnm = "velocity_cube";
    impmnus_ += 0;
    expmnus_ += 0;
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpVelocityAsciiMnuItm );

    mnunm = tr("Wavelets"), iconnm = "wavelet";
    addAsciiActionSubMenu( impmnu_, mnunm, iconnm, mImpWvltAsciiMnuItm );
    addAsciiActionSubMenu( expmnu_, mnunm, iconnm, mExpWvltAsciiMnuItm );
    addAction( manmnu_, mnunm, iconnm, mManWvltMnuItm );

    mnunm = tr("Wells"); iconnm = "well";
    fillWellImpSubMenu( addSubMenu(impmnu_,mnunm,iconnm) );
    expmnus_ += 0;
    addAction( manmnu_, mnunm, iconnm, mManWellMnuItm );

    impmnu_->insertSeparator(); expmnu_->insertSeparator();
    manmnu_->insertSeparator(); preloadmnu_->insertSeparator();
}


void uiODMenuMgr::fillWellImpSubMenu( uiMenu* mnu )
{
    imptrackmnu_ = addSubMenu( mnu, tr( "Track/Time-Depth" ), sTODOIcon );
    implogsmnu_ = addSubMenu( mnu, uiStrings::sLogs(), sTODOIcon );
    impmarkersmnu_ = addSubMenu( mnu, uiStrings::sMarker(mPlural), sTODOIcon );
    addAction( mnu, tr("Simple Multi-Well"), sTODOIcon,
		mImpWellSimpleMultiMnuItm );

    uiMenu* ascmnu = addSubMenu( imptrackmnu_, uiStrings::sASCII(), ascic );
    addAction( ascmnu, singstr, singic, mImpWellAsciiTrackMnuItm );
    addAction( ascmnu, tr("Multiple Tracks"), multic, mImpBulkWellTrackItm );
    addAction( ascmnu, tr("Multiple Depth-Time Models"), multic,
					mImpBulkWellD2TItm );

    ascmnu = addSubMenu( implogsmnu_, uiStrings::phrASCII(toUiString("(LAS)")),
			    ascic );
    addAction( ascmnu, singstr, singic, mImpWellAsciiLogsMnuItm );
    addAction( ascmnu, multstr, multic, mImpBulkWellLogsItm );

    ascmnu = addSubMenu( impmarkersmnu_, uiStrings::sASCII(), ascic );
    addAction( ascmnu, tr("Single-Well"), singic, mImpWellAsciiMarkersMnuItm );
    addAction( ascmnu, tr("Multi-Well"), multic, mImpBulkWellMarkersItm );
}


void uiODMenuMgr::createSeisSubMenus()
{
    uiString mnunm = tr("Seismic Data");
    const char* iconnm = "seis";
    uiMenu* impseis = addSubMenu( impmnu_, mnunm, iconnm );
    uiMenu* expseis = addSubMenu( expmnu_, mnunm, iconnm );
    add2D3DActions( manmnu_, mnunm, iconnm, mManSeis2DMnuItm, mManSeis3DMnuItm);
    addAction( preloadmnu_, mnunm, iconnm, mPreLoadSeisMnuItm );

    const uiString simpfilestr = tr("Simple File");
    uiMenu* simpimpmnu = addSubMenu( impseis, simpfilestr, ascic );
    uiMenu* simpexpmnu = addSubMenu( expseis, simpfilestr, ascic );
    const uiString linestr = tr("Line");
    const uiString cubestr = tr("Cube");
    const uiString psdatastr = tr("Pre-Stack Data");
    mGet2D3D();
    if ( !have2d || !have3d )
    {
	mnunm = have2d ? linestr : cubestr;
	iconnm = have2d ? "seismicline2d" : "seismiccube";
	addAction( simpimpmnu, mnunm, iconnm,
		have2d ? mImpSeisSimple2DMnuItm : mImpSeisSimple3DMnuItm );
	addAction( simpexpmnu, mnunm, iconnm,
		have2d ? mExpSeisSimple2DMnuItm : mExpSeisSimple3DMnuItm );
	mnunm = psdatastr;
	iconnm = have2d ? "prestackdataset2d" : "prestackdataset";
	addAction( simpimpmnu, mnunm, iconnm,
		have2d ? mImpSeisSimplePS2DMnuItm : mImpSeisSimplePS3DMnuItm );
	addAction( simpexpmnu, mnunm, iconnm,
		have2d ? mExpSeisSimplePS2DMnuItm : mExpSeisSimplePS3DMnuItm );
    }
    else
    {
	uiMenu* mnu2dimp = addSubMenu( simpimpmnu, uiStrings::s2D(), "2d" );
	uiMenu* mnu3dimp = addSubMenu( simpimpmnu, uiStrings::s3D(), "3d" );
	uiMenu* mnu2dexp = addSubMenu( simpexpmnu, uiStrings::s2D(), "2d" );
	uiMenu* mnu3dexp = addSubMenu( simpexpmnu, uiStrings::s3D(), "3d" );
	mnunm = linestr; iconnm = "seismicline2d";
	addAction( mnu2dimp, mnunm, iconnm, mImpSeisSimple2DMnuItm );
	addAction( mnu2dexp, mnunm, iconnm, mExpSeisSimple2DMnuItm );
	mnunm = cubestr; iconnm = "seismiccube";
	addAction( mnu3dimp, mnunm, iconnm, mImpSeisSimple3DMnuItm );
	addAction( mnu3dexp, mnunm, iconnm, mExpSeisSimple3DMnuItm );
	mnunm = psdatastr; iconnm = "prestackdataset2d";
	addAction( mnu2dimp, mnunm, iconnm, mImpSeisSimplePS2DMnuItm );
	addAction( mnu2dexp, mnunm, iconnm, mExpSeisSimplePS2DMnuItm );
	mnunm = psdatastr; iconnm = "prestackdataset";
	addAction( mnu3dimp, mnunm, iconnm, mImpSeisSimplePS3DMnuItm );
	addAction( mnu3dexp, mnunm, iconnm, mExpSeisSimplePS3DMnuItm );
    }

    if ( have3d )
    {
	mnunm = uiStrings::sOpendTect(); iconnm = "od";
	uiMenu* odimpmnu = addSubMenu( impseis, mnunm, iconnm );
	addAction( odimpmnu, tr("From File"), "singlefile",
		   mImpSeisODCubeMnuItm );
	addAction( odimpmnu, tr("From Other Survey"), "impfromothsurv",
		   mImpSeisODCubeOtherSurvMnuItm );
    }
}



#define mInsertItem(menu,txt,id) \
    menu->insertAction( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), id )

#define mInsertPixmapItem(menu,txt,id,pmfnm) { \
    menu->insertAction( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick), \
			pmfnm), id ); }


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

    csomnu_ = addSubMenu( procmnu_, tr("Create Seismic Output"), sTODOIcon );

// Attributes
    uiMenu* attritm = new uiMenu( uiStrings::sAttribute(mPlural) );
    csomnu_->addMenu( attritm );

    add2D3DMenuItem( *attritm, "seisout", tr("Single Attribute"),
		     mSeisOut2DMnuItm, mSeisOut3DMnuItm );
    mGet2D3D();

    if ( have3d )
    {
	attritm->insertAction(
	    new uiAction( m3Dots(tr("Multi Attribute")),
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
    if ( have3d )
    {
	itm2d3d = new uiMenu( menutext );
	csomnu_->addMenu( itm2d3d );
	mInsertItem( itm2d3d, m3Dots(tr("Create 2D from 3D")),
		     mCreate2DFrom3DMnuItm );
	mInsertItem( itm2d3d, m3Dots(tr("Extract 2D from 3D")),
		     m2DFrom3DMnuItm );
    }

    if ( have2d )
    {
#ifdef __debug__
	if ( !itm2d3d )
	{
	itm2d3d = new uiMenu( menutext );
	    csomnu_->addMenu( itm2d3d );
	}
	mInsertItem( itm2d3d, m3Dots(tr("Create 3D from 2D")), m3DFrom2DMnuItm);
#endif
    }

    if ( have3d )
    {
// Other 3D items
	csomnu_->insertAction(
	    new uiAction(m3Dots(tr("Angle Mute Function")),
			mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) ));
	csomnu_->insertAction(
	    new uiAction(m3Dots(tr("Bayesian Classification")),
			mCB(&applMgr(),uiODApplMgr,bayesClass3D), "bayes"));
	csomnu_->insertAction(
	    new uiAction(m3Dots(tr("From Well Logs")),
			mCB(&applMgr(),uiODApplMgr,createCubeFromWells) ));
    }

    add2D3DMenuItem( *csomnu_, "empty", tr("Prestack Processing"),
		     mPSProc2DMnuItm, mPSProc3DMnuItm );

    if ( have3d )
    {
// Velocity
	uiMenu* velitm = new uiMenu( tr("Velocity") );
	csomnu_->addMenu( velitm );
	velitm->insertAction(
	    new uiAction(m3Dots(tr("Time - Depth Conversion")),
			 mCB(&applMgr(),uiODApplMgr,processTime2Depth)) );
	velitm->insertAction(
	    new uiAction(m3Dots(tr("Velocity Conversion")),
			 mCB(&applMgr(),uiODApplMgr,processVelConv)) );
    }

    add2D3DMenuItem( *csomnu_, "empty", tr("Volume Builder"),
		     mVolProc2DMnuItm, mVolProc3DMnuItm );

    uiMenu* grditm = new uiMenu( &appl_, tr("Create Horizon Output") );
    add2D3DMenuItem( *grditm, "", uiStrings::sAttribute(mPlural),
		     mCreateSurf2DMnuItm, mCreateSurf3DMnuItm );
    procmnu_->addMenu( grditm );

    procwellmnu_ = new uiMenu( &appl_, uiStrings::sWells(), "well" );
    procwellmnu_->insertAction( new uiAction(m3Dots(uiStrings::sRockPhy()),
		mCB(&applMgr(),uiODApplMgr,launchRockPhysics),"rockphys") );
    procmnu_->addMenu( procwellmnu_ );

    mInsertItem( procmnu_, m3Dots(tr("(Re-)Start Batch Job")),
		 mStartBatchJobMnuItm );
}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();
    OD::Pol2D3D survtype( OD::Both2DAnd3D );
    if ( !DBM().isBad() )
	survtype = SI().survDataType();

    const char* attrpm = "attributes";
    if ( survtype == OD::Both2DAnd3D )
    {
	uiMenu* aitm = new uiMenu( &appl_, uiStrings::sAttribute(mPlural),
				   attrpm );
	mInsertPixmapItem( aitm, m3Dots(uiStrings::s2D()), mEdit2DAttrMnuItm,
			   "attributes_2d" );
	mInsertPixmapItem( aitm, m3Dots(uiStrings::s3D()), mEdit3DAttrMnuItm,
			   "attributes_3d" );

	analmnu_->addMenu( aitm );
	analmnu_->insertSeparator();

	uiMenu* vpitm = new uiMenu( &appl_, tr("Volume Builder"),
				   VolProc::uiChain::pixmapFileName() );
	mInsertPixmapItem( vpitm, m3Dots(uiStrings::s2D()), mVolProc2DMnuItm,
			   VolProc::uiChain::pixmapFileName() );
	mInsertPixmapItem( vpitm, m3Dots(uiStrings::s3D()), mVolProc3DMnuItm,
			   VolProc::uiChain::pixmapFileName() );

	analmnu_->addMenu( vpitm );
    }
    else
    {
	mInsertPixmapItem( analmnu_, m3Dots(uiStrings::sAttribute(mPlural)),
			   mEditAttrMnuItm, attrpm)
	analmnu_->insertSeparator();

	analmnu_->insertAction( new uiAction( m3Dots(tr("Volume Builder")),
	    survtype != OD::Only2D ? mCB(&applMgr(),uiODApplMgr,doVolProc3DCB)
				   : mCB(&applMgr(),uiODApplMgr,doVolProc2DCB),
				VolProc::uiChain::pixmapFileName() ) );
    }


    uiMenu* xplotmnu = new uiMenu( &appl_, tr("Cross-plot Data"),
				   "xplot");
    mInsertPixmapItem( xplotmnu, m3Dots(tr("Well logs vs Attributes")),
		       mXplotMnuItm, "xplot_wells" );
    mInsertPixmapItem( xplotmnu, m3Dots(tr("Attributes vs Attributes")),
		       mAXplotMnuItm, "xplot_attribs" );
    mInsertItem( xplotmnu, m3Dots(tr("Open Cross-plot")), mOpenXplotMnuItm );
    analmnu_->addMenu( xplotmnu );

    analwellmnu_ = new uiMenu( &appl_, uiStrings::sWells(), "well" );
    analwellmnu_->insertAction( new uiAction( m3Dots(tr("Edit Logs")),
	mCB(&applMgr(),uiODApplMgr,doWellLogTools), "well_props" ) );
    if (  !DBM().isBad() && SI().zIsTime() )
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
    mInsertItem( scenemnu_, uiStrings::sNew(), mAddSceneMnuItm );
    mInsertItem( scenemnu_, tr("New Map View"), mAddMapSceneMnuItm );

    addtimedepthsceneitm_ = new uiAction( ::toUiString("Dummy"),
					  mCB(this,uiODMenuMgr,handleClick) );
    scenemnu_->insertAction( addtimedepthsceneitm_, mAddTmeDepthMnuItm );

    add2D3DMenuItem( *scenemnu_, "empty", tr("New [Horizon Flattened]"),
		     mAddHorFlat2DMnuItm, mAddHorFlat3DMnuItm );
    lastsceneitm_ = scenemnu_->insertSeparator();

    mInsertItem( scenemnu_, tr("Cascade"), mCascadeMnuItm );
    uiMenu* tileitm = new uiMenu( &appl_, uiStrings::sTile() );
    scenemnu_->addMenu( tileitm );

    mInsertItem( tileitm, tr("Auto"), mTileAutoMnuItm );
    mInsertItem( tileitm, uiStrings::sHorizontal(), mTileHorMnuItm );
    mInsertItem( tileitm, uiStrings::sVertical(), mTileVerMnuItm );

    mInsertItem( scenemnu_, m3Dots(uiStrings::sProperties()),
		 mScenePropMnuItm );
    scenemnu_->insertSeparator();

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
	    itm = new uiAction( uiString::emptyString(),
				mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertAction( itm, id );
	    itm->setCheckable( true );
	}

	itm->setText( scenenms[idx] );
	itm->setChecked( idx==activescene );
    }

    uiString itmtxt = tr( "New [%1]" )
	 .arg( !DBM().isBad() && SI().zIsTime() ? uiStrings::sDepth()
						: uiStrings::sTime() );
    addtimedepthsceneitm_->setText( m3Dots(itmtxt) );
}


void uiODMenuMgr::fillViewMenu()
{
    viewmnu_->clear();
    mInsertItem( viewmnu_, m3Dots(tr("Work Area")), mWorkAreaMnuItm );
    mInsertItem( viewmnu_, m3Dots(tr("Z-Scale")), mZScaleMnuItm );
    mInsertItem( viewmnu_, m3Dots(tr("Viewer 2D")), m2DViewMnuItm );
    uiMenu* stereoitm = new uiMenu( &appl_, tr("Stereo Viewing") );
    viewmnu_->addMenu( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id) \
    itm = new uiAction( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertAction( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, tr("Off"), true, mStereoOffMnuItm)
    mInsertStereoItem( stereoredcyanitm_, tr("Red/Cyan"), false,
			mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, tr("Quad Buffer"), false,
			mStereoQuadMnuItm )

    stereooffsetitm_ = new uiAction( m3Dots(tr("Stereo Offset")),
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertAction( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.setName("Toolbars");
    viewmnu_->addMenu( &toolbarsmnu );
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
	mInsertItem( iconsmnu, toUiString(mnunm), mViewIconsMnuItm+nms.size() );
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
    mInsertItem( iconsmnu, tr("Default"), mViewIconsMnuItm+0 );
    BufferStringSet nms; nms.add( "Default" );
    addIconMnuItems( dlsett, iconsmnu, nms );
    addIconMnuItems( dlsite, iconsmnu, nms );
    addIconMnuItems( dlrel, iconsmnu, nms );
}


void uiODMenuMgr::fillUtilMenu()
{
    settmnu_ = new uiMenu( &appl_, uiStrings::sSettings() );
    utilmnu_->addMenu( settmnu_ );

    mInsertItem( settmnu_, m3Dots(tr("Auto-Save")), mSettAutoSaveMnuItm );
    mInsertItem( settmnu_, m3Dots(tr("Look and Feel")), mSettLkNFlMnuItm );
    mInsertItem( settmnu_, m3Dots(tr("Keyboard Shortcuts")),
		 mSettShortcutsMnuItm);
    uiMenu* advmnu = new uiMenu( &appl_, uiStrings::sAdvanced() );
    mInsertItem( advmnu, m3Dots(tr("Personal Settings")), mSettGeneral );
    mInsertItem( advmnu, m3Dots(tr("Survey Defaults")), mSettSurvey );
    settmnu_->addMenu( advmnu );

    toolsmnu_ = new uiMenu( &appl_, uiStrings::sTools() );
    utilmnu_->addMenu( toolsmnu_ );

    mInsertItem( toolsmnu_, m3Dots(tr("Batch Programs")), mBatchProgMnuItm );
    mInsertItem( toolsmnu_, m3Dots(tr("Position Conversion")), mPosconvMnuItm );
    BufferString develverstr;
    GetSpecificODVersion( "devel", develverstr );
    if ( !develverstr.isEmpty() )
	mInsertItem( toolsmnu_, m3Dots(tr("Create Plugin Devel. Env.")),
		     mCrDevEnvMnuItm );

    installmnu_ = new uiMenu( &appl_, tr("Installation") );
    utilmnu_->addMenu( installmnu_ );
    File::Path installerdir( ODInst::GetInstallerDir() );
    const bool hasinstaller = File::isDirectory( installerdir.fullPath() );
    if ( hasinstaller && !__ismac__ )
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	const bool aitfixed = ODInst::autoInstTypeIsFixed();
	if ( !aitfixed || ait == ODInst::UseManager || ait == ODInst::FullAuto )
	    mInsertItem( installmnu_, m3Dots(tr("Update")),
			 mInstMgrMnuItem );
	if ( !aitfixed )
	    mInsertItem( installmnu_, m3Dots(tr("Auto-update Policy")),
			 mInstAutoUpdPolMnuItm );
	installmnu_->insertSeparator();
    }

    mInsertItem( installmnu_, m3Dots(tr("Connection Settings")),
		 mInstConnSettsMnuItm );
    mInsertItem( installmnu_, m3Dots(tr("Plugins")), mPluginsMnuItm );
    mInsertItem( installmnu_, m3Dots(tr("Setup Multi-Machine Processing")),
		 mSetupBatchItm);
    mInsertItem( installmnu_, tr("Graphics Information"), mGraphicsInfoItm );

    const char* lmfnm = od_ostream::logStream().fileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, m3Dots(tr("Show Log File")), mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	mInsertItem( toolsmnu_, m3Dots(tr("DataPack Dump")),
		     mDumpDataPacksMnuItm);
	mInsertItem( toolsmnu_, m3Dots(tr("Display Memory Info")),
		     mDisplayMemoryMnuItm);
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
    mGet2D3D();

    if ( have2d && have3d )
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
	if ( have2d )
	    menu.insertAction( new uiAction(titledots,cb2d,iconnm), itmid2d );
	else if ( have3d )
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
		mCB(appman,uiODApplMgr,doVolProc3DCB) );

    const int xplotid = dtecttb_->addButton( "xplot", tr("Cross-plot") );
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
    mAddTB(dtecttb_,"2dlaunch",tr("Launch 2D Viewer"),
			false,launch2DViewer);

//    dTectTBChanged.trigger();
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
    mnu->insertAction( new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), itm );

void uiODMenuMgr::fillManTB()
{
    const int seisid =
	mAddTB(mantb_,"man_seis",
               uiStrings::phrManage(uiStrings::sVolDataName(true, true, false)),
                                    false,manSeis);
    const int horid = mAddTB(mantb_,"man_hor",
              uiStrings::phrManage( uiStrings::sHorizon(mPlural)),false,manHor);
    const int fltid = mAddTB(mantb_,"man_flt",
              uiStrings::phrManage( uiStrings::sFault(mPlural)),false,manFlt);
    mAddTB(mantb_,"man_wll",
           uiStrings::phrManage( uiStrings::sWells()),false,manWll);
    mAddTB(mantb_,"man_picks", uiStrings::phrManage(
			toUiString("%1/%2")
			   .arg(uiStrings::sPointSet(mPlural))
			   .arg(uiStrings::sPolygon(mPlural))),
                            false,manPick);
    mAddTB(mantb_,"man_body",
           uiStrings::phrManage( tr("Bodies/Regions")),false,manBody);
    mAddTB(mantb_,"man_wvlt",
           uiStrings::phrManage(uiStrings::sWavelet(mPlural)),false,manWvlt);
    mAddTB(mantb_,"man_strat",uiStrings::phrManage( uiStrings::sStratigraphy()),
           false,manStrat);

    mGet2D3D();

    uiMenu* seispopmnu = new uiMenu( &appl_, tr("Seismics Menu") );
    if ( have2d )
    {
	mAddPopupMnu( seispopmnu, tr("2D Seismics"), mManSeis2DMnuItm )
	mAddPopupMnu( seispopmnu, tr("2D Prestack Seismics"),
		      mManSeisPS2DMnuItm )
    }
    if ( have3d )
    {
	mAddPopupMnu( seispopmnu, tr("3D Seismics"), mManSeis3DMnuItm )
	mAddPopupMnu( seispopmnu, tr("3D Prestack Seismics"),
		      mManSeisPS3DMnuItm )
    }
    mantb_->setButtonMenu( seisid, seispopmnu, uiToolButton::InstantPopup );

    if ( have2d )
	mAddPopUp( tr("Horizon Menu"), tr("2D Horizons"),
		   tr("3D Horizons"),
		   mManHor2DMnuItm, mManHor3DMnuItm, horid );

    mAddPopUp( tr("Fault Menu"),uiStrings::sFault(mPlural),
               uiStrings::sFaultStickSet(mPlural),
	       mManFltMnuItm, mManFltSSMnuItm, fltid );
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiODMenuMgr,fn), fnm ); \
    mnu->insertAction( itm, idx ); }

void uiODMenuMgr::fillVisTB( uiODSceneMgr* scenemgr )
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
		mCB(this,uiODMenuMgr,polySelectionModeCB), true );
    uiMenu* mnu = new uiMenu( &appl_, tr("Menu") );
    mAddMnuItm( mnu, uiStrings::sPolygon(),
		handleToolClick, "polygonselect", 0 );
    mAddMnuItm( mnu, uiStrings::sRectangle(),
		handleToolClick, "rectangleselect", 1 );
    viewtb_->setButtonMenu( polyselectid_, mnu );

    removeselectionid_ = viewtb_->addButton( "trashcan", tr("Remove selection"),
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
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn(
	    !inviewmode_  && viewtb_->isOn(polyselectid_) );
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
    if ( !itm ) return; // Huh?

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
    case mImpFltSSAscii3DBulkMnuItm:	mDoOp(Imp,Fltss,2); break;
    case mImpFltSSAscii2DBulkMnuItm:	mDoOp(Imp,Fltss,3); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpXPlotAsciiMnuItm:		mDoOp(Imp,XPlot,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
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
    case mCreateSurf2DMnuItm:	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm:	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mCreate2DFrom3DMnuItm:	applMgr().create2DGrid(); break;
    case m2DFrom3DMnuItm:	applMgr().create2DFrom3D(); break;
    case m3DFrom2DMnuItm:	applMgr().create3DFrom2D(); break;
    case mStartBatchJobMnuItm:	applMgr().startBatchJob(); break;
    case mXplotMnuItm:		applMgr().doWellXPlot(); break;
    case mAXplotMnuItm:		applMgr().doAttribXPlot(); break;
    case mOpenXplotMnuItm:	applMgr().openCrossPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mAddTmeDepthMnuItm:	applMgr().addTimeDepthScene(); break;
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
    case mPosconvMnuItm:	applMgr().posConversion(); break;
    case mInstMgrMnuItem:	applMgr().startInstMgr(); break;
    case mInstAutoUpdPolMnuItm:	applMgr().setAutoUpdatePol(); break;
    case mCrDevEnvMnuItm:	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm:	showLogFile(); break;

    case mAddMapSceneMnuItm: {
	sceneMgr().tile();
    const int sceneid = sceneMgr().addScene( true, 0, tr("Map View") );
	ui3DViewer* vwr = sceneMgr().get3DViewer( sceneid );
	if ( vwr ) vwr->setMapView( true );
    } break;
    case mInstConnSettsMnuItm: {
	uiProxyDlg dlg( &appl_ ); dlg.go(); } break;

    case mSettAutoSaveMnuItm: {
	uiAutoSaverDlg dlg( &appl_ );
	dlg.go();
    } break;

    case mSettLkNFlMnuItm: {
	uiSettingsDlg dlg( &appl_ );
	dlg.go();
    } break;

    case mDumpDataPacksMnuItm: {
	uiFileSelector::Setup fssu( "/tmp/dpacks.txt" );
	fssu.setForWrite().setFormat( File::Format::textFiles() );
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
    dlg.setCancelText( uiString::emptyString() );
	dlg.go();
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set Personal Settings" );
	dlg.go();
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

    polySelectionModeCB( 0 );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    setSurveySubMenus();
    fillAnalMenu();
    fillProcMenu();
    fillSceneMenu();
    dTectMnuChanged.trigger();
}
