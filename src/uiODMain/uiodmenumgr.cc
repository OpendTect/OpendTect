/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodmenumgr.h"

#include "ui3dviewer.h"
#include "uicrdevenv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodfaulttoolman.h"
#include "uiodhelpmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uiproxydlg.h"
#include "uisettings.h"
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
#include "ioman.h"
#include "keystrs.h"
#include "measuretoolman.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsysmem.h"
#include "settings.h"
#include "od_ostream.h"
#include "survinfo.h"
#include "texttranslator.h"
#include "thread.h"


static const char* sKeyIconSetNm = "Icon set name";

uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmgr_(0)
    , measuretoolman_(0)
    , inviewmode_(false)
    , langmnu_(0)
{
    surveymnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&Survey")) );
    analmnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&Analysis")) );
    procmnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&Processing")) );
    scenemnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("S&cenes")) );
    viewmnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&View")) );
    utilmnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&Utilities")) );
    helpmnu_ = appl_.menuBar()->addMenu( new uiMenu(tr("&Help")) );

    dtecttb_ = new uiToolBar( &appl_, "OpendTect tools", uiToolBar::Top );
    cointb_ = new uiToolBar( &appl_, "Graphical tools", uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, "Manage data", uiToolBar::Right );

    faulttoolman_ = new uiODFaultToolMan( appl_ );

    uiVisPartServer* visserv = appl_.applMgr().visServer();
    visserv->createToolBars();

    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
    visserv->selectionmodechange.notify(
				mCB(this,uiODMenuMgr,selectionMode) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete appl_.removeToolBar( dtecttb_ );
    delete appl_.removeToolBar( cointb_ );
    delete appl_.removeToolBar( mantb_ );
    delete helpmgr_;
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

    fillDtectTB( appman );
    fillCoinTB( sceneman );
    fillManTB();

    measuretoolman_ = new MeasureToolMan( appl_ );
}


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
{ cointb_->turnOn( axisid_, shwaxis ); }

bool uiODMenuMgr::isSoloModeOn() const
{ return cointb_->isOn( soloid_ ); }

void uiODMenuMgr::enableMenuBar( bool yn )
{ appl_.menuBar()->setSensitive( yn ); }

void uiODMenuMgr::enableActButton( bool yn )
{
    if ( yn )
	{ cointb_->setSensitive( actviewid_, true ); return; }

    if ( !inviewmode_ )
	toggViewMode(0);
    cointb_->setSensitive( actviewid_, false );
}


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), id )

#define mInsertPixmapItem(menu,txt,id,pmfnm) { \
    menu->insertItem( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick), \
			pmfnm), id ); }

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
    mInsertPixmapItem( surveymnu_, "&Select/Setup ...", mManSurveyMnuItm,
		       "survey" )

    uiMenu* sessionitm = new uiMenu( &appl_, "S&ession" );
    mInsertItem( sessionitm, sSave(false), mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    mInsertItem( sessionitm, "&Auto-load ...", mSessAutoMnuItm );
    surveymnu_->insertItem( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiMenu( &appl_, "&Import" );
    fillImportMenu();
    surveymnu_->insertItem( impmnu_ );

    expmnu_ = new uiMenu( &appl_, "Ex&port" );
    fillExportMenu();
    surveymnu_->insertItem( expmnu_ );

    manmnu_ = new uiMenu( &appl_, "&Manage");
    fillManMenu();
    surveymnu_->insertItem( manmnu_ );

    preloadmnu_ = new uiMenu( &appl_, "&Pre-load");
    mInsertItem( preloadmnu_, "&Seismics ...", mPreLoadSeisMnuItm );
    mInsertItem( preloadmnu_, "&Horizons ...", mPreLoadHorMnuItm );
    surveymnu_->insertItem( preloadmnu_ );

    surveymnu_->insertSeparator();
    mInsertItem( surveymnu_, "E&xit", mExitMnuItm );
}



void uiODMenuMgr::fillImportMenu()
{
    impmnu_->clear();

    uiMenu* impattr = new uiMenu( &appl_, "&Attributes" );
    uiMenu* impseis = new uiMenu( &appl_, "&Seismics" );
    uiMenu* imphor = new uiMenu( &appl_, "&Horizons" );
    uiMenu* impfault = new uiMenu( &appl_, "&Faults" );
    uiMenu* impfaultstick = new uiMenu( &appl_, "F&aultStickSets" );
    uiMenu* impwell = new uiMenu( &appl_, "&Wells" );
    uiMenu* imppick = new uiMenu( &appl_, "&PickSets/Polygons" );
    uiMenu* impwvlt = new uiMenu( &appl_, "&Wavelets" );
    uiMenu* impmute = new uiMenu( &appl_, "&Mute Functions" );
    uiMenu* impcpd = new uiMenu( &appl_, "&Cross-plot data" );
    uiMenu* impvelfn = new uiMenu( &appl_, "&Velocity Functions" );
    uiMenu* imppdf =
	new uiMenu( &appl_, "Probability &Density Functions" );

    impmnu_->insertItem( impattr );
    mInsertItem( impmnu_, "&Color Table ...", mImpColTabMnuItm );
    impmnu_->insertItem( impcpd );
    impmnu_->insertItem( impfault );
    impmnu_->insertItem( impfaultstick );
    impmnu_->insertItem( imphor );
    impmnu_->insertItem( impmute );
    impmnu_->insertItem( imppick );
    impmnu_->insertItem( imppdf );
    impmnu_->insertItem( impseis );
    impmnu_->insertItem( impvelfn );
    impmnu_->insertItem( impwvlt );
    impmnu_->insertItem( impwell );
    impmnu_->insertSeparator();

    mInsertItem( impattr, "&ASCII ...", mImpAttrMnuItm );
//    mInsertItem( impattr, "&From other survey ...", mImpAttrOthSurvMnuItm );
    mInsertItem( imppick, "&ASCII ...", mImpPickAsciiMnuItm );
    mInsertItem( impwvlt, "&ASCII ...", mImpWvltAsciiMnuItm );
    mInsertItem( impmute, "&ASCII ...", mImpMuteDefAsciiMnuItm );
    mInsertItem( impcpd, "&ASCII ...", mImpPVDSAsciiMnuItm );
    mInsertItem( impvelfn, "&ASCII ...", mImpVelocityAsciiMnuItm );
    mInsertItem( imppdf, "RokDoc &ASCII ...", mImpPDFAsciiMnuItm );

    mInsertItem( impseis, "SEG-&Y ...", mImpSeisSEGYMnuItm );
    mInsertItem( impseis, "SEG-Y s&canned ...", mImpSeisSEGYDirectMnuItm );
    uiMenu* impseissimple = new uiMenu( &appl_, "&Simple file" );
    mInsertItem( impseissimple, "&3D ...", mImpSeisSimple3DMnuItm );
    mInsertItem( impseissimple, "&2D ...", mImpSeisSimple2DMnuItm );
    mInsertItem( impseissimple, "&Prestack 3D ...", mImpSeisSimplePS3DMnuItm );
    mInsertItem( impseissimple, "Pre&stack 2D ...", mImpSeisSimplePS2DMnuItm );
    impseis->insertItem( impseissimple );
    uiMenu* impcbvsseis = new uiMenu( &appl_, "&CBVS" );
    mInsertItem( impcbvsseis, "&From file ...", mImpSeisCBVSMnuItm );
    mInsertItem( impcbvsseis, "From other &survey ...",
					   mImpSeisCBVSOtherSurvMnuItm );
    impseis->insertItem( impcbvsseis );

    uiMenu* imphorasc = new uiMenu( &appl_, "&ASCII" );
    mInsertItem( imphorasc, "&Geometry 3D ...", mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, "&Attributes 3D ...", mImpHorAsciiAttribMnuItm );
    mInsertItem( imphorasc, "&Geometry 2D ...", mImpHor2DAsciiMnuItm );
    mInsertItem( imphorasc, "&Bulk 3D ...", mImpBulkHorAsciiMnuIm );
    imphor->insertItem( imphorasc );

    mInsertItem( impfault, "&ASCII ...", mImpFaultMnuItm );
    mInsertItem( impfaultstick, "&ASCII 3D ...", mImpFaultSSAscii3DMnuItm );
    mInsertItem( impfaultstick, "&ASCII 2D ...", mImpFaultSSAscii2DMnuItm );

    uiMenu* impwellasc = new uiMenu( &appl_, "&ASCII" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );
    mInsertItem( impwell, "&VSP (SEG-Y) ...", mImpWellSEGYVSPMnuItm );
    mInsertItem( impwell, "Simple &Multi-Well ...", mImpWellSimpleMnuItm );

    uiMenu* impwellbulk = new uiMenu( &appl_, "&Bulk" );
    mInsertItem( impwellbulk, "Track ... ", mImpBulkWellTrackItm );
    mInsertItem( impwellbulk, "Logs ...", mImpBulkWellLogsItm );
    mInsertItem( impwellbulk, "Markers ...", mImpBulkWellMarkersItm );
    mInsertItem( impwellbulk, "Depth/Time Model ...", mImpBulkWellD2TItm );
    impwell->insertItem( impwellbulk );

    impmnus_.erase();
    impmnus_.allowNull();
    impmnus_ += impseis; impmnus_ += imphor; impmnus_ += impfault;
    impmnus_ += impwell; impmnus_ += 0; impmnus_ += 0;
    impmnus_ += imppick; impmnus_ += 0; impmnus_ += 0;
    impmnus_ += impwvlt; impmnus_ += impmute; impmnus_ += impvelfn;
    impmnus_ += imppdf;
}


void uiODMenuMgr::fillExportMenu()
{
    expmnu_->clear();
    uiMenu* expseis = new uiMenu( &appl_, "&Seismics" );
    uiMenu* exphor = new uiMenu( &appl_, "&Horizons" );
    uiMenu* expflt = new uiMenu( &appl_, "&Faults" );
    uiMenu* expfltss = new uiMenu( &appl_, "F&aultStickSets" );
    uiMenu* exppick = new uiMenu( &appl_, "&PickSets/Polygons" );
    uiMenu* expwvlt = new uiMenu( &appl_, "&Wavelets" );
    uiMenu* expmute = new uiMenu( &appl_, "&Mute Functions" );
    uiMenu* exppdf =
	new uiMenu( &appl_, "Probability &Density Functions" );

    expmnu_->insertItem( expflt );
    expmnu_->insertItem( expfltss );
    expmnu_->insertItem( exphor );
    expmnu_->insertItem( expmute );
    expmnu_->insertItem( exppick );
    expmnu_->insertItem( exppdf );
    expmnu_->insertItem( expseis );
    expmnu_->insertItem( expwvlt );
    expmnu_->insertSeparator();

    uiMenu* expseissgy = new uiMenu( &appl_, "&SEG-Y" );
    mInsertItem( expseissgy, "&3D ...", mExpSeisSEGY3DMnuItm );
    mInsertItem( expseissgy, "&2D ...", mExpSeisSEGY2DMnuItm );
    mInsertItem( expseissgy, "&Prestack 3D ...", mExpSeisSEGYPS3DMnuItm );
    expseis->insertItem( expseissgy );
    uiMenu* expseissimple = new uiMenu( &appl_, "S&imple file" );
    mInsertItem( expseissimple, "&3D ...", mExpSeisSimple3DMnuItm );
    mInsertItem( expseissimple, "&2D ...", mExpSeisSimple2DMnuItm );
    mInsertItem( expseissimple, "&Prestack 3D ...", mExpSeisSimplePS3DMnuItm );
    expseis->insertItem( expseissimple );

    mInsertItem( exphor, "ASCII &3D...", mExpHorAscii3DMnuItm );
    mInsertItem( exphor, "ASCII &2D...", mExpHorAscii2DMnuItm );
    mInsertItem( expflt, "&ASCII ...", mExpFltAsciiMnuItm );
    mInsertItem( expfltss, "&ASCII ...", mExpFltSSAsciiMnuItm );
    mInsertItem( exppick, "&ASCII ...", mExpPickAsciiMnuItm );
    mInsertItem( expwvlt, "&ASCII ...", mExpWvltAsciiMnuItm );
    mInsertItem( expmute, "&ASCII ...", mExpMuteDefAsciiMnuItm );
    mInsertItem( exppdf, "RokDoc &ASCII ...", mExpPDFAsciiMnuItm );

    expmnus_.erase();
    expmnus_.allowNull();
    expmnus_ += expseis; expmnus_ += exphor; expmnus_+= expflt;
    expmnus_+=0; expmnus_+= 0; expmnus_+= 0;
    expmnus_+=exppick; expmnus_+= 0; expmnus_+= 0;
    expmnus_+=expwvlt; expmnus_+= expmute; expmnus_+= 0; expmnus_ += exppdf;
}


void uiODMenuMgr::fillManMenu()
{
    manmnu_->clear();
    mInsertPixmapItem( manmnu_, "2D &Geometry ...", mManGeomItm, "man2dgeom" );
    mInsertPixmapItem( manmnu_, "&Attribute Sets ...", mManAttrMnuItm,
			"man_attrs" );
    mInsertPixmapItem( manmnu_, "&Bodies ...", mManBodyMnuItm, "man_body" );
    mInsertPixmapItem( manmnu_, "Color Tables ...", mManColTabMnuItm,
		       uiIcon::None() );
    mInsertPixmapItem( manmnu_, "&Cross-plot data ...", mManCrossPlotItm,
			"manxplot" );
    mInsertPixmapItem( manmnu_, "&Faults ...", mManFaultMnuItm, "man_flt" )
    mInsertPixmapItem( manmnu_, "&FaultStickSets ...", mManFaultStickMnuItm,
			"man_fltss" );
    if ( SI().survDataType() == SurveyInfo::No2D )
	mInsertPixmapItem( manmnu_, "&Horizons ...", mManHor3DMnuItm,
			"man_hor" )
    else
    {
	uiMenu* mnu = new uiMenu( &appl_, "&Horizons", "man_hor");
	mInsertItem( mnu, "&2D ...", mManHor2DMnuItm );
	mInsertItem( mnu, "&3D ...", mManHor3DMnuItm );
	manmnu_->insertItem( mnu );
    }

    mInsertPixmapItem( manmnu_, "&Layer properties ...", mManPropsMnuItm,
			"man_props" );
    mInsertPixmapItem( manmnu_, "&PickSets/Polygons ...", mManPickMnuItm,
			"man_picks" );
    mInsertPixmapItem( manmnu_, "Probability &Density Functions ...",
		 mManPDFMnuItm, "man_prdfs" );
    create2D3DMnu( manmnu_, "&Seismics", mManSeis2DMnuItm, mManSeis3DMnuItm,
			"man_seis" );
    create2D3DMnu( manmnu_, "Seismics Prestack", mManSeisPS2DMnuItm,
		   mManSeisPS3DMnuItm, "man_ps" );
    mInsertPixmapItem( manmnu_, "S&essions ...", mManSessMnuItm, uiIcon::None())
    mInsertPixmapItem( manmnu_, "Strati&graphy ...", mManStratMnuItm,
			"man_strat" )
    mInsertPixmapItem( manmnu_, "Wa&velets ...", mManWvltMnuItm, "man_wvlt" )
    mInsertPixmapItem( manmnu_, "&Wells ...", mManWellMnuItm, "man_wll"  )
    manmnu_->insertSeparator();
}


void uiODMenuMgr::create2D3DMnu( uiMenu* itm, const char* title,
				 int id2d, int id3d, const char* pmfnm )
{
    if ( SI().has2D() && SI().has3D() )
    {
	uiMenu* mnu = new uiMenu( &appl_, title, pmfnm );
	mInsertItem( mnu, "&2D ...", id2d );
	mInsertItem( mnu, "&3D ...", id3d );
	itm->insertItem( mnu );
    }
    else
    {
	const BufferString titledots( title, " ..." );
	if ( SI().has2D() )
	    mInsertPixmapItem( itm, titledots, id2d, pmfnm )
	else if ( SI().has3D() )
	    mInsertPixmapItem( itm, titledots, id3d, pmfnm )
    }
}


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

    uiMenu* csoitm = new uiMenu( &appl_, "&Create Seismic Output" );
    create2D3DMnu( csoitm, "&Attribute", mSeisOut2DMnuItm, mSeisOut3DMnuItm,
		   "seisout");
    if ( SI().has3D() )
    {
	csoitm->insertItem(
	    new uiAction("Multi attribute ...",
			mCB(&applMgr(),uiODApplMgr,createMultiAttribVol)) );
	csoitm->insertItem(
	    new uiAction("Volume &Builder ...",
			mCB(&applMgr(),uiODApplMgr,createVolProcOutput)) );
	csoitm->insertItem(
	    new uiAction("&Time - depth conversion ...",
			mCB(&applMgr(),uiODApplMgr,processTime2Depth)) );
	csoitm->insertItem(
	    new uiAction("&Velocity conversion ...",
			mCB(&applMgr(),uiODApplMgr,processVelConv)) );
	csoitm->insertItem(
	    new uiAction("&Prestack processing ...",
			mCB(&applMgr(),uiODApplMgr,processPreStack)) );
	csoitm->insertItem(
	    new uiAction("&Create MultiCube DataStore ...",
			mCB(&applMgr(),uiODApplMgr,createMultiCubeDS)) );
	csoitm->insertItem(
	    new uiAction("Angle mute function ...",
			mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) ));
	csoitm->insertItem(
	    new uiAction("Bayesian &Classification ...",
			mCB(&applMgr(),uiODApplMgr,bayesClass3D), "bayes"));
	csoitm->insertItem(
	    new uiAction("Create from &wells ...",
			mCB(&applMgr(),uiODApplMgr,createCubeFromWells) ));
	mInsertItem( csoitm, "Create 2D from 3D ...", m2DFrom3DMnuItem );
    }
    if ( SI().has2D() )
    {
	mInsertItem( csoitm, "Create 3D from 2D ...", m3DFrom2DMnuItem );
    }

    create2D3DMnu( csoitm, "&Between horizons", mCompBetweenHor2DMnuItm,
		   mCompBetweenHor3DMnuItm, "betweenhors" );
    create2D3DMnu( csoitm, "&Along horizon", mCompAlongHor2DMnuItm,
		   mCompAlongHor3DMnuItm, "alonghor" );
    csoitm->insertItem( new uiAction("SEG-&Y Scanned Re-sort ...",
		    mCB(&applMgr(),uiODApplMgr,resortSEGY)) );

    procmnu_->insertItem( csoitm );

    uiMenu* grditm = new uiMenu( &appl_, "Create &Horizon Output");
    create2D3DMnu( grditm, "&Attribute", mCreateSurf2DMnuItm,
		   mCreateSurf3DMnuItm, "ongrid" );
    procmnu_->insertItem( grditm );

    mInsertItem( procmnu_, "(Re-)Start Batch &Job...", mStartBatchJobMnuItm );
}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();
    SurveyInfo::Pol2D survtype = SI().survDataType();
    const char* attrpm = "attributes";
    if ( survtype == SurveyInfo::Both2DAnd3D )
    {
	uiMenu* aitm = new uiMenu( &appl_, "&Attributes", attrpm );
	mInsertPixmapItem( aitm, "&2D ...", mEdit2DAttrMnuItm, "attributes_2d");
	mInsertPixmapItem( aitm, "&3D ...", mEdit3DAttrMnuItm, "attributes_3d");

	analmnu_->insertItem( aitm );
	analmnu_->insertSeparator();
    }
    else
    {
	mInsertPixmapItem( analmnu_, "&Attributes ...", mEditAttrMnuItm, attrpm)
	analmnu_->insertSeparator();
    }

    if ( survtype!=SurveyInfo::Only2D )
    {
	analmnu_->insertItem( new uiAction( "Volume Builder ...",
				mCB(&applMgr(),uiODApplMgr,doVolProcCB),
				VolProc::uiChain::pixmapFileName() ) );
    }

    uiMenu* xplotmnu = new uiMenu( &appl_, "&Cross-plot", "xplot");
    mInsertPixmapItem( xplotmnu, "&Well logs <--> Attributes ...",
		       mXplotMnuItm, "xplot_wells" );
    mInsertPixmapItem( xplotmnu, "&Attributes <--> Attributes ...",
		       mAXplotMnuItm, "xplot_attribs" );
    mInsertItem( xplotmnu, "&Open Cross-plot ...", mOpenXplotMnuItm );
    analmnu_->insertItem( xplotmnu );

    analwellmnu_ = new uiMenu( &appl_, "&Wells", "well" );
    analwellmnu_->insertItem( new uiAction( "&Edit logs ...",
	mCB(&applMgr(),uiODApplMgr,doWellLogTools), "well_props" ) );
    if (  SI().zIsTime() )
	analwellmnu_->insertItem( new uiAction( "&Tie Well to Seismic ...",
	mCB(&applMgr(),uiODApplMgr,tieWellToSeismic), "well_tie" ) );
    analwellmnu_->insertItem( new uiAction( "&Rock Physics ...",
		mCB(&applMgr(),uiODApplMgr,launchRockPhysics), "rockphys" ) );
    analmnu_->insertItem( analwellmnu_ );

    layermodelmnu_ = new uiMenu(
			&appl_, "&Layer Modeling", "stratlayermodeling" );
    layermodelmnu_->insertItem( new uiAction( "&Basic ...",
	mCB(&applMgr(),uiODApplMgr,doLayerModeling), uiIcon::None() ) );
    analmnu_->insertItem( layermodelmnu_ );
    analmnu_->insertSeparator();
}


void uiODMenuMgr::fillSceneMenu()
{
    scenemnu_->clear();
    mInsertItem( scenemnu_, "&New", mAddSceneMnuItm );

    addtimedepthsceneitm_ = new uiAction( "Dummy",
					  mCB(this,uiODMenuMgr,handleClick) );
    scenemnu_->insertItem( addtimedepthsceneitm_, mAddTmeDepthMnuItm );

    create2D3DMnu( scenemnu_, "New [Horizon flattened]",
		   mAddHorFlat2DMnuItm, mAddHorFlat3DMnuItm, 0 );
    lastsceneitm_ = scenemnu_->insertSeparator();

    mInsertItem( scenemnu_, "&Cascade", mCascadeMnuItm );
    uiMenu* tileitm = new uiMenu( &appl_, "&Tile" );
    scenemnu_->insertItem( tileitm );

    mInsertItem( tileitm, "&Auto", mTileAutoMnuItm );
    mInsertItem( tileitm, "&Horizontal", mTileHorMnuItm );
    mInsertItem( tileitm, "&Vertical", mTileVerMnuItm );

    mInsertItem( scenemnu_, "&Properties ...", mScenePropMnuItm );
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
    BufferStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int idx=0; idx<=scenenms.size(); idx++ )
    {
	const int id = mSceneSelMnuItm + idx;
	uiAction* itm = scenemnu_->findAction( id );

	if ( idx >= scenenms.size() )
	{
	    if ( itm )
		scenemnu_->removeItem( id );
	    continue;
	}

	if ( !itm )
	{
	    itm = new uiAction( "", mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertItem( itm, id );
	    itm->setCheckable( true );
	}

	itm->setText( scenenms.get(idx) );
	itm->setChecked( idx==activescene );
    }

    BufferString itmtxt( "New [" );
    itmtxt += SI().zIsTime() ? "&Depth]" : "T&ime]";
    addtimedepthsceneitm_->setText( itmtxt );
}


void uiODMenuMgr::fillViewMenu()
{
    if ( !viewmnu_ ) return;

    viewmnu_->clear();
#ifdef __debug__
    mInsertItem( viewmnu_, "&Base Map ...", mBaseMapMnuItm );
#endif
    mInsertItem( viewmnu_, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu_, "&Z-scale ...", mZScaleMnuItm );
    uiMenu* stereoitm = new uiMenu( &appl_, "&Stereo viewing" );
    viewmnu_->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id) \
    itm = new uiAction( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, "&Off", true, mStereoOffMnuItm )
    mInsertStereoItem( stereoredcyanitm_, "&Red/Cyan", false,
			mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, "&Quad buffer", false,
			mStereoQuadMnuItm )

    stereooffsetitm_ = new uiAction( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.setName("&Toolbars");
    viewmnu_->insertItem( &toolbarsmnu );
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
	mInsertItem( iconsmnu, mnunm, mViewIconsMnuItm+nms.size() );
	nms.add( nm );
    }
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dlsett( GetSettingsDir(), DirList::DirsOnly, "icons.*" );
    DirList dlsite( mGetApplSetupDataDir(), DirList::DirsOnly, "icons.*" );
    DirList dlrel( mGetSWDirDataDir(), DirList::DirsOnly, "icons.*" );
    if ( dlsett.size() + dlsite.size() + dlrel.size() < 2 )
	return;

    uiMenu* iconsmnu = new uiMenu( &appl_, "&Icons" );
    viewmnu_->insertItem( iconsmnu );
    mInsertItem( iconsmnu, "&Default", mViewIconsMnuItm+0 );
    BufferStringSet nms; nms.add( "Default" );
    addIconMnuItems( dlsett, iconsmnu, nms );
    addIconMnuItems( dlsite, iconsmnu, nms );
    addIconMnuItems( dlrel, iconsmnu, nms );
}


static void updateTranslateMenu( uiMenu& mnu )
{
    for ( int idx=0; idx<mnu.actions().size(); idx++ )
    {
	uiAction* itm = const_cast<uiAction*>(mnu.actions()[idx]);
	itm->setChecked( idx==TrMgr().currentLanguage() );
    }
}


void uiODMenuMgr::fillUtilMenu()
{
    settmnu_ = new uiMenu( &appl_, "&Settings" );
    utilmnu_->insertItem( settmnu_ );
    if ( TrMgr().nrSupportedLanguages() > 1 )
    {
	langmnu_ = new uiMenu( &appl_, tr("Language") );
	settmnu_->insertItem( langmnu_ );
	for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
	{
	    uiAction* itm = new uiAction( TrMgr().getLanguageUserName(idx),
					  mCB(this,uiODMenuMgr,handleClick) );
	    itm->setCheckable( true );
	    langmnu_->insertItem( itm, mLanguageMnu+idx );
	}

	updateTranslateMenu( *langmnu_ );
    }

    mInsertItem( settmnu_, "&Look and feel ...", mSettLkNFlMnuItm );
    mInsertItem( settmnu_, "&Mouse controls ...", mSettMouseMnuItm );
    mInsertItem( settmnu_, "&Keyboard shortcuts ...", mSettShortcutsMnuItm );
    uiMenu* advmnu = new uiMenu( &appl_, "&Advanced" );
    mInsertItem( advmnu, "&Personal settings ...", mSettGeneral );
    mInsertItem( advmnu, "&Survey defaults ...", mSettSurvey );
    settmnu_->insertItem( advmnu );

    toolsmnu_ = new uiMenu( &appl_, "&Tools" );
    utilmnu_->insertItem( toolsmnu_ );

    mInsertItem( toolsmnu_, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( toolsmnu_, "&Position conversion ...", mPosconvMnuItm );
    mInsertItem( toolsmnu_, "&Create Plugin Devel. Env. ...", mCrDevEnvMnuItm );

    installmnu_ = new uiMenu( &appl_, "&Installation" );
    utilmnu_->insertItem( installmnu_ );
    FilePath installerdir( ODInst::GetInstallerDir() );
    const bool hasinstaller = File::isDirectory( installerdir.fullPath() );
    if ( hasinstaller && !__ismac__ )
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	const bool aitfixed = ODInst::autoInstTypeIsFixed();
	if ( !aitfixed || ait == ODInst::UseManager || ait == ODInst::FullAuto )
	    mInsertItem( installmnu_, "OpendTect Installation &Manager ...",
			 mInstMgrMnuItem );
	if ( !aitfixed )
	    mInsertItem( installmnu_, "&Auto-update policy ...",
			 mInstAutoUpdPolMnuItm );
	mInsertItem( installmnu_, "&Connection Settings ...",
		     mInstConnSettsMnuItm );
	installmnu_->insertSeparator();
    }

    mInsertItem( installmnu_, "&Plugins ...", mPluginsMnuItm );
    mInsertItem( installmnu_, "&Setup Batch Processing ...", mSetupBatchItm );

    const char* lmfnm = od_ostream::logStream().fileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, "Show &log file ...", mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	mInsertItem( toolsmnu_, "Data pack dump ...", mDumpDataPacksMnuItm );
	mInsertItem( toolsmnu_, "Display memory info ...",mDisplayMemoryMnuItm);
    }
}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(appman,uiODApplMgr,fn), togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey","Survey setup",false,manSurvCB);

    SurveyInfo::Pol2D survtype = SI().survDataType();
    if ( survtype == SurveyInfo::Only2D )
    {
	mAddTB(dtecttb_,"attributes","Edit attributes",false,editAttr2DCB);
	mAddTB(dtecttb_,"out_2dlines","Create seismic output",
	       false,seisOut2DCB);
    }
    else if ( survtype == SurveyInfo::No2D )
    {
	mAddTB( dtecttb_,"attributes","Edit attributes",false,editAttr3DCB);
	mAddTB( dtecttb_,"out_vol","Create seismic output",false,
		seisOut3DCB);
	mAddTB( dtecttb_,VolProc::uiChain::pixmapFileName(),
		"Volume Builder",false,doVolProcCB);
    }
    else
    {
	mAddTB(dtecttb_,"attributes_2d","Edit 2D attributes",false,
	       editAttr2DCB);
	mAddTB(dtecttb_,"attributes_3d","Edit 3D attributes",false,
	       editAttr3DCB);
	mAddTB(dtecttb_,"out_2dlines","Create 2D seismic output",false,
	       seisOut2DCB);
	mAddTB(dtecttb_,"out_vol","Create 3D seismic output",false,
	       seisOut3DCB);
	mAddTB( dtecttb_,VolProc::uiChain::pixmapFileName(),
		"Volume Builder",false,doVolProcCB);
    }
    mAddTB(dtecttb_,"xplot_wells","Cross-plot Attribute vs Well data",
	   false,doWellXPlot);
    mAddTB(dtecttb_,"xplot_attribs","Cross-plot Attribute vs Attribute data",
	   false,doAttribXPlot);

    mAddTB(dtecttb_,"rockphys","Create new well logs using Rock Physics",
			false,launchRockPhysics);

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(this,uiODMenuMgr,fn), togg )

#define mAddPopUp( nm, txt1, txt2, itm1, itm2, mnuid ) { \
    uiMenu* popmnu = new uiMenu( &appl_, nm ); \
    popmnu->insertItem( new uiAction(txt1, \
		       mCB(this,uiODMenuMgr,handleClick)), itm1 ); \
    popmnu->insertItem( new uiAction(txt2, \
		       mCB(this,uiODMenuMgr,handleClick)), itm2 ); \
    mantb_ ->setButtonMenu( mnuid, popmnu ); }

#define mAddPopupMnu( mnu, txt, itm ) \
    mnu->insertItem( new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), itm );

void uiODMenuMgr::fillManTB()
{
    const int seisid =
	mAddTB(mantb_,"man_seis","Manage Seismic data",false,manSeis);
    const int horid =
	mAddTB(mantb_,"man_hor","Manage Horizons",false,manHor);
    const int fltid =
	mAddTB(mantb_,"man_flt","Manage Faults",false,manFlt);
    mAddTB(mantb_,"man_wll","Manage Well data",false,manWll);
    mAddTB(mantb_,"man_picks","Manage PickSets/Polygons",false,manPick);
    mAddTB(mantb_,"man_body","Manage Bodies/Regions",false,manBody);
    mAddTB(mantb_,"man_wvlt","Manage Wavelets",false,manWvlt);
    mAddTB(mantb_,"man_strat","Manage Stratigraphy",false,manStrat);

    uiMenu* seispopmnu = new uiMenu( &appl_, "Seismics Menu" );
    if ( SI().has2D() )
    {
	mAddPopupMnu( seispopmnu, "2D Seismics", mManSeis2DMnuItm )
	mAddPopupMnu( seispopmnu, "2D Prestack Seismics", mManSeisPS2DMnuItm )
    }
    if ( SI().has3D() )
    {
	mAddPopupMnu( seispopmnu, "3D Seismics", mManSeis3DMnuItm )
	mAddPopupMnu( seispopmnu, "3D Prestack Seismics", mManSeisPS3DMnuItm )
    }
    mantb_->setButtonMenu( seisid, seispopmnu );

    if ( SI().survDataType() != SurveyInfo::No2D )
	mAddPopUp( "Horizon Menu", "2D Horizons", "3D Horizons",
		   mManHor2DMnuItm, mManHor3DMnuItm, horid );

    mAddPopUp( "Fault Menu", "Faults", "FaultStickSets",
	       mManFaultMnuItm, mManFaultStickMnuItm, fltid );
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiODMenuMgr,fn), fnm ); \
    mnu->insertItem( itm, idx ); }

void uiODMenuMgr::fillCoinTB( uiODSceneMgr* scenemgr )
{
    actviewid_ = cointb_->addButton( "altpick", "Switch to view mode",
			mCB(this,uiODMenuMgr,toggViewMode), false );
    mAddTB(cointb_,"home","To home position",false,toHomePos);
    mAddTB(cointb_,"set_home","Save home position",false,saveHomePos);
    mAddTB(cointb_,"view_all","View all",false,viewAll);
    cameraid_ = mAddTB(cointb_,"perspective",
		       "Switch to orthographic camera",false,switchCameraType);

    curviewmode_ = ui3DViewer::Inl;
    bool separateviewbuttons = false;
    Settings::common().getYN( "dTect.SeparateViewButtons", separateviewbuttons);
    if ( !separateviewbuttons )
    {
	viewselectid_ = cointb_->addButton( "cube_inl","View In-line",
				mCB(this,uiODMenuMgr,handleViewClick), false );

	uiMenu* vwmnu = new uiMenu( &appl_, "View Menu" );
	mAddMnuItm( vwmnu, "View In-line", handleViewClick, "cube_inl", 0 );
	mAddMnuItm( vwmnu, "View Cross-line", handleViewClick, "cube_crl", 1 );
	mAddMnuItm( vwmnu, "View Z", handleViewClick, "cube_z", 2 );
	mAddMnuItm( vwmnu, "View North", handleViewClick, "view_N", 3 );
	mAddMnuItm( vwmnu, "View North - Z", handleViewClick, "view_NZ", 4 );
	cointb_->setButtonMenu( viewselectid_, vwmnu );
	viewinlid_ = viewcrlid_ = viewzid_ = viewnid_ = viewnzid_ = -1;
    }
    else
    {
#define mAddVB(img,txt) cointb_->addButton( img, txt, \
			mCB(this,uiODMenuMgr,handleViewClick), false );
	viewinlid_ = mAddVB( "cube_inl", "View In-line" );
	viewcrlid_ = mAddVB( "cube_crl", "View Cross-line" );
	viewzid_ = mAddVB( "cube_z", "View Z" );
	viewnid_ = mAddVB( "view_N", "View North" );
	viewnzid_ = mAddVB( "view_NZ", "View North Z" );
	viewselectid_ = -1;
    }

    mAddTB( cointb_, "dir-light", "Set directional light", false,
	    doDirectionalLight);

    axisid_ = mAddTB(cointb_,"axis","Display orientation axis",
		     true,showRotAxis);
    coltabid_ = cointb_->addButton( "colorbar", "Display color bar",
			    mCB(this,uiODMenuMgr,dispColorBar), true );
    uiMenu* colbarmnu = new uiMenu( &appl_, "ColorBar Menu" );
    mAddMnuItm( colbarmnu, "Settings ...", dispColorBar, "disppars", 0 );
    cointb_->setButtonMenu( coltabid_, colbarmnu );

    mAddTB(cointb_,"snapshot","Take snapshot",false,mkSnapshot);
    polyselectid_ = cointb_->addButton( "polygonselect",
	"Polygon Selection mode", mCB(this,uiODMenuMgr,selectionMode), true );
    uiMenu* mnu = new uiMenu( &appl_, "Menu" );
    mAddMnuItm( mnu, "Polygon", handleToolClick, "polygonselect", 0 );
    mAddMnuItm( mnu, "Rectangle", handleToolClick, "rectangleselect", 1 );
    cointb_->setButtonMenu( polyselectid_, mnu );

    removeselectionid_ = cointb_->addButton( "trashcan", "Remove selection",
		    mCB(this,uiODMenuMgr,removeSelection), false );

    soloid_ = mAddTB(cointb_,"solo","Display current element only",
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
    BufferString tt( "View In-line" );
    curviewmode_ = ui3DViewer::Inl;
    switch( itmid )
    {
	case 1: pm = "cube_crl"; tt = "View Cross-line";
		curviewmode_ = ui3DViewer::Crl; break;
	case 2: pm = "cube_z"; tt = "View Z";
		curviewmode_ = ui3DViewer::Z; break;
	case 3: pm = "view_N"; tt = "View North";
		curviewmode_ = ui3DViewer::Y; break;
	case 4: pm = "view_NZ"; tt = "View North Z";
		curviewmode_ = ui3DViewer::YZ; break;
    }

    cointb_->setIcon( viewselectid_, pm );
    cointb_->setToolTip( viewselectid_, tt );
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
	cointb_->turnOn( polyselectid_, visserv->isSelectionModeOn() );
	sIsPolySelect = visserv->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn(
	    !inviewmode_  && cointb_->isOn(polyselectid_) );
	visserv->setSelectionMode( mode );
    }

    cointb_->setIcon( polyselectid_, sIsPolySelect ?
			"polygonselect" : "rectangleselect" );
    cointb_->setToolTip( polyselectid_, sIsPolySelect ?
			"Polygon Selection mode" : "Rectangle Selection mode" );
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

    visserv.displaySceneColorbar( cointb_->isOn(coltabid_) );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    cointb_->setToolTip(cameraid_,perspective ? "Switch to orthographic camera"
					      : "Switch to perspective camera");
    cointb_->setIcon( cameraid_, perspective ? "perspective"
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
    case mManSurveyMnuItm:		applMgr().manageSurvey(); break;
    case mSessSaveMnuItm:		appl_.saveSession(); break;
    case mSessRestMnuItm:		appl_.restoreSession(); break;
    case mSessAutoMnuItm:		appl_.autoSession(); break;
    case mImpAttrMnuItm:		mDoOp(Imp,Attr,0); break;
    case mImpAttrOthSurvMnuItm:		mDoOp(Imp,Attr,1); break;
    case mImpColTabMnuItm:		mDoOp(Imp,ColTab,0); break;
    case mImpSeisCBVSMnuItm:		mDoOp(Imp,Seis,0); break;
    case mImpSeisSEGYMnuItm:		mDoOp(Imp,Seis,1); break;
    case mImpSeisSEGYDirectMnuItm:	mDoOp(Imp,Seis,2); break;
    case mImpSeisSimple3DMnuItm:	mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm:	mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm:	mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm:	mDoOp(Imp,Seis,8); break;
    case mImpSeisCBVSOtherSurvMnuItm:	mDoOp(Imp,Seis,9); break;
    case mExpSeisSEGY3DMnuItm:		mDoOp(Exp,Seis,1); break;
    case mExpSeisSEGY2DMnuItm:		mDoOp(Exp,Seis,2); break;
    case mExpSeisSEGYPS3DMnuItm:	mDoOp(Exp,Seis,3); break;
    case mExpSeisSEGYPS2DMnuItm:	mDoOp(Exp,Seis,4); break;
    case mExpSeisSimple3DMnuItm:	mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm:	mDoOp(Exp,Seis,6); break;
    case mExpSeisSimplePS3DMnuItm:	mDoOp(Exp,Seis,7); break;
    case mExpSeisSimplePS2DMnuItm:	mDoOp(Exp,Seis,8); break;
    case mImpHorAsciiMnuItm:		mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm:	mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm:		mDoOp(Imp,Hor,2); break;
    case mImpBulkHorAsciiMnuIm:		mDoOp(Imp,Hor,3); break;
    case mExpHorAscii3DMnuItm:		mDoOp(Exp,Hor,0); break;
    case mExpHorAscii2DMnuItm:		mDoOp(Exp,Hor,1); break;
    case mExpFltAsciiMnuItm:		mDoOp(Exp,Flt,0); break;
    case mExpFltSSAsciiMnuItm:		mDoOp(Exp,Flt,1); break;
    case mImpWellAsciiTrackMnuItm:	mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:	mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:	mDoOp(Imp,Wll,2); break;
    case mImpWellSEGYVSPMnuItm:		mDoOp(Imp,Wll,3); break;
    case mImpWellSimpleMnuItm:		mDoOp(Imp,Wll,4); break;
    case mImpBulkWellTrackItm:		mDoOp(Imp,Wll,5); break;
    case mImpBulkWellLogsItm:		mDoOp(Imp,Wll,6); break;
    case mImpBulkWellMarkersItm:	mDoOp(Imp,Wll,7); break;
    case mImpBulkWellD2TItm:		mDoOp(Imp,Wll,8); break;
    case mImpPickAsciiMnuItm:		mDoOp(Imp,Pick,0); break;
    case mExpPickAsciiMnuItm:		mDoOp(Exp,Pick,0); break;
    case mExpWvltAsciiMnuItm:		mDoOp(Exp,Wvlt,0); break;
    case mImpWvltAsciiMnuItm:		mDoOp(Imp,Wvlt,0); break;
    case mImpFaultMnuItm:		mDoOp(Imp,Flt,0); break;
    case mImpFaultSSAscii3DMnuItm:	mDoOp(Imp,Flt,1); break;
    case mImpFaultSSAscii2DMnuItm:	mDoOp(Imp,Flt,2); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpPVDSAsciiMnuItm:		mDoOp(Imp,PVDS,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
    case mManColTabMnuItm:		mDoOp(Man,ColTab,0); break;
    case mManSeis3DMnuItm:		mDoOp(Man,Seis,0); break;
    case mManSeis2DMnuItm:		mDoOp(Man,Seis,1); break;
    case mManSeisPS3DMnuItm:		mDoOp(Man,Seis,2); break;
    case mManSeisPS2DMnuItm:		mDoOp(Man,Seis,3); break;
    case mManHor3DMnuItm:		mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm:		mDoOp(Man,Hor,1); break;
    case mManFaultStickMnuItm:		mDoOp(Man,Flt,1); break;
    case mManFaultMnuItm:		mDoOp(Man,Flt,2); break;
    case mManBodyMnuItm:		mDoOp(Man,Body,0); break;
    case mManPropsMnuItm:		mDoOp(Man,Props,0); break;
    case mManWellMnuItm:		mDoOp(Man,Wll,0); break;
    case mManPickMnuItm:		mDoOp(Man,Pick,0); break;
    case mManWvltMnuItm:		mDoOp(Man,Wvlt,0); break;
    case mManAttrMnuItm:		mDoOp(Man,Attr,0); break;
    case mManNLAMnuItm:			mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:		mDoOp(Man,Sess,0); break;
    case mManStratMnuItm:		mDoOp(Man,Strat,0); break;
    case mManPDFMnuItm:			mDoOp(Man,PDF,0); break;
    case mManGeomItm:			mDoOp(Man,Geom,0); break;
    case mManCrossPlotItm:		mDoOp(Man,PVDS,0); break;

    case mPreLoadSeisMnuItm:	applMgr().manPreLoad(uiODApplMgr::Seis); break;
    case mPreLoadHorMnuItm:	applMgr().manPreLoad(uiODApplMgr::Hor); break;
    case mExitMnuItm:		appl_.exit(); break;
    case mEditAttrMnuItm:	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm:	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm:	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol(SI().has2D(),false); break;
    case mSeisOut2DMnuItm:	applMgr().createVol(true,false); break;
    case mSeisOut3DMnuItm:	applMgr().createVol(false,false); break;
    case mCreateSurf2DMnuItm:	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm:	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case m2DFrom3DMnuItem:	applMgr().create2Dfrom3D(); break;
    case m3DFrom2DMnuItem:	applMgr().create3Dfrom2D(); break;
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
    case mBaseMapMnuItm:	applMgr().showBaseMap(); break;
    case mWorkAreaMnuItm:	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm:		applMgr().setZStretch(); break;
    case mBatchProgMnuItm:	applMgr().batchProgs(); break;
    case mPluginsMnuItm:	applMgr().pluginMan(); break;
    case mSetupBatchItm:	applMgr().setupBatchHosts(); break;
    case mPosconvMnuItm:	applMgr().posConversion(); break;
    case mInstMgrMnuItem:	applMgr().startInstMgr(); break;
    case mInstAutoUpdPolMnuItm:	applMgr().setAutoUpdatePol(); break;
    case mCrDevEnvMnuItm:	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm:	showLogFile(); break;
    case mSettMouseMnuItm:	sceneMgr().setKeyBindings(); break;

    case mInstConnSettsMnuItm: {
	uiProxyDlg dlg( &appl_ ); dlg.go(); } break;

    case mSettLkNFlMnuItm: {
	uiSettingsDlg dlg( &appl_ );
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
	    od_ostream strm( dlg.fileName() );
	    if ( strm.isOK() )
		DataPackMgr::dumpDPMs( strm );
	}
    } break;
    case mDisplayMemoryMnuItm: {
	IOPar iopar;
	OD::dumpMemInfo( iopar );
	BufferString text;
	iopar.dumpPretty( text );
	uiDialog dlg( &appl_, uiDialog::Setup("Memory information", mNoDlgTitle,
					      mNoHelpKey ) );
	uiTextBrowser* browser = new uiTextBrowser( &dlg );
	browser->setText( text.buf() );
	dlg.setCancelText( 0 );
	dlg.go();
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set Personal Settings" );
	dlg.go();
    } break;
    case mSettSurvey: {
	uiSettings dlg( &appl_, "Set Survey default Settings",
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
	    const char* scenenm = itm->text().getFullString();
	    sceneMgr().setActiveScene( scenenm );
	    itm->setChecked( true );
	}

	if ( id>=mLanguageMnu && id<mLanguageMnu+499 )
	{
	    const int langidx = id - mLanguageMnu;
	    uiString errmsg;
	    if ( !TrMgr().setLanguage(langidx,errmsg) )
		uiMSG().error( errmsg );

	    updateTranslateMenu( *langmnu_ );
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


int uiODMenuMgr::ask2D3D( const char* txt, int res2d, int res3d, int rescncl )
{
    int res = rescncl;
    if ( !SI().has2D() )
	res = res3d;
    else if ( !SI().has3D() )
	res = res2d;
    else
    {
	const int msg = uiMSG().askGoOnAfter( txt, "Cancel", "2D", "3D" );
	res = msg == 2 ? rescncl : ( msg == 0 ? res2d : res3d );
    }

    return res;
}


void uiODMenuMgr::manHor( CallBacker* )
{
    const int opt = ask2D3D( "Manage 2D or 3D Horizons", 1, 2, 0 );
    if ( opt == 0 ) return;

    mDoOp(Man,Hor,opt);
}


void uiODMenuMgr::manSeis( CallBacker* )
{
    const int opt = ask2D3D( "Manage 2D or 3D Seismics", 1, 0, -1 );
    if ( opt == -1 ) return;

    mDoOp(Man,Seis,opt);
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


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb_->clear();
    mantb_->clear();
    fillDtectTB( &applMgr() );
    fillManTB();
}


void uiODMenuMgr::toggViewMode( CallBacker* cb )
{
    inviewmode_ = !inviewmode_;
    cointb_->setIcon( actviewid_, inviewmode_ ? "altview" : "altpick" );
    cointb_->setToolTip( actviewid_, inviewmode_
	    ? "Switch to interact mode" : "Switch to view mode" );
    if ( inviewmode_ )
	sceneMgr().viewMode( cb );
    else
	sceneMgr().actMode( cb );

    selectionMode( 0 );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    fillImportMenu();
    fillExportMenu();
    fillManMenu();

    fillSceneMenu();
    fillAnalMenu();
    fillProcMenu();
    dTectMnuChanged.trigger();
}
