/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodemsurftreeitem.h"
#include "uipickpartserv.h"
#include "uiseispartserv.h"
#include "uistereodlg.h"
#include "uistratlayermodel.h"
#include "uisurvey.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uiveldesc.h"
#include "uivispartserv.h"
#include "uivisdatapointsetdisplaymgr.h"
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

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "errh.h"
#include "externalattrib.h"
#include "filepath.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "oddirs.h"
#include "odsession.h"
#include "odver.h"
#include "pickset.h"
#include "posinfo2d.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "settings.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

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
	
    IOM().surveyToBeChanged.notify( mCB(this,uiODApplMgr,surveyToBeChanged) );
    IOM().surveyChanged.notify( mCB(this,uiODApplMgr,surveyChanged) );
}


uiODApplMgr::~uiODApplMgr()
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true); attrvishandler_.survChg(true);
    IOM().surveyToBeChanged.remove( mCB(this,uiODApplMgr,surveyToBeChanged) );
    delete mpeserv_;
    delete pickserv_;
    delete nlaserv_;
    delete attrserv_;
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


void uiODApplMgr::resetServers()
{
    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; delete mpeserv_;
    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    visserv_->deleteAllObjects();
    emserv_->removeUndo();
}


void uiODApplMgr::setNlaServer( uiNLAPartServer* s )
{
    nlaserv_ = s;
    if ( nlaserv_ )
	nlaserv_->setDPSDispMgr( visdpsdispmgr_ );
}


int uiODApplMgr::manageSurvey()
{
    BufferString prevnm = GetDataDir();
    uiSurvey dlg( &appl_ );
    if ( !dlg.go() )
	return 0;
    else if ( prevnm == GetDataDir() )
	return 1;
    else
	return 2;
}


void uiODApplMgr::addVisDPSChild( CallBacker* cb )
{
    mCBCapsuleUnpack( int, emid, cb );
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    sceneMgr().addEMItem( emid, sceneids[0] );
}


void uiODApplMgr::surveyToBeChanged( CallBacker* )
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true); attrvishandler_.survChg(true);

    bool anythingasked = false;
    if ( !appl_.askStore(anythingasked,"Survey change") )
    {
	IOM().disallowSurveyChange();
	return;
    }

    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; attrserv_ = 0;
    delete mpeserv_; mpeserv_ = 0;
    delete wellserv_; wellserv_ = 0;
    if ( appl_.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );
}


void uiODApplMgr::surveyChanged( CallBacker* )
{
    dispatcher_.survChg(false); attrvishandler_.survChg(false);
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" )
	sceneMgr().addScene( true );

    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );
    if ( survChgReqAttrUpdate() )
    {
	attrserv_->setAttrsNeedUpdt();
	tmpprevsurvinfo_.refresh();
    }

    mpeserv_ = new uiMPEPartServer( applservice_ );
    MPE::engine().init();

    wellserv_ = new uiWellPartServer( applservice_ );
}


bool uiODApplMgr::survChgReqAttrUpdate()
{
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
    menuMgr().dtectTB()->setSensitive( yn );
    menuMgr().manTB()->setSensitive( yn );
    menuMgr().enableMenuBar( yn );
}


void uiODApplMgr::enableTree( bool yn )
{
    sceneMgr().disabTrees( !yn );
    visServer()->blockMouseSelection( !yn );
}


void uiODApplMgr::enableSceneManipulation( bool yn )
{
    if ( !yn ) sceneMgr().setToViewMode();
    menuMgr().enableActButton( yn );
}


void uiODApplMgr::editAttribSet()
{ editAttribSet( SI().has2D() ); }
void uiODApplMgr::editAttribSet( bool is2d )
{
    enableMenusAndToolBars( false );
    enableSceneManipulation( false );
    attrserv_->editSet( is2d ); 
}

void uiODApplMgr::processTime2Depth( CallBacker* )
{
    seisserv_->processTime2Depth();
}


void uiODApplMgr::processVelConv( CallBacker* )
{
    seisserv_->processVelConv();
}


void uiODApplMgr::setStereoOffset()
{
    ObjectSet<ui3DViewer> vwrs;
    sceneMgr().getSoViewers( vwrs );
    uiStereoDlg dlg( &appl_, vwrs );
    dlg.go();
}


void uiODApplMgr::addTimeDepthScene()
{
    uiDialog::Setup setup("Velocity model",
		"Select velocity model to base scene on","0.4.4");
    uiSingleGroupDlg dlg( &appl_, setup );
    uiTimeDepthBase* uitrans = SI().zIsTime() 
	? (uiTimeDepthBase*) new uiTime2Depth( &dlg )
	: (uiTimeDepthBase*) new uiDepth2Time( &dlg );
    dlg.setGroup( uitrans );
    if ( !dlg.go() ) return;

    BufferString snm( SI().zIsTime() ? sKey::Depth() : sKey::Time() );
    RefMan<ZAxisTransform> ztrans = uitrans->getSelection();
    if ( !ztrans )
	return;

    snm += " (using '";
    snm += uitrans->selName();
    snm += "')";

    sceneMgr().tile();
    const int sceneid = sceneMgr().addScene( true, ztrans, snm );
    if ( sceneid!=-1 )
    {
	const float zscale = SI().zIsTime()
	    ? SurveyInfo::defaultXYtoZScale(SurveyInfo::Meter, SI().xyUnit())
	    : SurveyInfo::defaultXYtoZScale(SurveyInfo::Second, SI().xyUnit());

	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneid) );
	CubeSampling cs = SI().sampling( true );
	cs.zrg = uitrans->getZRange();
	scene->setCubeSampling( cs );
	scene->setZScale( zscale );
	sceneMgr().viewAll( 0 );
    }
}


void uiODApplMgr::showBaseMap()
{
    dispatcher_.showBaseMap();
}


void uiODApplMgr::setWorkingArea()
{
    if ( visserv_->setWorkingArea() )
	sceneMgr().viewAll(0);
}


void uiODApplMgr::selectWells( ObjectSet<MultiID>& wellids )
{ wellserv_->selectWells( wellids ); }

bool uiODApplMgr::storePickSets()
{ return pickserv_->storeSets(); }

bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv_->storeSet( ps ); }

bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv_->storeSetAs( ps ); }

bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{
    const int sceneid = sceneMgr().askSelectScene();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneid) );
    const float velocity = scene->getZStretch() * scene->getZScale();
    return attrserv_->setPickSetDirs(ps, nlaserv_ ? &nlaserv_->getModel():0,velocity);
}

bool uiODApplMgr::pickSetsStored() const
{ return pickserv_->pickSetsStored(); }


bool uiODApplMgr::getNewData( int visid, int attrib )
{
    if ( visid<0 ) return false;

    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    Attrib::SelSpec myas( *as );
    if ( myas.id() != Attrib::DescID::undef() )
	attrserv_->updateSelSpec( myas );
    if ( myas.id().isUnselInvalid() )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 

    bool res = false;
    switch ( visserv_->getAttributeFormat(visid,attrib) )
    {
	case uiVisPartServer::Cube :
	{
	    const DataPack::ID cacheid =
				visserv_->getDataPackID( visid, attrib );
	    if ( cacheid == DataPack::cNoID() )
		useDefColTab( visid, attrib );

	    CubeSampling cs = visserv_->getCubeSampling( visid, attrib );
	    if ( !cs.isDefined() )
		return false;

	    if ( myas.id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc = 
			    Attrib::ExtAttrFact().create( 0, myas, false );
		if ( !calc )
		{
		    BufferString errstr( "Selected attribute '", myas.userRef(),
			 "'\nis not present in the set and cannot be created" );
		    uiMSG().error( errstr );
		    return false;
		}

		uiTaskRunner progm( &appl_ );
		const DataPack::ID dpid =
		    calc->createAttrib( cs, cacheid, &progm );
		if ( dpid==DataPack::cNoID() && !calc->errmsg_.isEmpty() )
		{
		    uiMSG().error( calc->errmsg_ );
		    return false;
		}

		res = dpid != DataPack::cNoID();
		visserv_->setDataPackID( visid, attrib, dpid );
		break;
	    }

	    attrserv_->setTargetSelSpec( myas );
	    const DataPack::ID newid = attrserv_->createOutput( cs, cacheid );
	    if ( newid == DataPack::cNoID() )
	    {
		// clearing texture and set back original selspec
		visserv_->setSelSpec( visid, attrib, Attrib::SelSpec() );
		visserv_->setSelSpec( visid, attrib, myas );
		return false;
	    }

	    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pd,
		    visserv_->getObject(visid) );
	    if ( false && pd && pd->nrAttribs() > 1 )//disabled for now
	    {
		const float step0 = pd->getDisplayMinDataStep(true);
		const float step1 = pd->getDisplayMinDataStep(false);
		if ( step0>0 && step1>0 )
		{
		    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
		    const DataPack* dp = dpman.obtain( newid );
		    mDynamicCastGet(const FlatDataPack*, fdp, dp);
		    if ( fdp )
		    {
			const float newstep0 = (float) fdp->posData().range(true).step;
			const float newstep1 = (float) fdp->posData().range(false).step;
			if ( !(mIsEqual(step0,newstep0,(newstep0+step0)*5E-4)
			    && mIsEqual(step1,newstep1,(newstep1+step1)*5E-4)) )
			{
			    BufferString msg = fdp->name();
			    msg += " has different stepout as the loaded ones.";
			    msg += " Do you want to continue? If continue,";
			    msg += " all the attributes will be resampled to";
			    msg +=" the minimum stepout.";
			    if ( !uiMSG().askGoOn( msg.buf(),
					"Yes, load it","No, not now") )
			    {
				dpman.release( newid );
				return false;
			    }
			}
		    }

		    visserv_->setDataPackID( visid, attrib, newid );
		    res = true;
		    dpman.release( newid );
		    break;
		}
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
	    attrserv_->setTargetSelSpec( myas );
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
		    	    visserv_->getObject(visid) );
	    DataPack::ID cacheid = rdmtdisp->getDataPackID( attrib );
	    if ( cacheid == DataPack::cNoID() )
		useDefColTab( visid, attrib );
	    TypeSet<BinID>* trcspath = rdmtdisp ? rdmtdisp->getPath() : 0;
	    TypeSet<BinID>* trueknotspos = rdmtdisp ? rdmtdisp->getKnots() : 0;
	    if ( myas.id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc = 
			    Attrib::ExtAttrFact().create( 0, myas, false );

		// TODO implement 
		break;
	    }

	    const DataPack::ID newid =
		attrserv_->createRdmTrcsOutput( zrg, trcspath, trueknotspos );

	    res = true;
	    if ( newid == -1 )
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

    updateColorTable( visid, attrib );
    return res;
}


void uiODApplMgr::calShiftAttribute( int attrib, const Attrib::SelSpec& as )
{
    uiTreeItem* parent = sceneMgr().findItem( visserv_->getEventObjId() );
    if ( !parent ) return;

    ObjectSet<DataPointSet> dpsset;
    emattrserv_->fillHorShiftDPS( dpsset, 0 );

    if ( mIsUdf(attrib) )
    {
	uiODAttribTreeItem* itm =
	    new uiODEarthModelSurfaceDataTreeItem(
		    visserv_->getEventObjId(), 0, typeid(*parent).name() );
	parent->addChild( itm, false );
	attrib = visserv_->addAttrib( visserv_->getEventObjId() );
	emattrserv_->setAttribIdx( attrib );
    }

    attrserv_->setTargetSelSpec( as );
    attrserv_->createOutput( dpsset, 1 );

    TypeSet<DataPointSet::DataRow> drset;
    BufferStringSet nmset;
    mDeclareAndTryAlloc( DataPointSet*, dps,
	    DataPointSet( drset, nmset, false, true ) );
    if ( !dps )
    {
	deepErase( dpsset );
	return;
    }

    mDeclareAndTryAlloc(DataColDef*,siddef,DataColDef(emattrserv_->sidDef()));
    if ( !siddef )
    {
	deepErase( dpsset );
	return;
    }
    
    dps->dataSet().add( siddef );
    if ( !dps->bivSet().setNrVals( dpsset.size()+2 ) )
    {
	deepErase( dpsset );
	return;
    }

    mAllocVarLenArr( float, attribvals, dpsset.size()+2 );
    if ( !mIsVarLenArrOK(attribvals) )
    {
	deepErase( dpsset );
	return;
    }

    attribvals[0] = 0.0; //depth

    BinIDValueSet::Pos bvspos;
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

    deepErase( dpsset );
}


bool uiODApplMgr::calcRandomPosAttrib( int visid, int attrib )
{
    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    Attrib::SelSpec myas( *as );
    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
    {
	const MultiID surfmid = visserv_->getMultiID(visid);
	const EM::ObjectID emid = emserv_->getObjectID(surfmid);
	const int auxdatanr = emserv_->loadAuxData( emid, myas.userRef() );
        if ( auxdatanr<0 )
	    uiMSG().error( "Cannot find stored data" );
	else
	{
	    DataPointSet* data = new DataPointSet( false, true );
	    DPM( DataPackMgr::PointID() ).addAndObtain( data );

	    TypeSet<float> shifts( 1, 0 );
	    emserv_->getAuxData( emid, auxdatanr, *data, shifts[0] );
	    createAndSetMapDataPack( visid, attrib, *data, 2 );
	    DPM( DataPackMgr::PointID() ).release( data->id() );
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

    DataPointSet* data = new DataPointSet( false, true );
    DPM( DataPackMgr::PointID() ).addAndObtain( data );

    visserv_->getRandomPos( visid, *data );
    const int firstcol = data->nrCols();
    data->dataSet().add( new DataColDef(myas.userRef()) );
    attrserv_->setTargetSelSpec( myas );
    if ( !attrserv_->createOutput(*data,firstcol) )
    {
	DPM( DataPackMgr::PointID() ).release( data->id() );
	return false;
    }

    const int dataidx = data->dataSet().findColDef( DataColDef(myas.userRef()),
	    					    PosVecDataSet::NameExact );
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv_->getObject(visid))
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,visserv_->getObject(visid))
    if ( fd )
    {
	const int attrnr = visServer()->getSelAttribNr();
	const int id = fd->addDataPack( *data );
	fd->setDataPackID( attrnr, id, 0 );
	fd->setRandomPosData( attrnr, data, 0 );
    }
    else
    {
	createAndSetMapDataPack( visid, attrib, *data, dataidx );
	if ( hd )
	{
	    TypeSet<float> shifts( 1, (float) visserv_->getTranslation(visid).z );
	    hd->setAttribShift( attrib, shifts );
	}
    }

    DPM( DataPackMgr::PointID() ).release( data->id() );
    return true;
}


bool uiODApplMgr::evaluateAttribute( int visid, int attrib )
{
    uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
    if ( format == uiVisPartServer::Cube )
    {
	const CubeSampling cs = visserv_->getCubeSampling( visid );
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
	TypeSet<BinID>* trcspath = rdmtdisp ? rdmtdisp->getPath() : 0;
	TypeSet<BinID>* trueknotspos = rdmtdisp ? rdmtdisp->getKnots() : 0;
	const DataPack::ID dpid =
	    	attrserv_->createRdmTrcsOutput( zrg, trcspath, trueknotspos);
	visserv_->setDataPackID( visid, attrib, dpid );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	DataPointSet data( false, true );
	visserv_->getRandomPos( visid, data );
	attrserv_->createOutput( data, data.nrCols() );
	visserv_->setRandomPosData( visid, attrib, &data );
    }
    else
    {
	uiMSG().error( "Cannot evaluate attributes on this object" );
	return false;
    }

    return true;
}


bool uiODApplMgr::evaluate2DAttribute( int visid, int attrib )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    	    visserv_->getObject(visid))
    if ( !s2d ) return false;

    CubeSampling cs;
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
    cs.hrg.start.crl = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl = s2d->getTraceNrRange().stop;
    cs.zrg.setFrom( s2d->getZRange(false) );

    uiTaskRunner uitr( &appl_ ); 
    LineKey lk( s2d->name() );
    DataPack::ID dpid = attrserv_->create2DOutput( cs, lk, uitr );
    if ( dpid < 0 )
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
	    if ( nlaserv_ )
		nlaserv_->set2DEvent( isnlamod2d );
	    return nlaserv_ ? (void*)(&nlaserv_->getModel()) : 0;
	}
    }
    else
	pErrMsg("deliverObject for unsupported part server");

    return 0;
}


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);
	const int sceneid = mpeserv_->getCurSceneID();
	const int sdid = sceneMgr().addEMItem( emid, sceneid );
	if ( sdid==-1 )
	    return false;

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);

	//const MultiID mid = emserv_->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> hordisplayids;
	visserv_->findObject( typeid(visSurvey::HorizonDisplay), 
			      hordisplayids );

	TypeSet<int> hor2ddisplayids;
	visserv_->findObject( typeid(visSurvey::Horizon2DDisplay),
			      hor2ddisplayids );

	hordisplayids.append( hor2ddisplayids );	

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
	const Attrib::SelSpec* as = mpeserv_->getAttribSelSpec();
	if ( !as ) return false;
	const CubeSampling cs = mpeserv_->getAttribVolume(*as);
	DataPack::ID olddatapackid = mpeserv_->getAttribCacheID(*as);
	attrserv_->setTargetSelSpec( *as );
	DataPack::ID datapackid = attrserv_->createOutput( cs, olddatapackid );
	mpeserv_->setAttribData( *as, datapackid );
	if ( olddatapackid != datapackid )
	{
	    const bool isflat = cs.isFlat();
	    DataPackMgr& dpman = DPM( isflat ? DataPackMgr::FlatID()
		    			     : DataPackMgr::CubeID() );
	    dpman.release( olddatapackid );
	}
    }
    else if ( evid==uiMPEPartServer::evCreate2DSelSpec() )
    {
	LineKey lk( mpeserv_->get2DLineSet(), mpeserv_->get2DAttribName() );
	const Attrib::DescID attribid = attrServer()->getStoredID( lk, true );
	if ( !attribid.isValid() ) return false;

	const Attrib::SelSpec as( mpeserv_->get2DAttribName(), attribid );
	mpeserv_->set2DSelSpec( as );
    }
    else if ( evid==uiMPEPartServer::evShowToolbar() )
    {
	visserv_->disabToolBars( false );
	visserv_->showMPEToolbar();
    }
    else if ( evid==uiMPEPartServer::evHideToolBar() )
	visserv_->disabToolBars( true );
    else if ( evid==uiMPEPartServer::evMPEDispIntro() )
	visserv_->introduceMPEDisplay();
    else if ( evid==uiMPEPartServer::evInitFromSession() )
	visserv_->initMPEStuff();
    else if ( evid==uiMPEPartServer::evUpdateTrees() )
	sceneMgr().updateTrees();
    else if ( evid==uiMPEPartServer::evUpdateSeedConMode() )
	visserv_->updateSeedConnectMode();
    else if ( evid==uiMPEPartServer::evMPEStoreEMObject() )
    {
	storeEMObject();
    }
    else if ( evid==uiMPEPartServer::evSaveUnsavedEMObject() )
    {
	const EM::ObjectID emid = emserv_->saveUnsavedEMObject();
	const MultiID mid = emserv_->getStorageID(emid);
	TypeSet<int> ids;
	visserv_->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    visserv_->setObjectName( ids[idx],
		    		     (const char*) emserv_->getName(emid) );
	}

	if ( emserv_->getType(emid)==EM::Horizon3D::typeStr() || 
	     emserv_->getType(emid)==EM::Horizon2D::typeStr() )
	{
	    mpeserv_->saveSetup( mid );
	}

	sceneMgr().updateTrees();
    }
    else if ( evid==uiMPEPartServer::evRemoveUnsavedEMObject() )
    {
	emserv_->removeUnsavedEMObjectFromTree();
    }
    else if ( evid==uiMPEPartServer::evRetrackInVolume() )
    {
	visserv_->trackInVolume();
    }
    else
	pErrMsg("Unknown event from mpeserv");

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine() )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
	//enableTree( false );
	//enableMenusAndToolBars( false );
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

	const BufferStringSet& wellids = wellserv_->createdWellIDs();
	for ( int idx=0; idx<wellids.size(); idx++ )
	    sceneMgr().addWellItem( MultiID(wellids.get(idx)), sceneid );
    }

    return true;
}


bool uiODApplMgr::handleWellAttribServEv( int evid )
{
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon() )
    {
	const int sceneid = sceneMgr().askSelectScene();
	if ( sceneid<0 ) return false;

	const EM::ObjectID emid = emserv_->selEMID();
	sceneMgr().addEMItem( emid, sceneid );
	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evRemoveTreeObject() )
    {
	const EM::ObjectID emid = emserv_->selEMID();
	//const MultiID mid = emserv_->getStorageID(emid);

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
	    if ( fd && fd->getEMID()==emid )
		remove = true;

	    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fsd,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( fsd && fsd->getEMID()==emid )
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
	pErrMsg("Unknown event from emserv");

    return true;
}


bool uiODApplMgr::handleEMAttribServEv( int evid )
{
    const int visid = visserv_->getEventObjId();
    const int attribidx = emattrserv_->attribIdx();
    if ( evid == uiEMAttribPartServer::evCalcShiftAttribute() )
    {
	const Attrib::SelSpec as( emattrserv_->getAttribBaseNm(),
				  emattrserv_->attribID() );
	calShiftAttribute( attribidx, as );
    }
    else if ( evid == uiEMAttribPartServer::evHorizonShift() )
    {
	const int textureidx = emattrserv_->textureIdx();
	visserv_->setTranslation( visid, Coord3(0,0,emattrserv_->getShift()) );
	if ( !mIsUdf(attribidx) )
	    visserv_->selectTexture( visid, attribidx, textureidx );

	uiTreeItem* parent = sceneMgr().findItem( visid );
	if ( parent )
	    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( evid == uiEMAttribPartServer::evStoreShiftHorizons() )
    {
	const uiVisPartServer::AttribFormat format = 
				visserv_->getAttributeFormat( visid, -1 );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	DataPointSet data( false, true );
	visserv_->getRandomPosCache( visid, attribidx, data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const int nrvals = data.bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    float shift =
		emattrserv_->shiftRange().atIndex(idx) *
		SI().zDomain().userFactor();
	    BufferString shiftstr;
	    getStringFromFloat( SI().zIsTime() ? "%g" : "%f", shift,
		    		shiftstr.buf() );
	    if ( mIsZero(shift,1e-3) ) shiftstr = "0";
	    BufferString auxdatanm( emattrserv_->getAttribBaseNm() );
	    auxdatanm += " ["; auxdatanm += shiftstr; auxdatanm += "]";
	    emserv_->setAuxData( emid, data, auxdatanm, idx+2,
		   		 emattrserv_->shiftRange().atIndex(idx) );
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
		visserv_->removeAttrib( visid, emattrserv_->attribIdx() );
	    }
	}

	if ( !isok )
	{
	    visserv_->setTranslation( visid,
		    Coord3(0,0,emattrserv_->initialShift() ) );

	    for ( int idx=0; idx<enableattrib.size(); idx++ )
		visserv_->enableAttrib( visid, idx, enableattrib[idx] );

	    uiTreeItem* parent = sceneMgr().findItem( visid );
	    if ( parent )
		parent->updateColumnText( uiODSceneMgr::cNameColumn() );
	}
	else
	{
	    for ( int idx=0; idx<visserv_->getNrAttribs(visid); idx++ )
	    {
		if ( idx==attribidx )
		{
		    DataPointSet data( false, true );
		    visserv_->getRandomPosCache( visid, attribidx, data );
		    if ( data.isEmpty() )
			continue;

		    const int sididx = data.dataSet().findColDef(
			    emattrserv_->sidDef(), PosVecDataSet::NameExact );

		    int texturenr = emattrserv_->textureIdx() + 1;
		    if ( sididx<=texturenr )
			texturenr++;

		    const int nrvals = data.bivSet().nrVals();
		    for ( int idy=nrvals-1; idy>0; idy-- )
		    {
			if ( idy!=texturenr && idy!=sididx )
			    data.bivSet().removeVal( idy );
		    }

		    visserv_->setRandomPosData( visid, attribidx, &data );
		}
	    }
	}
    }
    else
	pErrMsg("Unknown event from emattrserv");

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
	TypeSet<EM::ObjectID> horids;
	const ObjectSet<MultiID>& storids = pickserv_->selHorIDs();
	for ( int idx=0; idx<storids.size(); idx++ )
	{
	    const MultiID horid = *storids[idx];
	    const EM::ObjectID id = emserv_->getObjectID(horid);
	    if ( id<0 || !emserv_->isFullyLoaded(id) )
		emserv_->loadSurface( horid );

	    horids += emserv_->getObjectID(horid);
	}
	
	emserv_->getSurfaceDef3D( horids, pickserv_->genDef(),
			       pickserv_->selHorSampling() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef2D() )
	emserv_->getSurfaceDef2D( pickserv_->selHorIDs(),
				  pickserv_->lineGeoms(),
				  pickserv_->selectLines(),
				  pickserv_->lineSetID(),
				  pickserv_->getPos2D(),
				  pickserv_->getHor2DZRgs() );
    else if ( evid == uiPickPartServer::evFillPickSet() )
	emserv_->fillPickSet( *pickserv_->pickSet(), pickserv_->horID() );
    else if ( evid == uiPickPartServer::evGet2DLineInfo() )
	seisserv_->get2DLineInfo( pickserv_->lineSets(),
				  pickserv_->lineSetIds(),
				  pickserv_->lineNames());
    else if ( evid == uiPickPartServer::evGet2DLineDef() )
    {
	BufferStringSet& lnms = pickserv_->selectLines();
	for ( int idx=0; idx<lnms.size(); idx++ )
	{
	    PosInfo::Line2DData geom;
	    seisserv_->get2DLineGeometry( pickserv_->lineSetID(), *lnms[idx],
		   			  geom );
	    pickserv_->lineGeoms() += new PosInfo::Line2DData( geom );
	}
    }
    else
	pErrMsg("Unknown event from pickserv");

    return true;
}


#define mGetSelTracker( tracker ) \
    const int selobjvisid = visserv_->getSelObjectId(); \
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,\
	                        visserv_->getObject(selobjvisid));\
    const EM::ObjectID emid = emod ? emod->getObjectID() : -1; \
    const int trackerid = mpeserv_->getTrackerID(emid); \
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid ); 

bool uiODApplMgr::handleVisServEv( int evid )
{
    int visid = visserv_->getEventObjId();
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
    else if ( evid == uiVisPartServer::evShowSetupDlg() )
    {
	mGetSelTracker( tracker );
	const MPE::EMSeedPicker* seedpicker = tracker ? 
					      tracker->getSeedPicker(false) : 0;
	const EM::SectionID sid = seedpicker ? seedpicker->getSectionID() : -1;
	mpeserv_->showSetupDlg( emid, sid );
	visserv_->updateMPEToolbar();
    }
    else if ( evid == uiVisPartServer::evPostponedLoadingData() )
	mpeserv_->postponeLoadingCurVol();
    else if ( evid == uiVisPartServer::evLoadPostponedData() )
	mpeserv_->loadPostponedVolume();
    else if ( evid == uiVisPartServer::evToggleBlockDataLoad() )
	mpeserv_->blockDataLoading( !mpeserv_->isDataLoadingBlocked() );
    else if ( evid == uiVisPartServer::evDisableSelTracker() )
    {
	mGetSelTracker( tracker );
	if ( tracker )
	    tracker->enable( false );
    }
    else if ( evid == uiVisPartServer::evColorTableChange() )
	updateColorTable( visserv_->getEventObjId(), 
			  visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evLoadAttribDataInMPEServ() )
	mpeserv_->fireLAttribData();
    else if ( evid == uiVisPartServer::evFromMPEManStoreEMObject() )
	storeEMObject();
    else if (evid == uiVisPartServer::evGetHeadOnIntensity() )
    {
	visserv_->setHeadOnIntensity( sceneMgr().getHeadOnLightIntensity( 
		    visserv_->getEventObjId() ) );
    }
    else if (evid == uiVisPartServer::evSetHeadOnIntensity() )
    {
	sceneMgr().setHeadOnLightIntensity( visserv_->getEventObjId(),
	       visserv_->getHeadOnIntensity() );
    }

    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite() )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet* ads = attrserv_->curDescSet(nlaserv_->is2DEvent());
	if ( !ads ) return false;
	IOPar& iopar = nlaserv_->modelPars();
	iopar.setEmpty();
	BufferStringSet inputs = nlaserv_->modelInputs();
	const Attrib::DescSet* cleanads = ads->optimizeClone( inputs );
	(cleanads ? cleanads : ads)->fillPar( iopar );
	delete cleanads;
    }
    else if ( evid == uiNLAPartServer::evPrepareRead() )
    {
	bool saved = attrserv_->setSaved(nlaserv_->is2DEvent());
        const char* msg = "Current attribute set is not saved.\nSave now?";
        if ( !saved && uiMSG().askSave( msg ) )
	    attrserv_->saveSet(nlaserv_->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished() )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent(),
	       		       nlaserv_->getModel().versionNr() );
	wellattrserv_->setNLAModel( &nlaserv_->getModel() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames() )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv_->getPossibleOutputs( nlaserv_->is2DEvent(),
				      nlaserv_->inputNames() );
	if ( nlaserv_->inputNames().size() == 0 )
	    { uiMSG().error( "No usable input" ); return false; }
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

	if ( !attrserv_->curDescSet(nlaserv_->is2DEvent()) ) 
	    { pErrMsg("Huh"); return false; }
	ObjectSet<DataPointSet> dpss;
	const bool dataextraction = nlaserv_->willDoExtraction();
	if ( !dataextraction )
	    dpss += new DataPointSet( nlaserv_->is2DEvent(), false );
	else
	{
	    nlaserv_->getDataPointSets( dpss );
	    if ( dpss.isEmpty() )
		{ uiMSG().error("No matching well data found"); return false;}
	    bool allempty = true;
	    for ( int idx=0; idx<dpss.size(); idx++ )
		{ if ( !dpss[idx]->isEmpty() ) { allempty = false; break; } }
	    if ( allempty )
		{ uiMSG().error("No valid data locations found"); return false;}
	    if ( !attrserv_->extractData(dpss) )
		{ deepErase(dpss); return true; }
	    IOPar& iop = nlaserv_->storePars();
	    attrserv_->curDescSet(nlaserv_->is2DEvent())->fillPar( iop );
	    if ( iop.name().isEmpty() )
		iop.setName( "Attributes" );
	}

	const FixedString res = nlaserv_->prepareInputData( dpss );
	if ( res!=uiNLAPartServer::sKeyUsrCancel() )
	    uiMSG().warning( res );

	float vsn = mODMajorVersion + 0.1*mODMinorVersion;
	if ( !dataextraction ) // i.e. if we have just read a DataPointSet
	    attrserv_->replaceSet( dpss[0]->dataSet().pars(), dpss[0]->is2D(),
		   		   vsn );
	deepErase(dpss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass() )
    {
	const DataPointSet& dps = nlaserv_->dps();
	DataPointSet mcpicks( dps.is2D() );
	for ( int irow=0; irow<dps.size(); irow++ )
	{
	    if ( dps.group(irow) == 3 )
		mcpicks.addRow( dps.dataRow(irow) );
	}
	mcpicks.dataChanged();
	pickserv_->setMisclassSet( mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet() )
    {
	Attrib::DescSet attrset(nlaserv_->is2DEvent());
	if ( !attrserv_->createAttributeSet(nlaserv_->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv_->modelPars() );
	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent(),
	       		       nlaserv_->getModel().versionNr() );
    }
    else if ( evid == uiNLAPartServer::evCr2DRandomSet() )
    {
	pickserv_->createRandom2DSet();
    }
    else
	pErrMsg("Unknown event from nlaserv");

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
	    uiMSG().error( "Please select an attribute element in the tree" );
	    return false;
	}

	visserv_->setSelSpec( visid, attrib, as );
	visserv_->setColTabMapperSetup( visid, attrib, ColTab::MapperSetup() );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet() )
    {
	attribSetChg.trigger();
	mpeserv_->setCurrentAttribDescSet(
				attrserv_->curDescSet(attrserv_->is2DEvent()) );
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
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttrib() );
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    uiMSG().error( "Please select an attribute element in the tree" );
	    return false;
	}
	if ( !calcMultipleAttribs( as ) )
	{
	    uiMSG().error( "Could not evaluate this attribute" );
	    return false;
	}

	const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
	const ColTab::MapperSetup* ms =
	    visserv_->getColTabMapperSetup( visid, attrib, tmpset.size()/2 );

	attrserv_->setEvalBackupColTabMapper( ms );

	if ( ms )
	{
	    ColTab::MapperSetup myms = *ms;
	    myms.type_ = ColTab::MapperSetup::Fixed;
	    visserv_->setColTabMapperSetup( visid, attrib, myms );
	}
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

	DataPointSet data( false, true );
	visserv_->getRandomPosCache( visid, attrnr, data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const float shift = (float) visserv_->getTranslation(visid).z;
	const TypeSet<Attrib::SelSpec>& specs = attrserv_->getTargetSelSpecs();
	const int nrvals = data.bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv_->setAuxData( emid, data, specs[idx].userRef(),
				 idx+2, shift );
	    BufferString dummy;
	    emserv_->storeAuxData( emid, dummy, false );
	}
    }
    else if ( evid==uiAttribPartServer::evEvalRestore() )
    {
	if ( attrserv_->getEvalBackupColTabMapper() )
	{
	    visserv_->setColTabMapperSetup( visid, attrib,
		    *attrserv_->getEvalBackupColTabMapper() );
	}

	Attrib::SelSpec* as = const_cast<Attrib::SelSpec*>(
		visserv_->getSelSpec(visid,attrib) );
	const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
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
	pErrMsg("Unknown event from attrserv");

    return true;
}


bool uiODApplMgr::calcMultipleAttribs( Attrib::SelSpec& as )
{
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    const int visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
    BufferString savedusrref = tmpset.size() ? tmpset[0].objectRef() : "";
    as.setObjectRef( savedusrref );
    as.set2DFlag( attrserv_->is2DEvent() );  
    as.setUserRef( tmpset[0].userRef() );
    visserv_->setSelSpec( visid, attrib, as );
    BufferStringSet* refs = new BufferStringSet();
    for ( int idx=0; idx<tmpset.size(); idx++ )
	refs->add( tmpset[idx].userRef() );
    visserv_->setUserRefs( visid, attrib, refs );
    visserv_->setColTabMapperSetup( visid, attrib, ColTab::MapperSetup() );
    return as.is2D() ? evaluate2DAttribute(visid,attrib)
		     : evaluateAttribute(visid,attrib);
}


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv_->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<int> plids;
    TypeSet<int> sceneids;
    visSurvey::PolyLineDisplay* pl = visSurvey::PolyLineDisplay::create();
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


void uiODApplMgr::storeEMObject()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
	    			surface, visserv_->getObject(selectedids[0]) );
    if ( !surface ) return;
    
    const EM::ObjectID emid = surface->getObjectID();
    MultiID mid = emserv_->getStorageID( emid );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    const bool saveas = mid.isEmpty() || !ioobj;
    emserv_->storeObject( emid, saveas );

    TypeSet<int> ids;
    mid = emserv_->getStorageID( emid );
    visserv_->findObject( mid, ids );
    
    for ( int idx=0; idx<ids.size(); idx++ )
	visserv_->setObjectName( ids[idx], emserv_->getName(emid).buf() );

    mpeserv_->saveSetup( mid );
    sceneMgr().updateTrees();
}


void uiODApplMgr::tieWellToSeismic( CallBacker* )
{ wellattrserv_->createD2TModel(MultiID()); }
void uiODApplMgr::doWellLogTools( CallBacker* )
{ wellserv_->doLogTools(); }
void uiODApplMgr::doLayerModeling( CallBacker* )
{ uiStratLayerModel::doBasicLayerModel(); }
void uiODApplMgr::doVolProcCB( CallBacker* )
{ attrserv_->doVolProc( 0 ); }
void uiODApplMgr::doVolProc( const MultiID& mid )
{ attrserv_->doVolProc( &mid ); }
void uiODApplMgr::createVolProcOutput( CallBacker* )
{ attrserv_->createVolProcOutput(); }
bool uiODApplMgr::editNLA( bool is2d )
{ return attrvishandler_.editNLA( is2d ); }
void uiODApplMgr::createHorOutput( int tp, bool is2d )
{ attrvishandler_.createHorOutput( tp, is2d ); }
void uiODApplMgr::createVol( bool is2d )
{ attrvishandler_.createVol( is2d ); }
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
void uiODApplMgr::setHistogram( int visid, int attrib )
{ attrvishandler_.setHistogram(visid,attrib); }
void uiODApplMgr::colMapperChg( CallBacker* )
{ attrvishandler_.colMapperChg(); }
void uiODApplMgr::createAndSetMapDataPack( int visid, int attrib,
					   const DataPointSet& data, int colnr )
{ attrvishandler_.createAndSetMapDataPack(visid,attrib,data,colnr); }
void uiODApplMgr::pageUpDownPressed( bool pageup )
{ attrvishandler_.pageUpDownPressed(pageup); sceneMgr().updateTrees(); }
void uiODApplMgr::updateColorTable( int visid, int attrib )
{ attrvishandler_.updateColorTable( visid, attrib ); }
void uiODApplMgr::colSeqChg( CallBacker* )
{ attrvishandler_.colSeqChg(); sceneMgr().updateSelectedTreeItem(); }
NotifierAccess* uiODApplMgr::colorTableSeqChange()
{ return attrvishandler_.colorTableSeqChange(); }
void uiODApplMgr::useDefColTab( int visid, int attrib )
{ attrvishandler_.useDefColTab(visid,attrib); }
void uiODApplMgr::saveDefColTab( int visid, int attrib )
{ attrvishandler_.saveDefColTab(visid,attrib); }

void uiODApplMgr::processPreStack( CallBacker* )
{ dispatcher_.processPreStack(); }
void uiODApplMgr::genAngleMuteFunction( CallBacker* )
{ dispatcher_.genAngleMuteFunction(); }
void uiODApplMgr::createCubeFromWells( CallBacker* )
{ dispatcher_.createCubeFromWells(); }
void uiODApplMgr::bayesClass2D( CallBacker* )
{ dispatcher_.bayesClass(true); }
void uiODApplMgr::bayesClass3D( CallBacker* )
{ dispatcher_.bayesClass(false); }
void uiODApplMgr::resortSEGY( CallBacker* )
{ dispatcher_.resortSEGY(); }
void uiODApplMgr::reStartProc()
{ dispatcher_.reStartProc(); }
void uiODApplMgr::batchProgs()
{ dispatcher_.batchProgs(); }
void uiODApplMgr::pluginMan()
{ dispatcher_.pluginMan(); }
void uiODApplMgr::posConversion()
{ dispatcher_.posConversion(); }
void uiODApplMgr::manageShortcuts()
{ dispatcher_.manageShortcuts(); }
void uiODApplMgr::setFonts()
{ dispatcher_.setFonts(); }
int uiODApplMgr::createMapDataPack( const DataPointSet& data, int colnr )
{ return dispatcher_.createMapDataPack( data, colnr ); }
void uiODApplMgr::startInstMgr()
{ dispatcher_.startInstMgr(); }
void uiODApplMgr::setAutoUpdatePol()
{ dispatcher_.setAutoUpdatePol(); }
void uiODApplMgr::create2Dfrom3D()
{ dispatcher_.process2D3D( true ); }
void uiODApplMgr::create3Dfrom2D()
{ dispatcher_.process2D3D( false ); }


void uiODApplMgr::MiscSurvInfo::refresh()
{
    xyunit_ = SI().xyUnit();
    zunit_ = SI().zUnit();
    zstep_ = SI().zStep();
}
