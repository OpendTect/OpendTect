/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodapplmgr.cc,v 1.65 2004-12-16 16:31:11 nanne Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"

#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uiattribpartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uiempartserv.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "uitrackingpartserv.h"
#include "vissurvpickset.h"
#include "vissurvsurf.h"
#include "vissurvsurfeditor.h"
#include "visinterpret.h"
#include "uiattrsurfout.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "seisbuf.h"
#include "binidvalset.h"
#include "pickset.h"
#include "survinfo.h"
#include "errh.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "featset.h"
#include "helpview.h"
#include "filegen.h"
#include "ptrman.h"

#include "uimsg.h"
#include "uifontsel.h"
#include "uitoolbar.h"
#include "uibatchlaunch.h"
#include "uipluginman.h"
#include "uibatchprogs.h"
#include "uiviscoltabed.h"
#include "uifiledlg.h"
#include "uisurvey.h"
#include "uistereodlg.h"

static BufferString retstr;


class uiODApplService : public uiApplService
{
public:
    			uiODApplService( uiParent* p, uiODApplMgr& am )
			: par_(p), applman(am)	{}
    uiParent*		parent() const		{ return par_; }
    bool		eventOccurred( const uiApplPartServer* ps, int evid )
			{ return applman.handleEvent( ps, evid ); }
    void*		getObject( const uiApplPartServer* ps, int evid )
			{ return applman.deliverObject( ps, evid ); }

    uiODApplMgr&	applman;
    uiParent*		par_;

};


uiODApplMgr::uiODApplMgr( uiODMain& a )
	: appl(a)
	, applservice(*new uiODApplService(&a,*this))
    	, nlaserv(0)
	, getnewdata(this)
{
    pickserv = new uiPickPartServer( applservice );
    visserv = new uiVisPartServer( applservice );
    attrserv = new uiAttribPartServer( applservice );
    seisserv = new uiSeisPartServer( applservice );
    emserv = new uiEMPartServer( applservice );
    wellserv = new uiWellPartServer( applservice );
    wellattrserv = new uiWellAttribPartServer( applservice );
    trackserv = new uiTrackingPartServer( applservice );
    trackserv->setAttribDescSet( attrserv->curDescSet() );
}


uiODApplMgr::~uiODApplMgr()
{
    delete trackserv;
    delete pickserv;
    delete nlaserv;
    delete attrserv;
    delete seisserv;
    delete visserv;
    delete emserv;
    delete wellserv;
    delete wellattrserv;
    delete &applservice;
}

void uiODApplMgr::resetServers()
{
    if ( nlaserv ) nlaserv->reset();
    delete attrserv; attrserv = new uiAttribPartServer( applservice );
    delete trackserv; trackserv = new uiTrackingPartServer( applservice );
    trackserv->setAttribDescSet( attrserv->curDescSet() );
    visserv->deleteAllObjects();
}


int uiODApplMgr::manageSurvey()
{
    BufferString prevnm = GetDataDir();
    uiSurvey dlg( &appl );
    if ( !dlg.go() )
	return 0;
    else if ( prevnm == GetDataDir() )
	return 1;
    else
    {
	bool saved = attrserv->setSaved();
        BufferString msg( "Current attribute set is not saved.\nSave now?" );
        if ( !saved && uiMSG().askGoOn( msg ) )
	{
	    IOM().setRootDir( prevnm );
	    attrserv->saveSet();
	    IOM().setRootDir( GetDataDir() );
	}

	if ( nlaserv ) nlaserv->reset();
	delete attrserv; attrserv = new uiAttribPartServer( applservice );
	if ( appl.sceneMgrAvailable() )
	    sceneMgr().cleanUp( true );
	return 2;
    }
}


bool uiODApplMgr::manageNLA()
{
    if ( !nlaserv ) return false;

    bool res = nlaserv->go();
    if ( !res ) attrserv->setNLAName( nlaserv->modelName() );
    return res;
}


void uiODApplMgr::doOperation( ObjType ot, ActType at, int opt )
{
    switch ( ot )
    {
    case Seis:
	switch ( at )
	{
	case Imp:	seisserv->importSeis( opt ? uiSeisPartServer::CBVS
						  : uiSeisPartServer::SegY );
			break;
	case Exp:	seisserv->exportSeis();		break;
	case Man:	seisserv->manageSeismics();	break;
	}
    break;
    case Hor:
	switch ( at )
	{
	case Imp:	emserv->importHorizon();	break;
	case Exp:	emserv->exportHorizon();	break;
	case Man:	emserv->manageSurfaces(true);	break;
	}
    break;
    case Flt:
        switch( at )
	{
	case Man:	emserv->manageSurfaces(false);	break;
	}
    break;
    case Wll:
	switch ( at )
	{
	case Imp:
	    if ( opt == 0 )
		wellserv->importTrack();
	    else if ( opt == 1 )
		wellserv->importLogs();
	    else if ( opt == 2 )
		wellserv->importMarkers();

	break;
	case Man:	wellserv->manageWells();	break;
	}
    break;
    case Attr:
	if ( at == Man ) manageAttributes();
    break;
    }
}


void uiODApplMgr::impexpPickSet( bool imp ) { pickserv->impexpPickSet(imp); }
void uiODApplMgr::importLMKFault() { emserv->importLMKFault(); }


void uiODApplMgr::enableSceneMenu( bool yn )
{
    sceneMgr().disabRightClick( !yn );
    if ( !yn ) sceneMgr().setToViewMode();
    menuMgr().dtectTB()->setSensitive( yn );
    menuMgr().enableActButton( yn );
    menuMgr().enableMenuBar( yn );
}


void uiODApplMgr::manageAttributes()
{
    enableSceneMenu( false );
    attrserv->editSet(); 
}


void uiODApplMgr::createVol()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    attrserv->outputVol( nlaid );
}


void uiODApplMgr::createSurfOutput()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    uiAttrSurfaceOut dlg( &appl, *attrserv->curDescSet(), 
	    		  nlaserv ? &nlaserv->getModel() : 0, nlaid );
    dlg.go();
}


void uiODApplMgr::reStartProc() { uiRestartBatchDialog dlg( &appl ); dlg.go(); }
void uiODApplMgr::batchProgs() { uiBatchProgLaunch dlg( &appl ); dlg.go(); }
void uiODApplMgr::pluginMan() { uiPluginMan dlg( &appl ); dlg.go(); }


void uiODApplMgr::setFonts()
{
    uiSetFonts dlg( &appl, "Set font types" );
    dlg.go();
}


void uiODApplMgr::setStereoOffset()
{
    ObjectSet<uiSoViewer> vwrs;
    sceneMgr().getSoViewers( vwrs );
    uiStereoDlg dlg( &appl, vwrs );
    dlg.go();
}


void uiODApplMgr::setWorkingArea()
{
    if ( visserv->setWorkingArea() )
	sceneMgr().viewAll(0);
}


void uiODApplMgr::setZScale()
{
    visserv->setZScale();
}


bool uiODApplMgr::selectAttrib( int id )
{
    if ( id < 0 ) return false;
    const AttribSelSpec* as = visserv->getSelSpec( id );
    if ( !as ) return false;

    AttribSelSpec myas( *as );
    bool selok = attrserv->selectAttrib( myas );
    if ( selok )
	visserv->setSelSpec( id, myas );

    return selok;
}


bool uiODApplMgr::selectColorAttrib( int id )
{
    if ( id < 0 ) return false;
    const ColorAttribSel* as = visserv->getColorSelSpec( id );
    if ( !as ) return false;

    ColorAttribSel myas( *as );
    bool selok = attrserv->selectColorAttrib( myas );
    if ( selok )
	visserv->setColorSelSpec( id, myas );

    return selok;
}


void uiODApplMgr::storeSurface( int visid, bool storeas )
{
    if ( storeas )
    {
	ObjectSet<BinIDValueSet> bivs;
	visserv->fetchSurfaceData( visid, bivs );
	const AttribSelSpec* as = visserv->getSelSpec( visid );
	BufferString dispname( as ? as->userRef() : 0 );
	if ( as && as->isNLA() )
	{
	    dispname = as->objectRef();
	    const char* nodenm = as->userRef();
	    if ( IOObj::isKey(as->userRef()) )
		nodenm = IOM().nameOf( as->userRef(), false );
	    dispname += " ("; dispname += nodenm; dispname += ")";
	}

	if ( as && as->id() >= 0 )
	    emserv->setDataVal( *visserv->getMultiID(visid), bivs, dispname );
    }

    emserv->storeObject( *visserv->getMultiID(visid), storeas );
}


void uiODApplMgr::selectWells( ObjectSet<MultiID>& wellids )
{ wellserv->selectWells( wellids ); }
void uiODApplMgr::selectHorizon( MultiID& emhorid )
{ if ( !emserv->selectHorizon(emhorid) ) emhorid = MultiID(""); }
void uiODApplMgr::selectFault( MultiID& emfaultid )
{ if ( !emserv->selectFault(emfaultid) ) emfaultid = MultiID(""); }
void uiODApplMgr::selectStickSet( MultiID& stickid )
{ if ( !emserv->selectStickSet(stickid) ) stickid = MultiID(""); }


const Color& uiODApplMgr::getPickColor() { return pickserv->getPickColor(); }
void uiODApplMgr::getPickSetGroup( PickSetGroup& p ) { p = pickserv->group(); }
bool uiODApplMgr::storePickSets() { return pickserv->storePickSets(); }


bool uiODApplMgr::storeSinglePickSet( int id )
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, visserv->getObject(id) );
    PickSet* ps = new PickSet( psd->name() );
    psd->copyToPickSet(*ps);
    return pickserv->storeSinglePickSet( ps );
}


bool uiODApplMgr::setPickSetDirs( int id )
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, visserv->getObject(id) );
    PickSet ps( psd->name());
    psd->copyToPickSet(ps);
    if ( !attrserv->setPickSetDirs(ps,nlaserv?&nlaserv->getModel():0) )
	return false;

    psd->copyFromPickSet( ps );
    return true;
}


void uiODApplMgr::renamePickset( int id )
{
    BufferString newname;
    const char* oldname = visserv->getObjectName(id);
    pickserv->renamePickset( oldname, newname );
    visserv->setObjectName( id, newname );
}


bool uiODApplMgr::getNewData( int visid, bool colordata )
{
    if ( visid<0 ) return false;

    if ( visserv->getAttributeFormat(visid) == 3 )
    {
	CubeSampling cs = visserv->getCubeSampling( visid );
	bool res = trackserv->setWorkCube( cs );
	return res;
    }

    const AttribSelSpec* as = colordata ? &visserv->getColorSelSpec(visid)->as
					: visserv->getSelSpec(visid);
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    AttribSelSpec myas( *as );
    if ( myas.id()!=-1 ) attrserv->updateSelSpec( myas );
    if ( myas.id()<-1 && !colordata )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 
    
    if ( myas.isNLA() && !colordata )
    {
	if ( nlaserv && nlaserv->isClassification() )
	    visserv->setClipRate( visid, 0 );
    }

    bool res = false;
    switch ( visserv->getAttributeFormat(visid) )
    {
	case 0:
	{
	    if ( myas.id()<-1 && colordata )
	    { visserv->setCubeData(visid, true, 0 ); return true; }

	    const AttribSliceSet* prevset =
				visserv->getCachedData( visid, colordata );

	    CubeSampling cs = visserv->getCubeSampling( visid );
	    AttribSliceSet* slices = attrserv->createOutput(cs,myas,prevset);

	    if ( !slices ) return false;
	    visserv->setCubeData( visid, colordata, slices );
	    res = true;
	    break;
	}
	case 1:
	{
	    if ( myas.id()<-1 && colordata )
	    { SeisTrcBuf b; visserv->setTraceData(visid,true,b); return true; }

	    const Interval<float> zrg = visserv->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv->getDataTraceBids( visid, bids );
	    SeisTrcBuf data( true );
	    if ( !attrserv->createOutput( bids, zrg, data, myas ) )
		return false;
	    visserv->setTraceData( visid, colordata, data );
	    return true;
	}
	case 2:
	{
	    if ( myas.id()<-1 && colordata )
	    { visserv->stuffSurfaceData(visid,true,0); return true; }

	    if ( myas.id() == AttribSelSpec::otherAttrib )
	    {
		bool selok = emserv->loadAuxData( 
		    *visserv->getMultiID(visid), myas.userRef() );
		if ( selok ) handleStoredSurfaceData( visid );
		else uiMSG().error( "Cannot find stored data" );
		return selok;
	    }

	    ObjectSet<BinIDValueSet> data;
	    visserv->fetchSurfaceData( visid, data );
	    if ( !attrserv->createOutput(data,myas) )
	    {
		deepErase( data );
		return false;
	    }

	    visserv->stuffSurfaceData( visid, colordata, &data );
	    deepErase( data );
	    return true;
	}
	case -2:
	{
	    getnewdata.trigger(visid);
	    return true;
	}
    }

    setHistogram( visid );
    return res;
}


bool uiODApplMgr::evaluateAttribute( int visid )
{
    /* TODO: Perhaps better to merge this with uiODApplMgr::getNewData(), 
       for now it works */
    int format = visserv->getAttributeFormat( visid );
    if ( format == 0 )
    {
	const CubeSampling cs = visserv->getCubeSampling( visid );
	visserv->setCubeData( visid, false, attrserv->createSliceSet(cs) );
    }
    else if ( format == 2 )
    {
	ObjectSet<BinIDValueSet> data;
	visserv->fetchSurfaceData( visid, data );
	attrserv->createOutput( attrserv->curDescSet(), data );
	visserv->stuffSurfaceData( visid, false, &data );
	deepErase( data );
    }
    else
    {
	uiMSG().error( "Cannot evaluate this attribute on this object" );
	return false;
    }

    return true;
}



bool uiODApplMgr::handleEvent( const uiApplPartServer* aps, int evid )
{
    if ( !aps ) return true;

    if ( aps == pickserv )
	return handlePickServEv(evid);
    else if ( aps == visserv )
	return handleVisServEv(evid);
    else if ( aps == nlaserv )
	return handleNLAServEv(evid);
    else if ( aps == attrserv )
	return handleAttribServEv(evid);
    else if ( aps == emserv )
	return handleEMServEv(evid);
    else if ( aps == wellserv )
	return handleWellServEv(evid);
    else if ( aps == trackserv )
	return handleTrackServEv(evid);

    return false;
}


void* uiODApplMgr::deliverObject( const uiApplPartServer* aps, int id )
{
    if ( aps == attrserv )
    {
	if ( id == uiAttribPartServer::objNLAModel )
	    return nlaserv ? (void*)(&nlaserv->getModel()) : 0;
    }
    else
	pErrMsg("deliverObject for unsupported part server");

    return 0;
}


bool uiODApplMgr::handleTrackServEv( int evid )
{
    int sceneid = trackserv->sceneID();
    if ( evid == uiTrackingPartServer::evAddInterpreter )
    {
	int id = visserv->addInterpreter( sceneid );
	visserv->setTrackMan( id, trackserv->trackManager() );
	trackserv->setInterpreterID( sceneid, id );
    }
    else if ( evid == uiTrackingPartServer::evAddSurface )
    {
	const bool addhorizon = trackserv->isHorizon();
	const char* nm = trackserv->surfaceName();
	MultiID mid;
	const bool success = addhorizon ? emserv->createHorizon( mid, nm )
					: emserv->createFault( mid, nm );
	if ( !success ) return false;
	trackserv->setNewSurfaceID( mid );
	int sdid = sceneMgr().addSurfaceItem( mid, sceneid, addhorizon );
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(sdid))
	trackserv->setDisplayID( sdid );
	sd->useTexture( false );
	sd->turnOnWireFrame(true);
	sd->enableEditing(true);
	sd->getEditor()->enableSeedStick(true);
	sd->getEditor()->setTrackManager( &trackserv->trackManager() );
	sd->setResolution( sd->nrResolutions()-1 );
	sceneMgr().updateTrees();
    }
    else if ( evid == uiTrackingPartServer::evSelectStickSet )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(trackserv->displayID()));
	sd->getEditor()->selectSeedStick( true );
	sceneMgr().disabTree( sceneid, true );
	menuMgr().enableMenuBar( false );
	menuMgr().dtectTB()->setSensitive( false );
	menuMgr().manTB()->setSensitive( false );
    }
    else if ( evid == uiTrackingPartServer::evChangeStickSet )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(trackserv->displayID()));
	sd->getEditor()->setSeedStickStyle( trackserv->lineStyle(),
					    trackserv->markerStyle() );
    }
    else if ( evid == uiTrackingPartServer::evCheckStickSet )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(trackserv->displayID()))
	TypeSet<Coord3> stick;
	sd->getEditor()->getSeedStick( stick );
	if ( !stick.size() ) return false;
    }
    else if ( evid == uiTrackingPartServer::evFinishInit )
    {
	const int surfid = trackserv->displayID();
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(surfid));
	if ( !sd ) return false;

	TypeSet<Coord3> stick;
	sd->getEditor()->getSeedStick( stick );
	trackserv->createSeedFromStick( stick );
	storeSurface( surfid, false );
	trackserv->calcInterpreterCube( stick );
	sd->setColor( sd->getEditor()->getSeedStickColor() );
	sd->getEditor()->enableSeedStick( false );
    }
    else if ( evid == uiTrackingPartServer::evShowManager )
    {
	visserv->showTrackingManager();
    }
    else if ( evid == uiTrackingPartServer::evGetData )
    {
	const CubeSampling cs = trackserv->getAttribCube();
	const AttribSelSpec* as = trackserv->getSelSpec();
	if ( !as ) return false;
	const AttribSliceSet* sliceset = trackserv->getCachedData( *as );
	AttribSliceSet* newset = attrserv->createOutput( cs, *as, sliceset );
	trackserv->setSliceSet( newset );
    }
    else if ( evid == uiTrackingPartServer::evInitVisStuff )
    {
	TypeSet<int> sceneids;
	visserv->getChildIds( -1, sceneids );
	TypeSet<int> interpreterids;
	visserv->findObject( typeid(visSurvey::SurfaceInterpreterDisplay), 
			     interpreterids );
	if ( !interpreterids.size() ) return false;
	const int interpreterid = interpreterids[0];
	for ( int idx=0; idx<sceneids.size(); idx++ )
	    trackserv->setInterpreterID( sceneids[idx], interpreterid );
	visserv->setTrackMan( interpreterid, trackserv->trackManager() );
    }
    else if ( evid == uiTrackingPartServer::evRemoveSurface )
    {
	visserv->removeObject( trackserv->displayID(), sceneid );
	sceneMgr().removeTreeItem( trackserv->displayID() );
    }
    else if ( evid == uiTrackingPartServer::evWizardFinished )
    {
	sceneMgr().disabTree( sceneid, false );
	menuMgr().enableMenuBar( true );
	menuMgr().dtectTB()->setSensitive( true );
	menuMgr().manTB()->setSensitive( true );
    }
    else
	pErrMsg("Unknown event from trackserv");

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon )
    {
	TypeSet<int> sceneids;
	visserv->getChildIds( -1, sceneids );
	if ( !sceneids.size() ) return false;

	const MultiID& emid = emserv->selEMID();
	sceneMgr().addSurfaceItem( emid, sceneids[0], true );
	sceneMgr().updateTrees();
	return true;
    }

    else
	pErrMsg("Unknown event from emserv");

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetAvailableSets )
    {
	TypeSet<int> ids;
	visserv->findObject( typeid(visSurvey::PickSetDisplay), ids );
	for ( int idx=0; idx<ids.size(); idx++ )
	    pickserv->availableSets().add( visserv->getObjectName(ids[idx]));
    }
    else if ( evid == uiPickPartServer::evFetchPicks )
    {
	pickserv->group().clear();
	for ( int idx=0; idx<pickserv->selectedSets().size(); idx++ )
	{
	    if ( !pickserv->selectedSets()[idx] ) continue;
	    const char* nm = pickserv->availableSets().get(idx).buf();
	    PickSet* ps = new PickSet( nm );
	    mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
		    	     visserv->getObject(sceneMgr().getIDFromName(nm)) );
	    psd->copyToPickSet( *ps );
	    pickserv->group().add( ps );
	}
    }
    else if ( evid == uiPickPartServer::evGetHorInfo )
    {
	emserv->getSurfaceInfo( pickserv->horInfos() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef )
    {
	emserv->getSurfaceDef( pickserv->selHorIDs(), pickserv->genDef(),
			       pickserv->selBinIDRange() );
    }
    else

	pErrMsg("Unknown event from pickserv");

    return true;
}


bool uiODApplMgr::handleVisServEv( int evid )
{
    int visid = visserv->getEventObjId();
    visserv->unlockEvent();

    if ( evid == uiVisPartServer::evUpdateTree )
	sceneMgr().updateTrees();
    else if ( evid == uiVisPartServer::evDeSelection
	   || evid == uiVisPartServer::evSelection )
	sceneMgr().updateSelectedTreeItem();
    else if ( evid == uiVisPartServer::evGetNewData )
	return getNewData( visid, false );
    else if ( evid == uiVisPartServer::evInteraction )
	sceneMgr().setItemInfo( visid );
    else if ( evid == uiVisPartServer::evMouseMove )
	sceneMgr().setMousePos();
    else if ( evid == uiVisPartServer::evSelectAttrib )
	return selectAttrib( visid );
    else if ( evid == uiVisPartServer::evSelectColorAttrib )
	return selectColorAttrib( visid );
    else if ( evid == uiVisPartServer::evGetColorData )
	return getNewData( visid, true );
    else if ( evid == uiVisPartServer::evViewAll )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos )
	sceneMgr().toHomePos(0);
    else if (  evid == uiVisPartServer::evRemoveTrackTools )
	appl.removeDockWindow( visserv->getTrackTB() );
    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evIs2D )
    {
	const AttribDescSet* ads = attrserv->curDescSet();
	return ads ? ads->is2D() : false;
    }
    else if ( evid == uiNLAPartServer::evPrepareWrite )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const AttribDescSet* ads = attrserv->curDescSet();
	if ( !ads ) return false;
	IOPar& iopar = nlaserv->modelPars();
	iopar.clear();
	ads->fillPar( iopar );
    }
    else if ( evid == uiNLAPartServer::evPrepareRead )
    {
	bool saved = attrserv->setSaved();
        const char* msg = "Current attribute set is not saved.\nSave now?";
        if ( !saved && uiMSG().askGoOn( msg ) )
	    attrserv->saveSet();
    }
    else if ( evid == uiNLAPartServer::evReadFinished )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv->replaceSet( nlaserv->modelPars() );
	wellattrserv->setNLAModel( &nlaserv->getModel() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv->getPossibleOutputs( nlaserv->inputNames() );
	if ( nlaserv->inputNames().size() == 0 )
	    { uiMSG().error( "No usable input" ); return false; }
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput )
    {
	AttribSelInfo attrinf( attrserv->curDescSet() );
	TypeSet<int> idxs;
	nlaserv->getNeededStoredInputs(attrinf.ioobjids,idxs);
	for ( int idx=0; idx<idxs.size(); idx++ )
            attrserv->addToDescSet( attrinf.ioobjids.get( idxs[idx] ) );
    }
    else if ( evid == uiNLAPartServer::evGetData )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test feature sets

	if ( !attrserv->curDescSet() ) { pErrMsg("Huh"); return false; }
	ObjectSet<BinIDValueSet> bivss;
	if ( nlaserv->willDoExtraction() )
	{
	    nlaserv->getBinIDValueSets( bivss );
	    if ( !bivss.size() )
		{ uiMSG().error("No valid data locations found"); return false;}
	}
	ObjectSet<FeatureSet> fss;
	bool extrres = attrserv->extractFeatures( nlaserv->creationDesc(),
						  bivss, fss );
	deepErase( bivss );
	if ( extrres )
	{
	    FeatureSet fswrite;
	    attrserv->curDescSet()->fillPar( fswrite.pars() );
	    const char* res = nlaserv->transferData( fss, fswrite );
	    if ( res && *res )
		uiMSG().warning( res );
	}
	deepErase(fss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass )
    {
	const FeatureSet& fsmc = nlaserv->fsMCA();
	BinIDValueSet mcpicks( 2, true );
	for ( int idx=0; idx<fsmc.size(); idx++ )
	{
	    const FeatureVec& fv = *fsmc[idx];
	    const float conf = fv[2];
	    if ( mIsUndefined(conf) )
		continue;

	    if ( fv[0] != fv[1] )
		mcpicks.add( fv.fvPos(), fv.fvPos().ver, conf );
	}
	pickserv->setMisclassSet( mcpicks );
	PickSetGroup& psg = pickserv->group();
	int psid = sceneMgr().getIDFromName( psg.name() );
	bool haspicks = psg.nrSets();
	PickSet pset( psg.name() );
	pset.color.set(255,0,0);
	if ( psid < 0 )
	    sceneMgr().addPickSetItem( haspicks ? psg.get(0) : &pset, -1 );
	else
	{
	    mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
		    	     visserv->getObject(psid));
	    psd->copyFromPickSet( haspicks ? *psg.get(0) : pset );
	}

	sceneMgr().updateTrees();
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet )
    {
	AttribDescSet attrset;
	if ( !attrserv->createAttributeSet(nlaserv->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv->modelPars() );
	attrserv->replaceSet( nlaserv->modelPars() );
    }
    else

	pErrMsg("Unknown event from nlaserv");

    return true;
}


bool uiODApplMgr::handleAttribServEv( int evid )
{
    if ( evid==uiAttribPartServer::evDirectShowAttr )
    {
	AttribSelSpec as;
	attrserv->getDirectShowAttrSpec( as );
	int visid = visserv->getEventObjId();
	visserv->setSelSpec( visid, as );
	getNewData( visid, false );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet )
    {
	trackserv->setAttribDescSet( attrserv->curDescSet() );
    }
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed )
    {
	enableSceneMenu( true );
    }
    else if ( evid==uiAttribPartServer::evEvaluateAttr )
    {
	int visid = visserv->getEventObjId();
	AttribSelSpec as( "Evaluation", AttribSelSpec::otherAttrib );
	visserv->setSelSpec( visid, as );
	if ( !evaluateAttribute(visid) )
	    return false;
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evShowAttrSlice )
    {
	int visid = visserv->getEventObjId();
	visserv->selectTexture( visid, attrserv->getSliceIdx() );
	modifyColorTable( visid );
    }
    else
	pErrMsg("Unknown event from attrserv");

    return true;
}


void uiODApplMgr::handleStoredSurfaceData( int visid )
{
    BufferString attrnm;
    float shift = 0;
    ObjectSet<BinIDValueSet> data;
    if ( !emserv->getDataVal(*visserv->getMultiID(visid),data,attrnm,shift) )
	return;

    AttribSelSpec myas( attrnm, AttribSelSpec::otherAttrib );
    visserv->setSelSpec( visid, myas );
    visserv->stuffSurfaceData( visid, false, &data );
    deepErase( data );

    mDynamicCastGet( visSurvey::SurfaceDisplay*, sd, visserv->getObject(visid));
    sd->setShift( shift );

    setHistogram( visid );
    sceneMgr().updateTrees();
}


void uiODApplMgr::modifyColorTable( int visid )
{
    appl.colTabEd().setColTab( visserv->getColTabId(visid) );
    setHistogram( visid );
}


void uiODApplMgr::setHistogram( int visid )
{ appl.colTabEd().setHistogram( visserv->getHistogram(visid) ); }
