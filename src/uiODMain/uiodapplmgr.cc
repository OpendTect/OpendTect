/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2002
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
#include "uiodapplmgraux.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"
#include "uiodtreeitem.h"

#include "uiattribcrossplot.h"
#include "uiattribpartserv.h"
#include "uiconvpos.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uimain.h"
#include "uimpepartserv.h"
#include "uisurvinfoed.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodemsurftreeitem.h"
#include "uiodviewer2dposdlg.h"
#include "uiodviewer2dmgr.h"
#include "uipickpartserv.h"
#include "uiposprovider.h"
#include "uiseispartserv.h"
#include "uisip.h"
#include "uistereodlg.h"
#include "uistratlayermodel.h"
#include "uistrings.h"
#include "uisurveymanager.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uiveldesc.h"
#include "uivispartserv.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "uivolprocpartserv.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"
#include "visselman.h"
#include "visseis2ddisplay.h"
#include "vistexturechannels.h"

#include "attribdescset.h"
#include "bendpointfinder.h"
#include "cubesubsel.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emmanager.h"
#include "emobject.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "externalattrib.h"
#include "genc.h"
#include "dbman.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "oddirs.h"
#include "odsession.h"
#include "pickset.h"
#include "posinfo2d.h"
#include "posvecdataset.h"
#include "randomlinegeom.h"
#include "unitofmeasure.h"
#include "od_helpids.h"
#include "uiodstratlayermodelmgr.h"

uiODApplMgr::uiODApplMgr( uiODMain& a )
	: appl_(a)
	, applservice_(*new uiODApplService(&a,*this))
	, nlaserv_(0)
	, attribSetChg(this)
	, getOtherFormatData(this)
	, otherformatvisid_(-1)
	, otherformatattrib_(-1)
	, visdpsdispmgr_(0)
	, mousecursorexchange_( *new MouseCursorExchange )
	, dispatcher_(*new uiODApplMgrDispatcher(*this,&appl_))
	, attrvishandler_(*new uiODApplMgrAttrVisHandler(*this,&appl_))
{
    pickserv_ = new uiPickPartServer( applservice_ );
    visserv_ = new uiVisPartServer( applservice_ );
    visserv_->setMouseCursorExchange( &mousecursorexchange_ );
    attrserv_ = new uiAttribPartServer( applservice_ );
    volprocserv_ = new uiVolProcPartServer( applservice_ );

    seisserv_ = new uiSeisPartServer( applservice_ );
    emserv_ = new uiEMPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );
    wellserv_ = new uiWellPartServer( applservice_ );
    wellattrserv_ = new uiWellAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );

    visdpsdispmgr_ = new uiVisDataPointSetDisplayMgr( *visserv_ );
    visdpsdispmgr_->treeToBeAdded.notify( mCB(this,uiODApplMgr,addVisDPSChild));
    wellattrserv_->setDPSDispMgr( visdpsdispmgr_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );

    if ( survChgReqAttrUpdate() )
    {
	attrserv_->setAttrsNeedUpdt();
	tmpprevsurvinfo_.refresh();
    }

    appl_.afterPopup.notify( mCB(this,uiODApplMgr,mainWinUpCB) );

    DBM().surveyChangeOK.notify(
			mCB(this,uiODApplMgr,surveyChangeOKCB),true );
    DBM().surveyToBeChanged.notify(
			mCB(this,uiODApplMgr,surveyToBeChanged),true );
    DBM().surveyChanged.notify( mCB(this,uiODApplMgr,surveyChanged) );
}


uiODApplMgr::~uiODApplMgr()
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true); attrvishandler_.survChg(true);
    DBM().surveyChangeOK.remove( mCB(this,uiODApplMgr,surveyChangeOKCB) );
    DBM().surveyToBeChanged.remove( mCB(this,uiODApplMgr,surveyToBeChanged) );
    delete mpeserv_;
    delete pickserv_;
    delete nlaserv_;
    delete attrserv_;
    delete volprocserv_;
    delete seisserv_;
    delete visserv_;

    delete emserv_;
    delete emattrserv_;
    delete wellserv_;
    delete wellattrserv_;
    delete &applservice_;
    delete &dispatcher_;
    delete visdpsdispmgr_;

    delete &attrvishandler_;
    delete &mousecursorexchange_;
}


MouseCursorExchange& uiODApplMgr::mouseCursorExchange()
{ return mousecursorexchange_; }


void uiODApplMgr::mainWinUpCB( CallBacker* cb )
{
    handleSurveySelect();
}


void uiODApplMgr::resetServers()
{
    if ( nlaserv_ ) nlaserv_->reset();

    delete attrserv_;
    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );

    delete volprocserv_;
    volprocserv_ = new uiVolProcPartServer( applservice_ );

    delete mpeserv_;
    mpeserv_ = new uiMPEPartServer( applservice_ );

    delete emattrserv_;
    emattrserv_ = new uiEMAttribPartServer( applservice_ );

    visserv_->deleteAllObjects();
    emserv_->removeUndo();
}


void uiODApplMgr::setNlaServer( uiNLAPartServer* s )
{
    nlaserv_ = s;
    if ( nlaserv_ )
	nlaserv_->setDPSDispMgr( visdpsdispmgr_ );
}


extern int OD_Get_2D_Data_Conversion_Status();
extern void OD_Convert_2DLineSets_To_2DDataSets(uiString& errmsg,TaskRunner*);

bool uiODApplMgr::Convert_OD4_Data_To_OD5()
{
    /*
    const int status = OD_Get_2D_Data_Conversion_Status();
    if ( !status )
	return true;

    if ( status == 3 ) // Pre 4.2 surveys
    {
	uiString msg( tr("The survey %1 appears to be too old. "
		"Please open this survey first in OpendTect 4.6 to update "
		"its database before using it in newer versions of OpendTect.")
		.arg(DBM().surveyName()) );
	if ( gUiMsg().askGoOn(msg,tr("Select another survey"),
			     uiStrings::phrExitOD() ) )
	    return false;

	uiMain::theMain().exit();
    }

    if ( status == 1 )
    {
	uiString msg( tr(
	    "The 2D seismic database of survey '%1' is still '4.6' or lower. "
	    "It will now be converted. "
	    "This may take some time depending on the amount of data. "
	    "Note that after the conversion you will still be able to use "
	    "this 2D data in older versions of OpendTect.")
	    .arg(DBM().surveyName()) );

	const int res = gUiMsg().question( msg, tr("Convert now"),
					  tr("Select another survey"),
					  uiStrings::phrExitOD() );
	if ( res < 0 )
	    uiMain::theMain().exit();

	if ( !res )
	{
	    gUiMsg().message( tr("Please note that you can copy the survey "
				"using 'Copy Survey' tool in the "
				"'Survey Setup and Selection' window.") );

	    return false;
	}
    }

    uiString errmsg;
    if ( !Survey::GMAdmin().fetchFrom2DGeom(errmsg) )
    {
	gUiMsg().error( errmsg );
	return false;
    }

    uiTaskRunner taskrnr( ODMainWin(), false );
    OD_Convert_2DLineSets_To_2DDataSets( errmsg, &taskrnr );
    if ( !errmsg.isEmpty() )
    {
	gUiMsg().error( errmsg );
	return false;
    }

    */
    return true;
}


extern bool OD_Get_Body_Conversion_Status();
extern bool OD_Convert_Body_To_OD5( uiString& errmsg );

bool uiODApplMgr::Convert_OD4_Body_To_OD5()
{
    const bool status = OD_Get_Body_Conversion_Status();
    if ( !status )
	return true;

    /*

    uiString msg( tr("OpendTect has a new geo-body format. "
		"All the old geo-bodies of survey '%1' will now be converted. "
		"Note that after the conversion, you will still be able to use "
		"those geo-bodies in OpendTect 4.6.0, but only in patch p or "
		"later.").arg(DBM().surveyName()) );

    const int res = gUiMsg().question( msg, tr("Convert now"),
					   tr("Do it later"),
					   uiStrings::phrExitOD() );
    if ( res < 0 )
	uiMain::theMain().exit();

    if ( !res )
    {
	gUiMsg().message( tr("Please note that you will not be able to use "
			    "any of the old geo-bodies in this survey.") );
	return false;
    }

    uiString errmsg;
    if ( !OD_Convert_Body_To_OD5(errmsg) )
	{ gUiMsg().error( errmsg ); return false; }

    gUiMsg().message( tr("All the geo-bodies have been converted.") );
    */
    return true;
}


bool uiODApplMgr::selectSurvey( uiParent* p )
{
    return manSurv( p );
}


void uiODApplMgr::handleSIPImport()
{
    IOPar iop;
    SI().getFreshSetupData( iop );
    BufferString sipnm;
    iop.get( uiSurvInfoProvider::sKeySIPName(), sipnm );
    if ( sipnm.isEmpty() )
	return;

    uiSurvInfoProvider* sip = uiSurvInfoProvider::getByName( sipnm );
    if ( !sip )
	{ pErrMsg("Cannot find SIP"); return; }

    const uiString askq = sip->importAskQuestion();
    if ( askq.isEmpty() )
	return;

    if ( gUiMsg().askGoOn(askq) )
	sip->startImport( ODMainWin(), iop );
}


void uiODApplMgr::handleSurveySelect()
{
    if ( SI().isFresh() )
    {
	setZStretch();
	handleSIPImport();
	SI().setNotFresh();
    }
    else
    {
	while ( !Convert_OD4_Data_To_OD5() )
	{
	    if ( !manSurv(0) )
		uiMain::theMain().exit();
	}
	Convert_OD4_Body_To_OD5();
    }
}


bool uiODApplMgr::manSurv( uiParent* p )
{
    uiSurveyManagerDlg dlg( p ? p : ODMainWin(), false );
    const bool rv = dlg.go();
    if ( rv )
	handleSurveySelect();
    return rv;
}


void uiODApplMgr::addVisDPSChild( CallBacker* cb )
{
    mCBCapsuleUnpack( DBKey, emid, cb );
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    sceneMgr().addEMItem( emid, sceneids[0] );
}


void uiODApplMgr::surveyChangeOKCB( CallBacker* )
{
    bool anythingasked = false;
    if ( !appl_.askStore(anythingasked,tr("Survey change")) )
	DBM().setSurveyChangeUserAbort();
}


void uiODApplMgr::surveyToBeChanged( CallBacker* )
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true); attrvishandler_.survChg(true);

    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; attrserv_ = 0;
    delete emattrserv_; emattrserv_ = 0;
    delete volprocserv_; volprocserv_ = 0;
    delete mpeserv_; mpeserv_ = 0;
    if ( appl_.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );
}


void uiODApplMgr::surveyChanged( CallBacker* )
{
    dispatcher_.survChg(false); attrvishandler_.survChg(false);
    bool douse = false;
    DBKey id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id.isInvalid() )
	sceneMgr().addScene( true );

    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );
    if ( survChgReqAttrUpdate() )
    {
	attrserv_->setAttrsNeedUpdt();
	tmpprevsurvinfo_.refresh();
    }

    volprocserv_ = new uiVolProcPartServer( applservice_ );

    mpeserv_ = new uiMPEPartServer( applservice_ );
    MPE::engine().init();

    wellserv_ = new uiWellPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );
}


bool uiODApplMgr::survChgReqAttrUpdate()
{
    if ( DBM().isBad() )
	return true;

    return !( SI().xyUnit() == tmpprevsurvinfo_.xyunit_ &&
		SI().zUnit() == tmpprevsurvinfo_.zunit_ &&
		mIsEqual( SI().zStep(),tmpprevsurvinfo_.zstep_, 1e-6 ) );
}


void uiODApplMgr::doOperation( ObjType ot, ActType at, int opt )
{
    dispatcher_.doOperation( (int)ot, (int)at, opt );
}


void uiODApplMgr::manPreLoad( ObjType ot )
{
    dispatcher_.manPreLoad( (int)ot );
}


void uiODApplMgr::enableMenusAndToolBars( bool yn )
{
    sceneMgr().disabRightClick( !yn );
    visServer()->disabMenus( !yn );
    visServer()->disabToolBars( !yn );
    if ( appl_.menuMgrAvailable() )
    {
	appl_.menuMgr().dtectTB()->setSensitive( yn );
	appl_.menuMgr().manTB()->setSensitive( yn );
	appl_.menuMgr().enableMenuBar( yn );
    }
}


void uiODApplMgr::enableTree( bool yn )
{
    sceneMgr().disabTrees( !yn );
    visServer()->blockMouseSelection( !yn );
}


void uiODApplMgr::enableSceneManipulation( bool yn )
{
    if ( !yn ) sceneMgr().setToViewMode();
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().enableActButton( yn );
}


void uiODApplMgr::editAttribSet()
{ editAttribSet( SI().has2D() ); }

void uiODApplMgr::editAttribSet( bool is2d )
{ attrserv_->editSet( is2d ); }

void uiODApplMgr::processTime2Depth( bool is2d )
{ seisserv_->processTime2Depth( is2d ); }

void uiODApplMgr::processVelConv( CallBacker* )
{ seisserv_->processVelConv(); }

void uiODApplMgr::createMultiCubeDS( CallBacker* )
{ seisserv_->createMultiCubeDataStore(); }

void uiODApplMgr::createMultiAttribVol( CallBacker* )
{ attrserv_->outputVol( DBKey::getInvalid(), false, true ); }

void uiODApplMgr::setStereoOffset()
{
    ObjectSet<ui3DViewer> vwrs;
    sceneMgr().get3DViewers( vwrs );
    uiStereoDlg dlg( &appl_, vwrs );
    dlg.go();
}


void uiODApplMgr::addTimeDepthScene( bool is2d )
{
    uiZAxisTransformSel* uitrans = SI().zIsTime()
	? new uiZAxisTransformSel( 0, false, ZDomain::sKeyTime(),
				  ZDomain::sKeyDepth(), true, false, is2d )
	: new uiZAxisTransformSel( 0, false, ZDomain::sKeyDepth(),
				  ZDomain::sKeyTime(), true, false, is2d );

    if ( !uitrans->isOK() )
    {
	gUiMsg().error(tr("No suitable transforms found"));
	return;
    }

    uiSingleGroupDlg<> dlg( &appl_, uitrans);
    dlg.setCaption( tr("Velocity model") );
    dlg.setTitleText( tr("Select velocity model to base scene on") );
    dlg.setHelpKey( mODHelpKey(mODApplMgraddTimeDepthSceneHelpID) );

    if ( !dlg.go() ) return;

    RefMan<ZAxisTransform> ztrans = uitrans->getSelection();
    if ( !ztrans )
	return;

    StepInterval<float> zsampling;
    if ( !uitrans->getTargetSampling( zsampling ) )
    {
	pErrMsg( "Cannot get sampling.");
	return;
    }

    const uiString snm = tr("%1 (using '%2')")
	    .arg( SI().zIsTime() ? sKey::Depth() : sKey::Time() )
	    .arg( ztrans->factoryDisplayName() );

    sceneMgr().tile();
    const int sceneid = sceneMgr().addScene( true, ztrans, snm );
    if ( sceneid!=-1 )
    {
	const float zscale = ztrans->zScale();
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneid) );
	TrcKeyZSampling cs( OD::UsrWork );
	cs.zsamp_ = zsampling;
	scene->setTrcKeyZSampling( cs );
	scene->setZScale( zscale );
    }
}


void uiODApplMgr::addHorFlatScene( bool is2d )
{
    RefMan<ZAxisTransform> transform = emserv_->getHorizonZAxisTransform(is2d);
    if ( !transform ) return;

    const DBKey hormid = DBKey( transform->fromZDomainInfo().getID() );
    PtrMan<IOObj> ioobj = hormid.getIOObj();
    const BufferString hornm = ioobj ? BufferString(ioobj->name())
				: transform->factoryDisplayName().getString();
    uiString scenenm = tr("Flattened on '%1'").arg( hornm );
    sceneMgr().tile();
    sceneMgr().addScene( true, transform, scenenm );
}


void uiODApplMgr::show2DViewer()
{
    uiODViewer2DPosDlg viewposdlg( appl_ );
    viewposdlg.go();
}


void uiODApplMgr::setWorkingArea()
{
    uiPosProvider::Setup su( false, false, true );
    su.useworkarea(false);
    uiPosProvDlg dlg( &appl_, su, tr("Set Work Area") );
    TrcKeyZSampling tkzs( OD::UsrWork );
    dlg.setSampling( tkzs );
    dlg.setHelpKey( mODHelpKey(mWorkAreaDlgHelpID) );
    if ( !dlg.go() )
	return;

    dlg.getSampling( tkzs );
    SI().setWorkRanges( tkzs );
    sceneMgr().viewAll(0);
}


void uiODApplMgr::selectWells( DBKeySet& wellids )
{ wellserv_->selectWells( wellids ); }

bool uiODApplMgr::storePickSets( int polyopt, const char* cat )
{ return pickserv_->storePickSets(polyopt,cat); }

bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv_->storePickSet( ps ); }

bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv_->storePickSetAs( ps ); }

bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{
    const int sceneid = sceneMgr().askSelectScene();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneid) );
    const float velocity =
	scene ? scene->getFixedZStretch() * scene->getZScale() : 0;
    return attrserv_->setPickSetDirs( ps, nlaserv_ ? nlaserv_->getModel() : 0,
				      velocity );
}


bool uiODApplMgr::getNewData( int visid, int attrib )
{
    if ( visid<0 ) return false;

    const Attrib::SelSpecList* as = visserv_->getSelSpecs( visid, attrib );
    if ( !as )
    {
	gUiMsg().error( tr("Cannot calculate attribute on this object") );
	return false;
    }

    Attrib::SelSpecList myas( *as );
    for ( int idx=0; idx<myas.size(); idx++ )
    {
	if ( myas[idx].id().isValid() )
	    attrserv_->updateSelSpec( myas[idx] );

	if ( !myas[idx].isUsable() )
	{
	    gUiMsg().error( tr("Cannot find selected attribute") );
	    return false;
	}
    }

    const DataPack::ID cacheid = visserv_->getDataPackID( visid, attrib );
    bool res = false;
    switch ( visserv_->getAttributeFormat(visid,attrib) )
    {
	case uiVisPartServer::Cube :
	{
	    TrcKeyZSampling cs = visserv_->getTrcKeyZSampling( visid, attrib );
	    if ( !cs.isDefined() )
		return false;

	    if ( myas[0].id() == Attrib::SelSpec::cExternalAttribID() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc =
		    Attrib::ExtAttribCalc::factory().createSuitable( myas[0] );
		if ( !calc )
		{
		    uiString errstr(tr("Selected attribute '%1'\nis not present"
				       " in the set and cannot be created")
				  .arg(myas[0].userRef()));
		    gUiMsg().error( errstr );
		    return false;
		}

		uiTaskRunner progm( &appl_ );
		RefMan<DataPack> dp =
		    calc->createAttrib( cs, cacheid, &progm );

		if ( !dp && !calc->errmsg_.isEmpty() )
		{
		    gUiMsg().error( calc->errmsg_ );
		    return false;
		}

		const DataPack::ID newid = dp ? dp->id() : DataPack::cNoID();
		visserv_->setDataPackID( visid, attrib, newid );
		res = newid != DataPack::cNoID();
		break;
	    }

	    attrserv_->setTargetSelSpecs( myas );
	    const DataPack::ID newid = attrserv_->createOutput( cs, cacheid );
	    if ( newid == DataPack::cNoID() )
	    {
		// clearing texture and set back original selspec
		const bool isattribenabled =
				    visserv_->isAttribEnabled( visid, attrib );
		visserv_->setSelSpecs( visid, attrib, myas );
		visserv_->enableAttrib( visid, attrib, isattribenabled );
		return false;
	    }

	    visserv_->setDataPackID( visid, attrib, newid );
	    res = true;
	    break;
	}
	case uiVisPartServer::Traces :
	{
	    const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv_->getDataTraceBids( visid, bids );
	    attrserv_->setTargetSelSpecs( myas );
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
			    visserv_->getObject(visid) );
	    if ( myas[0].id() == Attrib::SelSpec::cExternalAttribID() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc =
		    Attrib::ExtAttribCalc::factory().createSuitable( myas[0] );
		// TODO implement
		break;
	    }

	    const DataPack::ID newid = attrserv_->createRdmTrcsOutput(
		    zrg, rdmtdisp->getRandomLineID() );
	    res = true;
	    if ( newid.isInvalid() )
		res = false;
	    visserv_->setDataPackID( visid, attrib, newid );
	    break;
	}
	case uiVisPartServer::RandomPos :
	{
	    res = calcRandomPosAttrib( visid, attrib );
	    break;
	}
	case uiVisPartServer::OtherFormat :
	{
	    otherformatvisid_ = visid;
	    otherformatattrib_ = attrib;
	    getOtherFormatData.trigger();
	    otherformatvisid_ = -1;
	    otherformatattrib_ = -1;
	    res = true;
	    break;
	}
	default :
	{
	    pErrMsg("Invalid format");
	    return false;
	}
    }

    if ( cacheid == DataPack::cNoID() )
	useDefColTab( visid, attrib );
    else
	updateColorTable( visid, attrib );

    return res;
}


bool uiODApplMgr::getDefaultDescID( Attrib::DescID& descid, bool is2d,
				    bool isstored ) const
{
    if ( !isstored )
	descid = attrServer()->getDefaultAttribID( is2d );
    else
    {
	const DBKey dbky = seisServer()->getDefaultDataID( is2d );
	if ( dbky.isInvalid() )
	    return false;
	descid = attrServer()->getStoredID( dbky, is2d );
    }
    return descid.isValid();
}


void uiODApplMgr::calcShiftAttribute( int attrib, const Attrib::SelSpec& as )
{
    uiTreeItem* parent = sceneMgr().findItem( visserv_->getEventObjId() );
    if ( !parent ) return;

    ObjectSet<DataPointSet> dpsset;
    emattrserv_->fillHorShiftDPS( dpsset, 0 );

    if ( mIsUdf(attrib) )
    {
	mDynamicCastGet(uiODEarthModelSurfaceTreeItem*,emitem,parent)
	if ( emitem )
	{
	    uiODAttribTreeItem* itm =
		new uiODEarthModelSurfaceDataTreeItem( emitem->emObjectID(), 0,
						       typeid(*parent).name() );
	    parent->addChild( itm, false );
	    attrib = visserv_->addAttrib( visserv_->getEventObjId() );
	    emattrserv_->setAttribIdx( attrib );
	}
    }

    attrserv_->setTargetSelSpec( as );
    attrserv_->createOutput( dpsset, 1 );

    TypeSet<DataPointSet::DataRow> drset;
    BufferStringSet nmset;
    mDeclareAndTryAlloc( DataPointSet*, dps,
	    DataPointSet( drset, nmset, false, true ) );
    if ( !dps )
	{ deepUnRef( dpsset ); return; }

    mDeclareAndTryAlloc(DataColDef*,siddef,DataColDef(emattrserv_->sidDef()));
    if ( !siddef )
	{ deepUnRef( dpsset ); return; }

    dps->dataSet().add( siddef );
    if ( !dps->bivSet().setNrVals( dpsset.size()+2 ) )
	{ deepUnRef( dpsset ); return; }

    mAllocVarLenArr( float, attribvals, dpsset.size()+2 );
    if ( !mIsVarLenArrOK(attribvals) )
	{ deepUnRef( dpsset ); return; }

    attribvals[0] = 0.0; //depth

    BinnedValueSet::SPos bvspos;
    while ( dpsset[0]->bivSet().next(bvspos) )
    {
	const BinID binid = dpsset[0]->bivSet().getBinID( bvspos );
	for ( int idx=0; idx<dpsset.size(); idx++ )
	{
	    const float* vals = dpsset[idx]->bivSet().getVals( bvspos );
	    if ( !idx )
		attribvals[1] = vals[1]; //Sid

	    attribvals[idx+2] = vals[2]; //attrib
	}

	dps->bivSet().add( binid, attribvals );
    }

    dps->dataChanged();
    visServer()->setRandomPosData( visServer()->getEventObjId(),
				   attrib, dps );
    visserv_->setSelSpec( visserv_->getEventObjId(), attrib, as );
    visServer()->selectTexture( visServer()->getEventObjId(), attrib,
				emattrserv_->textureIdx() );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );

    deepUnRef( dpsset );
}


bool uiODApplMgr::calcRandomPosAttrib( int visid, int attrib )
{
    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	gUiMsg().error( tr("Cannot calculate attribute on this object") );
	return false;
    }
    else if ( as->id() == as->cNoAttribID()
	   || as->id() == as->cAttribNotSelID() )
	return false;

    Attrib::SelSpec myas( *as );
    DataPackMgr& dpm = DPM(DataPackMgr::PointID());
    if ( myas.id() == Attrib::SelSpec::cOtherAttribID() )
    {
	const DBKey surfid = visserv_->getDBKey(visid);
	const int auxdatanr = emserv_->loadAuxData( surfid, myas.userRef() );
	if ( auxdatanr>=0 )
	{
	    RefMan<DataPointSet> dps = new DataPointSet( false, true );
	    dpm.add( dps );
	    TypeSet<float> shifts( 0 );
	    emserv_->getAuxData( surfid, auxdatanr, *dps, shifts[0] );
	    setRandomPosData( visid, attrib, *dps );
	    mDynamicCastGet(visSurvey::HorizonDisplay*,vishor,
			    visserv_->getObject(visid) )
	    vishor->setAttribShift( attrib, shifts );

	    BufferStringSet* userrefs = new BufferStringSet;
	    userrefs->add( "Section ID" );
	    userrefs->add( myas.userRef() );
	    vishor->setUserRefs( attrib, userrefs );
	}

	return auxdatanr>=0;
    }

    RefMan<DataPointSet> dps = new DataPointSet( false, true );
    dpm.add( dps );
    visserv_->getRandomPos( visid, *dps );
    const int firstcol = dps->nrCols();
    dps->dataSet().add( new DataColDef(myas.userRef()) );
    attrserv_->setTargetSelSpec( myas );
    if ( !attrserv_->createOutput(*dps,firstcol) )
	return false;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv_->getObject(visid))
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,visserv_->getObject(visid))
    if ( fd )
    {
	const DataPack::ID id = fd->addDataPack( *dps );
	fd->setDataPackID( attrib, id, 0 );
	fd->setRandomPosData( attrib, dps.ptr(), SilentTaskRunnerProvider() );
	if ( visServer()->getSelAttribNr() == attrib )
	    fd->useTexture( true, true ); // tree only, not at restore session
    }
    else
    {
	setRandomPosData( visid, attrib, *dps );
	if ( hd )
	{
	    TypeSet<float> shifts( 1,
		(float)visserv_->getTranslation(visid).z_ );
	    hd->setAttribShift( attrib, shifts );
	}
    }

    return true;
}


bool uiODApplMgr::evaluateAttribute( int visid, int attrib )
{
    uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
    if ( format == uiVisPartServer::Cube )
    {
	const TrcKeyZSampling cs = visserv_->getTrcKeyZSampling( visid );
	DataPack::ID packid  = attrserv_->createOutput( cs, DataPack::cNoID() );
	visserv_->setDataPackID( visid, attrib, packid );
    }
    else if ( format==uiVisPartServer::Traces )
    {
	const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	TypeSet<BinID> bids;
	visserv_->getDataTraceBids( visid, bids );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
			visserv_->getObject(visid) );
	const DataPack::ID dpid = attrserv_->createRdmTrcsOutput(
		zrg, rdmtdisp->getRandomLineID() );
	visserv_->setDataPackID( visid, attrib, dpid );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	RefMan<DataPointSet> data = new DataPointSet( false, true );
	visserv_->getRandomPos( visid, *data );
	attrserv_->createOutput( *data, data->nrCols() );
	visserv_->setRandomPosData( visid, attrib, data );
    }
    else
    {
	gUiMsg().error( tr("Cannot evaluate attributes on this object") );
	return false;
    }

    return true;
}


bool uiODApplMgr::evaluate2DAttribute( int visid, int attrib )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(visid))
    if ( !s2d ) return false;

    const DataPack::ID dpid = attrserv_->createOutput(
					s2d->getTrcKeyZSampling(false),
					DataPack::ID::get(0) );
    if ( dpid.isInvalid() )
	return false;

    s2d->setDataPackID( attrib, dpid, 0 );
    return true;
}


bool uiODApplMgr::handleEvent( const uiApplPartServer* aps, int evid )
{
    if ( !aps ) return true;

    if ( aps == pickserv_ )
	return handlePickServEv(evid);
    else if ( aps == visserv_ )
	return handleVisServEv(evid);
    else if ( aps == nlaserv_ )
	return handleNLAServEv(evid);
    else if ( aps == attrserv_ )
	return handleAttribServEv(evid);
    else if ( aps == volprocserv_ )
	return handleVolProcServEv(evid);
    else if ( aps == emserv_ )
	return handleEMServEv(evid);
    else if ( aps == emattrserv_ )
	return handleEMAttribServEv(evid);
    else if ( aps == wellserv_ )
	return handleWellServEv(evid);
    else if ( aps == wellattrserv_ )
	return handleWellAttribServEv(evid);
    else if ( aps == mpeserv_ )
	return handleMPEServEv(evid);

    return false;
}


void* uiODApplMgr::deliverObject( const uiApplPartServer* aps, int id )
{
    if ( aps == attrserv_ )
    {
	bool isnlamod2d = id == uiAttribPartServer::objNLAModel2D();
	bool isnlamod3d = id == uiAttribPartServer::objNLAModel3D();
	if ( isnlamod2d || isnlamod3d  )
	{
	    if ( !nlaserv_ )
		return 0;
	    nlaserv_->set2DEvent( isnlamod2d );
	    return const_cast<NLAModel*>( nlaserv_->getModel() );
	}
    }
    else
    {
	pErrMsg("deliverObject for unsupported part server");
    }

    return 0;
}


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const DBKey emid = mpeserv_->getEMObjectID(trackerid);
	const int sceneid = mpeserv_->getCurSceneID();
	const int sdid = sceneMgr().addEMItem( emid, sceneid );
	if ( sdid==-1 )
	    return false;
	const EM::Object* emobj = EM::MGR().getObject( emid );
	if ( EM::Horizon3D::typeStr()==emobj->getTypeStr() )
	    viewer2DMgr().addNewTrackingHorizon3D( emid );
	else
	    viewer2DMgr().addNewTrackingHorizon2D( emid );

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const DBKey emid = mpeserv_->getEMObjectID( trackerid );

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> hordisplayids;
	visserv_->findObject( typeid(visSurvey::HorizonDisplay),
			      hordisplayids );

	TypeSet<int> hor2ddisplayids;
	visserv_->findObject( typeid(visSurvey::Horizon2DDisplay),
			      hor2ddisplayids );

	hordisplayids.append( hor2ddisplayids );
	viewer2DMgr().removeHorizon3D( emid );
	viewer2DMgr().removeHorizon2D( emid );

	for ( int idx=0; idx<hordisplayids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv_->getObject(hordisplayids[idx]));
	    if ( emod && emod->getObjectID()==emid )
	    {
		for ( int idy=0; idy<sceneids.size(); idy++ )
		    visserv_->removeObject( hordisplayids[idx], sceneids[idy] );
		sceneMgr().removeTreeItem(hordisplayids[idx] );
	    }
	}

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid==uiMPEPartServer::evHorizonTracking() )
    {
	sceneMgr().updateItemToolbar( visserv_->getSelObjectId() );
    }
    else if ( evid == uiMPEPartServer::evStartSeedPick() )
    {
	//Turn off everything
	visserv_->turnSeedPickingOn( true );
	sceneMgr().setToViewMode( false );
    }
    else if ( evid==uiMPEPartServer::evEndSeedPick() )
    {
	visserv_->turnSeedPickingOn( false );
    }
    else if ( evid==uiMPEPartServer::evSetupClosed() )
    {
	visserv_->reportTrackingSetupActive( false );
    }
    else if ( evid==uiMPEPartServer::evSetupLaunched() )
    {
	visserv_->reportTrackingSetupActive( true );
    }
    else if ( evid==uiMPEPartServer::evGetAttribData() )
    {
    }
    else if ( evid==uiMPEPartServer::evInitFromSession() )
	visserv_->initMPEStuff();
    else if ( evid==uiMPEPartServer::evUpdateTrees() )
	sceneMgr().updateTrees();
    else if ( evid==uiMPEPartServer::evStoreEMObject() )
	storeEMObject();
    else
    {
	pErrMsg("Unknown event from mpeserv");
    }

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine() )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
    }
    else if ( evid == uiWellPartServer::evCleanPreview() )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    else if ( evid == uiWellPartServer::evDisplayWell() )
    {
	const int sceneid = sceneMgr().askSelectScene();
	if ( sceneid<0 ) return false;

	const DBKeySet& wellids = wellserv_->createdWellIDs();
	for ( int idx=0; idx<wellids.size(); idx++ )
	    sceneMgr().addWellItem( wellids.get(idx), sceneid );
    }

    return true;
}


bool uiODApplMgr::handleWellAttribServEv( int evid )
{
    if ( evid == uiWellAttribPartServer::evPreview2DFromWells() )
    {
	TypeSet<Coord> coords;
	if ( !wellattrserv_->getPrev2DFromWellCoords(coords) )
	    return false;
	setupRdmLinePreview( coords );
    }
    else if ( evid == uiWellAttribPartServer::evShow2DFromWells() )
    {
	Pos::GeomID wellto2dgeomid = wellattrserv_->new2DFromWellGeomID();
	if ( !wellto2dgeomid.isValid() )
	    return false;
	sceneMgr().add2DLineItem( wellto2dgeomid );
	sceneMgr().updateTrees();
    }
    else if ( evid == uiWellAttribPartServer::evCleanPreview() )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon() )
    {
	const int sceneid = sceneMgr().askSelectScene();
	if ( sceneid<0 ) return false;

	const DBKey emid = emserv_->selEMID();
	sceneMgr().addEMItem( emid, sceneid );
	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evRemoveTreeObject() )
    {
	const DBKey emid = emserv_->selEMID();

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> emdisplayids;

	TypeSet<int> hordisplayids;
	visserv_->findObject( typeid(visSurvey::HorizonDisplay),
			      hordisplayids );
	emdisplayids.append( hordisplayids );

	TypeSet<int> hor2ddisplayids;
	visserv_->findObject( typeid(visSurvey::Horizon2DDisplay),
			      hor2ddisplayids );
	emdisplayids.append( hor2ddisplayids );

	TypeSet<int> faultdisplayids;
	visserv_->findObject( typeid(visSurvey::FaultDisplay),
			      faultdisplayids );
	emdisplayids.append( faultdisplayids );

	TypeSet<int> faultstickdisplay;
	visserv_->findObject( typeid(visSurvey::FaultStickSetDisplay),
			      faultstickdisplay );
	emdisplayids.append( faultstickdisplay );

	for ( int idx=0; idx<emdisplayids.size(); idx++ )
	{
	    bool remove = false;
	    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( emod && emod->getObjectID()==emid )
		remove = true;

	    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( fd && fd->getEMObjectID()==emid )
		remove = true;

	    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fsd,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( fsd && fsd->getEMObjectID()==emid )
		remove = true;

	    if ( !remove ) continue;

	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( emdisplayids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(emdisplayids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else
    {
	pErrMsg("Unknown event from emserv");
    }

    return true;
}


bool uiODApplMgr::handleEMAttribServEv( int evid )
{
    const int visid = visserv_->getEventObjId();
    const int attribidx = emattrserv_->attribIdx();
    const int shiftvisid = emattrserv_->getShiftedObjectVisID();

    if ( evid == uiEMAttribPartServer::evCalcShiftAttribute() )
    {
	const Attrib::SelSpec as( emattrserv_->getAttribBaseNm(),
				  emattrserv_->attribID() );
	calcShiftAttribute( attribidx, as );
    }
    else if ( evid == uiEMAttribPartServer::evHorizonShift() )
    {
	const int textureidx = emattrserv_->textureIdx();
	visserv_->setTranslation( shiftvisid,
				    Coord3(0,0,emattrserv_->getShift()) );
	if ( !mIsUdf(attribidx) )
	    visserv_->selectTexture( shiftvisid, attribidx, textureidx );

	uiTreeItem* parent = sceneMgr().findItem( shiftvisid );
	if ( parent )
	    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( evid == uiEMAttribPartServer::evStoreShiftHorizons() )
    {
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, -1 );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	RefMan<DataPointSet> data = new DataPointSet( false, true );
	visserv_->getRandomPosCache( visid, attribidx, *data );
	if ( data->isEmpty() ) return false;

	const DBKey emid = visserv_->getDBKey( visid );
	const int nrvals = data->bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float shift = emattrserv_->shiftRange().atIndex(idx);
	    float usrshift = shift * SI().zDomain().userFactor();
	    if ( mIsZero(usrshift,1e-3) ) usrshift = 0.f;
	    BufferString shiftstr;
	    shiftstr.set( usrshift ).embed( '[', ']' );
	    BufferString nm( emattrserv_->getAttribBaseNm(), " ", shiftstr );
	    emserv_->setAuxData( emid, *data, nm, idx+2, shift );
	    BufferString dummy;
	    if ( !emserv_->storeAuxData( emid, dummy, false ) )
		return false;
	}
    }
    else if ( evid == uiEMAttribPartServer::evShiftDlgOpened() )
    {
	enableMenusAndToolBars( false );
    }
    else if ( evid==uiEMAttribPartServer::evShiftDlgClosedCancel() ||
	      evid==uiEMAttribPartServer::evShiftDlgClosedOK() )
    {
	enableMenusAndToolBars( true );

	const bool isok = evid==uiEMAttribPartServer::evShiftDlgClosedOK();
	const BoolTypeSet& enableattrib = emattrserv_->initialAttribStatus();
	const int textureidx = emattrserv_->textureIdx();

	if ( !isok || mIsUdf(textureidx) )
	{
	    uiTreeItem* parent = sceneMgr().findItem(visid);
	    if ( !mIsUdf(emattrserv_->attribIdx() ) )
	    {
		uiTreeItem* itm = parent->lastChild();
		while ( true )
		{
		    uiTreeItem* nxt = itm->siblingAbove();
		    if ( !nxt ) break;
		    itm = nxt;
		}

		parent->removeChild( itm );
		visserv_->removeAttrib( shiftvisid, emattrserv_->attribIdx() );
	    }
	}

	if ( !isok )
	{
	    visserv_->setTranslation( shiftvisid,
		    Coord3(0,0,emattrserv_->initialShift() ) );

	    for ( int idx=0; idx<enableattrib.size(); idx++ )
		visserv_->enableAttrib( shiftvisid, idx, enableattrib[idx] );

	    uiTreeItem* parent = sceneMgr().findItem( shiftvisid );
	    if ( parent )
		parent->updateColumnText( uiODSceneMgr::cNameColumn() );
	}
	else
	{
	    for ( int idx=0; idx<visserv_->getNrAttribs(visid); idx++ )
	    {
		if ( idx==attribidx )
		{
		    RefMan<DataPointSet> data = new DataPointSet( false, true );
		    visserv_->getRandomPosCache( shiftvisid, attribidx, *data );
		    if ( data->isEmpty() )
			continue;

		    const int sididx = data->dataSet().findColDef(
			    emattrserv_->sidDef(), PosVecDataSet::NameExact );

		    int texturenr = emattrserv_->textureIdx() + 1;
		    if ( sididx<=texturenr )
			texturenr++;

		    const int nrvals = data->bivSet().nrVals();
		    for ( int idy=nrvals-1; idy>0; idy-- )
		    {
			if ( idy!=texturenr && idy!=sididx )
			    data->bivSet().removeVal( idy );
		    }

		    visserv_->setRandomPosData( shiftvisid, attribidx, data );
		}
	    }
	}
    }
    else if ( evid == uiEMAttribPartServer::evDisplayEMObject() )
    {
	const int sceneid = sceneMgr().askSelectScene();
	if ( sceneid<0 ) return false;

	const DBKeySet& emobjids = emattrserv_->getEMObjIDs();
	for ( int idx=0; idx<emobjids.size(); idx++ )
	    sceneMgr().addEMItem( emobjids[idx], sceneid );

	sceneMgr().updateTrees();
    }
    else
    {
	pErrMsg("Unknown event from emattrserv");
    }

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetHorInfo3D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), false );
    }
    else if ( evid == uiPickPartServer::evGetHorInfo2D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), true );
    }
    else if ( evid == uiPickPartServer::evGetHorDef3D() )
    {
	const DBKeySet& horids = pickserv_->selHorIDs();
	for ( int idx=0; idx<horids.size(); idx++ )
	{
	    if ( !emserv_->isFullyLoaded(horids[idx]) )
		emserv_->loadSurface( horids[idx] );
	}

	emserv_->getSurfaceDef3D( horids, pickserv_->genDef(),
			       pickserv_->selTrcKeySampling() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef2D() )
	emserv_->getSurfaceDef2D( pickserv_->selHorIDs(),
				  pickserv_->selectLines(),
				  pickserv_->getPos2D(),
				  pickserv_->getGeomIDs2D(),
				  pickserv_->getHor2DZRgs() );
    else if ( evid == uiPickPartServer::evFillPickSet() )
	emserv_->fillPickSet( *pickserv_->pickSet(), pickserv_->horID() );
    else if ( evid == uiPickPartServer::evDisplayPickSet() )
    {
	sceneMgr().addPickSetItem( pickserv_->pickSetID() );
    }
    else
    {
	pErrMsg("Unknown event from pickserv");
    }

    return true;
}


#define mGetSelTracker( tracker ) \
    const int selobjvisid = visserv_->getSelObjectId(); \
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,\
				visserv_->getObject(selobjvisid));\
    const DBKey emid = emod ? emod->getObjectID() : DBKey::getInvalid(); \
    const int trackerid = mpeserv_->getTrackerID(emid); \
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

bool uiODApplMgr::handleVisServEv( int evid )
{
    const int visid = visserv_->getEventObjId();
    visserv_->unlockEvent();

    if ( evid == uiVisPartServer::evUpdateTree() )
	sceneMgr().updateTrees();
    else if ( evid == uiVisPartServer::evDeSelection()
	   || evid == uiVisPartServer::evSelection() )
	sceneMgr().updateSelectedTreeItem();
    else if ( evid == uiVisPartServer::evGetNewData() )
	return getNewData( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evInteraction() )
	sceneMgr().setItemInfo( visid );
    else if ( evid==uiVisPartServer::evPickingStatusChange() ||
	      evid == uiVisPartServer::evMouseMove() )
	sceneMgr().updateStatusBar();
    else if ( evid == uiVisPartServer::evViewModeChange() )
	sceneMgr().setToWorkMode( visserv_->getWorkMode() );
    else if ( evid == uiVisPartServer::evSelectAttrib() )
	return selectAttrib( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evViewAll() )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos() )
	sceneMgr().toHomePos(0);
    else if ( evid == uiVisPartServer::evShowMPEParentPath() )
    {
	mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
			visserv_->getObject(visid))
	if ( !hd || !hd->getScene() )
	    return false;

	const TrcKeyValue tkv = hd->getScene()->getMousePos();
	addMPEParentPath( visid, tkv.tk_ );
    }
    else if ( evid == uiVisPartServer::evShowMPESetupDlg() )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			visserv_->getObject(visserv_->getSelObjectId()));
	const DBKey emid = emod ? emod->getObjectID() : DBKey::getInvalid();
	mpeserv_->showSetupDlg( emid );
    }
    else if ( evid == uiVisPartServer::evShowSetupGroupOnTop() )
    {
	mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
			 visserv_->getObject(visserv_->getSelObjectId()) );
	if ( !emod )
	    return false;

	return mpeserv_->showSetupGroupOnTop( emod->getObjectID(),
					      visserv_->getTopSetupGroupName());
    }
    else if ( evid == uiVisPartServer::evDisableSelTracker() )
    {
	mGetSelTracker( tracker );
	if ( tracker )
	    tracker->enable( false );
    }
    else if ( evid == uiVisPartServer::evColorTableChange() )
	updateColorTable( visserv_->getEventObjId(),
			  visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evStoreEMObject() )
	storeEMObject( false );
    else if ( evid == uiVisPartServer::evStoreEMObjectAs() )
	storeEMObject( true );
    else if ( evid == uiVisPartServer::evKeyboardEvent() )
    {
    }
    else if ( evid == uiVisPartServer::evMouseEvent() )
    {
    }
    else
    {
	pErrMsg("Unknown event from visserv");
    }

    return true;
}


void uiODApplMgr::addMPEParentPath( int visid, const TrcKey& tk )
{
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv_->getObject(visid))
    if ( !hd ) return;

    mDynamicCastGet(EM::Horizon3D*,hor3d,
		    EM::MGR().getObject(hd->getObjectID()))
    if ( !hor3d )
	return;

    TypeSet<TrcKey> trcs; hor3d->getParents( tk, trcs );
    if ( trcs.isEmpty() ) return;

    BendPointFinderTrcKey bpf( trcs, 10 );
    if ( !bpf.execute() ) return;

    const TypeSet<int>& bends = bpf.bendPoints();
    RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
    BufferString rlnm = "Parents path";
    rlnm.add( " [" ).add( tk.lineNr() ).add( "," )
	.add( tk.trcNr() ).add( "]" );
    rl->setName( rlnm );
    Geometry::RLM().add( rl );
    for ( int idx=0; idx<bends.size(); idx++ )
	rl->addNode( trcs[bends[idx]].binID() );

    /*TODO use displaIn2DViewer with probe
    const int rlvisid =
	sceneMgr().addRandomLineItem( rl->ID(), hd->getSceneID() );
    use displaIn2DViewer with probe
      viewer2DMgr().displayIn2DViewer( rlvisid, 0, false );*/
    visserv_->setSelObjectId( visid );
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite() )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet& ads =
	    attrserv_->curDescSet( nlaserv_->is2DEvent() );
	NLAModel* mdl = const_cast<NLAModel*>( nlaserv_->getModel() );
	if ( mdl )
	{
	    IOPar& iopar = mdl->pars();
	    iopar.setEmpty();
	    const BufferStringSet inputs = mdl->design().inputs;
	    const Attrib::DescSet* cleanads = ads.optimizeClone( inputs );
	    (cleanads ? cleanads : &ads)->fillPar( iopar );
	    delete cleanads;
	}
    }
    else if ( evid == uiNLAPartServer::evPrepareRead() )
    {
	bool saved = attrserv_->setSaved(nlaserv_->is2DEvent());
	uiString msg = tr("Current attribute set is not saved.\nSave now?");
	if ( !saved && gUiMsg().askSave( msg ) )
	    attrserv_->saveSet(nlaserv_->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished() )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	const NLAModel* mdl = nlaserv_->getModel();
	IOPar dummy;
	attrserv_->replaceSet( mdl ? mdl->pars() : dummy,
			       nlaserv_->is2DEvent() );
	wellattrserv_->setNLAModel( mdl );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames() )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	const NLAModel* mdl = nlaserv_->getModel();
	BufferStringSet inpnms;
	if ( mdl )
	    inpnms = mdl->design().inputs;
	attrserv_->getPossibleOutputs( nlaserv_->is2DEvent(), inpnms );
	if ( inpnms.isEmpty() )
	    { gUiMsg().error( tr("No usable input") ); return false; }
	nlaserv_->inputNames() = inpnms;
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput() )
    {
	BufferStringSet linekeys;
	nlaserv_->getNeededStoredInputs( linekeys );
	for ( int idx=0; idx<linekeys.size(); idx++ )
	    attrserv_->addToDescSet( linekeys.get(idx), nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evGetData() )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	ObjectSet<DataPointSet> dpss;
	const bool dataextraction = nlaserv_->willDoExtraction();
	if ( !dataextraction )
	    dpss += new DataPointSet( nlaserv_->is2DEvent(), false );
	else
	{
	    nlaserv_->getDataPointSets( dpss );
	    if ( dpss.isEmpty() )
	    {
		  gUiMsg().error(tr("No matching well data found"));
		  return false;
	    }
	    bool allempty = true;
	    for ( int idx=0; idx<dpss.size(); idx++ )
		{ if ( !dpss[idx]->isEmpty() ) { allempty = false; break; } }
	    if ( allempty )
	    {
		    gUiMsg().error(tr("No valid data locations found"));
		    return false;
	    }
	    if ( !attrserv_->extractData(dpss) )
		{ deepUnRef(dpss); return true; }
	    IOPar dummy;
	    const NLAModel* mdl = nlaserv_->getModel();
	    if ( mdl )
	    {
		IOPar& iop = nlaserv_->storePars();
		attrserv_->curDescSet(nlaserv_->is2DEvent()).fillPar( iop );
		if ( iop.name().isEmpty() )
		    iop.setName( "Attributes" );
	    }
	}

	const uiString res = nlaserv_->prepareInputData( dpss );
	if ( res == uiNLAPartServer::sKeyUsrCancel() )
	    return true;
	else if ( !res.isEmpty() )
	    gUiMsg().warning( res );

	if ( !dataextraction ) // i.e. if we have just read a DataPointSet
	    attrserv_->replaceSet( dpss[0]->dataSet().pars(), dpss[0]->is2D() );

	deepUnRef(dpss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass() )
    {
	RefMan<DataPointSet> dps = nlaserv_->dps();
	RefMan<DataPointSet> mcpicks = new DataPointSet( dps->is2D() );
	for ( int irow=0; irow<dps->size(); irow++ )
	{
	    if ( dps->group(irow) == 3 )
		mcpicks->addRow( dps->dataRow(irow) );
	}
	mcpicks->dataChanged();
	pickserv_->setMisclassSet( *mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCr2DRandomSet() )
    {
	pickserv_->createRandom2DSet();
    }
    else
    {
	pErrMsg("Unknown event from nlaserv");
    }

    return true;
}


bool uiODApplMgr::handleAttribServEv( int evid )
{
    const int visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    if ( evid==uiAttribPartServer::evDirectShowAttr() )
    {
	Attrib::SelSpec as;
	attrserv_->getDirectShowAttrSpec( as );
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    gUiMsg().error( uiStrings::phrSelect(tr("an attribute"
			      " element in the tree")) );
	    return false;
	}

	visserv_->setSelSpec( visid, attrib, as );
	visserv_->setColTabMapper( visid, attrib, *new ColTab::Mapper );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet() )
    {
	attribSetChg.trigger();
    }
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed() )
    {
	enableMenusAndToolBars( true );
	enableSceneManipulation( true );
    }
    else if ( evid==uiAttribPartServer::evEvalAttrInit() )
    {
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
	const bool alloweval = !( format==uiVisPartServer::None );
	const bool allowstorage = format==uiVisPartServer::RandomPos;
	attrserv_->setEvaluateInfo( alloweval, allowstorage );
    }
    else if ( evid==uiAttribPartServer::evEvalCalcAttr() )
    {
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttribID() );
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    gUiMsg().error( uiStrings::phrSelect(tr("an attribute"
			      " element in the tree")) );
	    return false;
	}
	if ( !calcMultipleAttribs(as) )
	{
	    gUiMsg().error( tr("Cannot evaluate this attribute") );
	    return false;
	}

	const ColTab::Mapper& mpr = visserv_->getColTabMapper( visid, attrib );
	attrserv_->setEvalBackupColTabMapper( &mpr );

	RefMan<ColTab::Mapper> mympr = new ColTab::Mapper( mpr );
	mympr->setup().setFixedRange( mpr.getRange() );
	visserv_->setColTabMapper( visid, attrib, *mympr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalShowSlice() )
    {
	const int sliceidx = attrserv_->getSliceIdx();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	visserv_->selectTexture( visid, attrnr, sliceidx );

	updateColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices() )
    {
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	RefMan<DataPointSet> data = new DataPointSet( false, true );
	visserv_->getRandomPosCache( visid, attrnr, *data );
	if ( data->isEmpty() ) return false;

	const DBKey emid = visserv_->getDBKey( visid );
	const float shift = (float) visserv_->getTranslation(visid).z_;
	const Attrib::SelSpecList& specs = attrserv_->getTargetSelSpecs();
	const int nrvals = data->bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv_->setAuxData( emid, *data, specs[idx].userRef(),
				 idx+2, shift );
	    BufferString dummy;
	    emserv_->storeAuxData( emid, dummy, false );
	}
    }
    else if ( evid==uiAttribPartServer::evEvalRestore() )
    {
	if ( attrserv_->getEvalBackupColTabMapper() )
	    visserv_->setColTabMapper( visid, attrib,
		    *attrserv_->getEvalBackupColTabMapper() );

	Attrib::SelSpec* as = const_cast<Attrib::SelSpec*>(
		visserv_->getSelSpec(visid,attrib) );
	const Attrib::SelSpecList& tmpset = attrserv_->getTargetSelSpecs();
	const int sliceidx = attrserv_->getSliceIdx();
	if ( as && tmpset.validIdx(sliceidx) )
	{
	    // userref stored in objectref during evaluation process
	    BufferString usrref = as->objectRef();
	    *as = tmpset[sliceidx];
	    as->setUserRef( usrref );
	}

	sceneMgr().updateTrees();
    }
    else
    {
	pErrMsg("Unknown event from attrserv");
    }

    return true;
}


bool uiODApplMgr::handleVolProcServEv( int evid )
{
    return true;
}


bool uiODApplMgr::calcMultipleAttribs( Attrib::SelSpec& as )
{
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    const int visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    const Attrib::SelSpecList& tmpset = attrserv_->getTargetSelSpecs();
    BufferString savedusrref = tmpset.size() ? tmpset[0].objectRef() : "";
    as.setObjectRef( savedusrref );
    as.set2D( attrserv_->is2DEvent() );
    as.setUserRef( tmpset[0].userRef() );
    if ( tmpset.isEmpty() )
	visserv_->setSelSpec( visid, attrib, as );
    else
	visserv_->setSelSpecs( visid, attrib, tmpset );

    BufferStringSet* refs = new BufferStringSet();
    for ( int idx=0; idx<tmpset.size(); idx++ )
	refs->add( tmpset[idx].userRef() );
    visserv_->setUserRefs( visid, attrib, refs );
    visserv_->setColTabMapper( visid, attrib, *new ColTab::Mapper );
    return as.is2D() ? evaluate2DAttribute(visid,attrib)
		     : evaluateAttribute(visid,attrib);
}


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv_->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<int> plids;
    TypeSet<int> sceneids;
    visSurvey::PolyLineDisplay* pl = new visSurvey::PolyLineDisplay;
    pl->fillPolyLine( coords );
    mDynamicCastGet(visBase::DataObject*,doobj,pl);
    visserv_->getChildIds( -1, sceneids );

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv_->addObject( doobj, sceneids[idx], true );
	plids.addIfNew( doobj->id() );
    }

    wellserv_->setPreviewIds(plids);
}


void uiODApplMgr::cleanPreview()
{
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    TypeSet<int>& previds = wellserv_->getPreviewIds();
    if ( previds.size() == 0 ) return;
    for ( int idx=0; idx<sceneids.size(); idx++ )
	visserv_->removeObject( previds[0],sceneids[idx] );

    previds.erase();
}


void uiODApplMgr::storeEMObject( bool saveasreq )
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
				surface, visserv_->getObject(selectedids[0]) );
    if ( !surface ) return;

    const DBKey emid = surface->getObjectID();
    PtrMan<IOObj> ioobj = emid.getIOObj();
    const bool saveas = emid.isInvalid() || !ioobj || saveasreq;
    emserv_->storeObject( emid, saveas );

    TypeSet<int> ids;
    visserv_->findObject( emid, ids );

    for ( int idx=0; idx<ids.size(); idx++ )
	visserv_->setObjectName( ids[idx], emid.name() );

    mpeserv_->saveSetup( emid );
    sceneMgr().updateTrees();
}


void uiODApplMgr::manSurvCB( CallBacker* )
{
    manSurv( ODMainWin() );
}

void uiODApplMgr::tieWellToSeismic( CallBacker* )
{ wellattrserv_->createD2TModel(DBKey()); }
void uiODApplMgr::doWellLogTools( CallBacker* )
{ wellserv_->doLogTools(); }
void uiODApplMgr::launchRockPhysics( CallBacker* )
{ wellserv_->launchRockPhysics(); }
void uiODApplMgr::launch2DViewer( CallBacker* )
{ show2DViewer(); }
void uiODApplMgr::doLayerModeling( CallBacker* )
{ uiStratLayerModelManager::doBasicLayerModel(); }

void uiODApplMgr::doVolProc2DCB( CallBacker* )
{ volprocserv_->doVolProc( true, 0 ); }
void uiODApplMgr::doVolProc3DCB( CallBacker* )
{ volprocserv_->doVolProc( false, 0 ); }
void uiODApplMgr::doVolProc( const DBKey& mid )
{ volprocserv_->doVolProc( false, &mid ); }
void uiODApplMgr::createVolProcOutput( bool is2d )
{ volprocserv_->createVolProcOutput( is2d, 0 ); }

bool uiODApplMgr::editNLA( bool is2d )
{ return attrvishandler_.editNLA( is2d ); }
bool uiODApplMgr::uvqNLA( bool is2d )
{ return attrvishandler_.uvqNLA( is2d ); }
void uiODApplMgr::createHorOutput( int tp, bool is2d )
{ attrvishandler_.createHorOutput( tp, is2d ); }
void uiODApplMgr::createVol( bool is2d, bool multiattrib )
{ attrvishandler_.createVol( is2d, multiattrib ); }

void uiODApplMgr::seisOut2DCB(CallBacker*)
{ createVol(true,false); }
void uiODApplMgr::seisOut3DCB(CallBacker*)
{ createVol(false,false); }

void uiODApplMgr::doWellXPlot( CallBacker* )
{ attrvishandler_.doXPlot(); }
void uiODApplMgr::doAttribXPlot( CallBacker* )
{ attrvishandler_.crossPlot(); }
void uiODApplMgr::openCrossPlot( CallBacker* )
{ dispatcher_.openXPlot(); }
void uiODApplMgr::setZStretch()
{ attrvishandler_.setZStretch(); }
bool uiODApplMgr::selectAttrib( int id, int attrib )
{ return attrvishandler_.selectAttrib( id, attrib ); }
void uiODApplMgr::setRandomPosData( int visid, int attrib,
				const DataPointSet& data )
{ attrvishandler_.setRandomPosData(visid,attrib,data); }
void uiODApplMgr::pageUpDownPressed( bool pageup )
{ attrvishandler_.pageUpDownPressed(pageup); sceneMgr().updateTrees(); }
void uiODApplMgr::hideColorTable()
{ attrvishandler_.hideColorTable(); }
void uiODApplMgr::updateColorTable( int visid, int attrib )
{ attrvishandler_.updateColorTable( visid, attrib ); }
void uiODApplMgr::colSeqChg( CallBacker* )/*TODO: Remove*/
{ attrvishandler_.colSeqChg(); sceneMgr().updateSelectedTreeItem();  }
void uiODApplMgr::useDefColTab( int visid, int attrib )/*TODO: Remove*/
{ attrvishandler_.useDefColTab(visid,attrib); }
void uiODApplMgr::saveDefColTab( int visid, int attrib )/*TODO: Remove*/
{ attrvishandler_.saveDefColTab(visid,attrib); }

void uiODApplMgr::processPreStack( bool is2d )
{ dispatcher_.processPreStack( is2d ); }
void uiODApplMgr::genAngleMuteFunction( CallBacker* )
{ dispatcher_.genAngleMuteFunction(); }
void uiODApplMgr::createCubeFromWells( CallBacker* )
{ dispatcher_.createCubeFromWells(); }
void uiODApplMgr::bayesClass2D( CallBacker* )
{ dispatcher_.bayesClass(true); }
void uiODApplMgr::bayesClass3D( CallBacker* )
{ dispatcher_.bayesClass(false); }
void uiODApplMgr::startBatchJob()
{ dispatcher_.startBatchJob(); }
void uiODApplMgr::setupBatchHosts()
{ dispatcher_.setupBatchHosts(); }
void uiODApplMgr::batchProgs()
{ dispatcher_.batchProgs(); }
void uiODApplMgr::pluginMan()
{ dispatcher_.pluginMan(); }
void uiODApplMgr::posConversion()
{ dispatcher_.posConversion(); }
void uiODApplMgr::crsPosConversion()
{ dispatcher_.crsPosConversion(); }
void uiODApplMgr::startInstMgr()
{ dispatcher_.startInstMgr(); }
void uiODApplMgr::setAutoUpdatePol()
{ dispatcher_.setAutoUpdatePol(); }
void uiODApplMgr::process2D3D( int opt )
{ dispatcher_.process2D3D( opt ); }

void uiODApplMgr::MiscSurvInfo::refresh()
{
    if ( DBM().isBad() )
	return;

    xyunit_ = SI().xyUnit();
    zunit_ = SI().zUnit();
    zstep_ = SI().zStep();
}


bool uiODApplMgr::isRestoringSession() const
{ return appl_.isRestoringSession(); }


