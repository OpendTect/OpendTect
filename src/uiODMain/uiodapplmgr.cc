/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodapplmgr.cc,v 1.23 2004-05-04 16:03:56 bert Exp $
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
#include "vissurvstickset.h"
#include "visinterpret.h"
#include "vishingeline.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "pickset.h"
#include "survinfo.h"
#include "errh.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "featset.h"
#include "helpview.h"
#include "nlacrdesc.h"
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
{
    pickserv = new uiPickPartServer( applservice );
    visserv = new uiVisPartServer( applservice );
    attrserv = new uiAttribPartServer( applservice );
    seisserv = new uiSeisPartServer( applservice );
    emserv = new uiEMPartServer( applservice );
    wellserv = new uiWellPartServer( applservice );
    wellattrserv = new uiWellAttribPartServer( applservice );
    trackserv = new uiTrackingPartServer( applservice );
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
}

void uiODApplMgr::resetServers()
{
    if ( nlaserv ) nlaserv->reset();
    delete attrserv; attrserv = new uiAttribPartServer( applservice );
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
    case Wll:
	switch ( at )
	{
	case Imp:	wellserv->importWell();		break;
	case Man:	wellserv->manageWells();	break;
	}
    break;
    case Attr:
	if ( at == Man ) manageAttributes();
    break;
    }
}


void uiODApplMgr::importPickSet() { pickserv->importPickSet(); }
void uiODApplMgr::importLMKFault() { emserv->importLMKFault(); }


void uiODApplMgr::manageAttributes()
{
    sceneMgr().disabRightClick(true);
    sceneMgr().setToViewMode();
    menuMgr().dtectTB()->setSensitive( false );
    menuMgr().enableActButton( false );
    menuMgr().enableMenuBar( false );

    attrserv->editSet(); 
}


void uiODApplMgr::createVol()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    attrserv->outputVol( nlaid );
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


void uiODApplMgr::storeSurface( int visid )
{
    ObjectSet< TypeSet<BinIDZValues> > bidzvset;
    visserv->getRandomPosDataPos( visid, bidzvset );
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
	emserv->setDataVal( *visserv->getMultiID(visid), bidzvset, dispname );
    emserv->storeObject( *visserv->getMultiID(visid) );
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
    
    AttribSelSpec as(*(colordata ? &visserv->getColorSelSpec(visid)->as
					: visserv->getSelSpec(visid)));
    
    if ( as.id()!=-1 ) attrserv->updateSelSpec( as );
    if ( as.id()<-1 && !colordata )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 
    
    if ( as.isNLA() && !colordata )
    {
	if ( nlaserv && nlaserv->isClassification() )
	    visserv->setClipRate( visid, 0 );
    }

    bool res = false;
    switch ( visserv->getAttributeFormat(visid) )
    {
	case 0:
	{
	    if ( as.id()<-1 && colordata )
	    { visserv->setCubeData(visid, true, 0 ); return true; }

	    const AttribSliceSet* prevset =
				visserv->getCachedData( visid, colordata );

	    CubeSampling cs = visserv->getCubeSampling( visid );
	    AttribSliceSet* slices = attrserv->createOutput(cs,as,prevset);

	    if ( !slices ) return false;
	    visserv->setCubeData( visid, colordata, slices );
	    res = true;
	    break;
	}
	case 1:
	{
	    if ( as.id()<-1 && colordata )
	    { visserv->setTraceData(visid,true,0); return true; }

	    const Interval<float> zrg = visserv->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv->getDataTraceBids( visid, bids );
	    ObjectSet<SeisTrc> data;
	    if ( !attrserv->createOutput( bids, zrg, data, as ) )
		return false;
	    
	    visserv->setTraceData( visid, colordata, &data );

	    return true;
	}
	case 2:
	{
	    if ( as.id()<-1 && colordata )
	    { visserv->setRandomPosData(visid,true,0); return true; }

	    if ( as.id() == -1 )
	    {
		bool selok = emserv->loadAuxData( 
		    *visserv->getMultiID(visid), as.userRef() );
		if ( selok ) handleStoredSurfaceData( visid );
		else uiMSG().error( "Cannot find stored data" );
		return selok;
	    }

	    ObjectSet< TypeSet<BinIDZValues> > data;
	    visserv->getRandomPosDataPos( visid, data );
	    if ( !attrserv->createOutput(data,as) )
		return false;

	    const ObjectSet< const TypeSet<const BinIDZValues> >& to_pass =
		reinterpret_cast< const ObjectSet< 
			    const TypeSet< const BinIDZValues > >& >(data);
	    visserv->setRandomPosData( visid, colordata, &to_pass );

	    deepErase( data );

	    return true;
	}
	case 3:
	{
	    CubeSampling cs = visserv->getCubeSampling( visid );
	    res = trackserv->setWorkCube( cs );
	}
    }

    setHistogram( visid );
    return res;
}

/*
bool uiODApplMgr::evaluateAttribute( int visid )
{
    if ( visserv->isInlCrlTsl(visid,-1) )
    {
	const CubeSampling cs = visserv->getCubeSampling( visid );
	visserv->setCubeData( visid, attrserv->createSliceSet(cs) );
    }
    else if ( visserv->isHorizon(visid) )
    {
	ObjectSet< TypeSet<BinIDZValues> > data;
	visserv->getRandomPosDataPos( visid, data );
	attrserv->createOutput( attrserv->curDescSet(), data );

	const ObjectSet< const TypeSet< const BinIDZValues > >& to_pass =
	    reinterpret_cast< const ObjectSet< 
			    const TypeSet< const BinIDZValues > >& >( data );
	visserv->setRandomPosData( visid, &to_pass );
    }
    else
    {
	uiMSG().error( "Cannot evaluate this attribute on this object" );
	return false;
    }

    return true;
}
*/


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
	visSurvey::SurfaceInterpreterDisplay* sid = 
	    	visSurvey::SurfaceInterpreterDisplay::create();
	sid->setTrackMan( trackserv->trackManager() );
	visserv->addObject( sid, sceneid, true );
	trackserv->setInterpreterID( sceneid, sid->id() );
    }
    else if ( evid == uiTrackingPartServer::evAddSurface )
    {
	visSurvey::StickSetDisplay* ssd = visSurvey::StickSetDisplay::create();
	visserv->addObject( ssd, sceneid, true );
	trackserv->setStickSetID( ssd->id() );

	bool addhorizon = trackserv->isHorizon();
	const char* nm = trackserv->surfaceName();
	MultiID mid;
	bool success = addhorizon ? emserv->createHorizon( mid, nm )
	    			  : emserv->createFault( mid, nm );
	if ( !success ) return false;
	trackserv->setNewSurfaceID( mid );
	sceneMgr().addSurfaceItem( mid, sceneid, addhorizon );
	sceneMgr().updateTrees();
    }
    else if ( evid == uiTrackingPartServer::evSelStickSet )
    {
	int selid = visserv->getSelObjectId();
	int ssid = trackserv->stickSetID();
	bool desel = selid == ssid;
	visserv->setSelObjectId( desel ? -1 : ssid );
	sceneMgr().disabTree( sceneid, !desel );
	if ( !desel ) sceneMgr().actMode();
	mDynamicCastGet(visSurvey::StickSetDisplay*,ssd,
						visserv->getObject(ssid))
	if ( ssd )
	{
	    ssd->setLineStyle( trackserv->lineStyle() );
	    ssd->setMarkerStyle( trackserv->markerStyle() );
	}
    }
    else if ( evid == uiTrackingPartServer::evChangeStickSet )
    {
	int ssid = trackserv->stickSetID();
	mDynamicCastGet(visSurvey::StickSetDisplay*,ssd,
						visserv->getObject(ssid))
	if ( ssd )
	{
	    ssd->setLineStyle( trackserv->lineStyle() );
	    ssd->setMarkerStyle( trackserv->markerStyle() );
	}
    }
    else if ( evid == uiTrackingPartServer::evFinishInit )
    {
	int ssid = trackserv->stickSetID();
	mDynamicCastGet(visSurvey::StickSetDisplay*,ssd,
						visserv->getObject(ssid))
	if ( !ssd ) return false;
	int stickidx = 0;
	TypeSet<Coord3> stick;
	for ( int idx=0; idx<ssd->nrKnots(stickidx); idx++ )
	    stick += ssd->getKnot( stickidx, idx );
	trackserv->createSeedFromStickset( stick );
	trackserv->calcInterpreterCube( stick );
    }
    else if ( evid == uiTrackingPartServer::evShowManager )
    {
	int interpreterid = trackserv->interpreterID( sceneid );
//	visserv->launchTrackingManager( interpreterid, 
//					trackserv->trackManager() );
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
    else if ( evid == uiTrackingPartServer::evAddHingeLine )
    {
	visSurvey::HingeLineDisplay* hld = 
	    		visSurvey::HingeLineDisplay::create();
	hld->setHingeLine( trackserv->hingeLine() );
	visserv->addObject( hld, sceneid, true );
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
	TypeSet<BinID> bidset;
	TypeSet<Interval<float> > zrgset;
	emserv->getSurfaceDef( pickserv->selHorIDs(), bidset, zrgset, 
			       pickserv->selBinIDRange() );
	pickserv->horDef() = bidset;
	pickserv->horDepth() = zrgset;
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
    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const AttribDescSet* ads = attrserv->curDescSet();
	if ( !ads ) return true;
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

	AttribSelInfo attrinf( attrserv->curDescSet() );
	if ( attrinf.attrnms.size() + attrinf.ioobjnms.size() < 1 )
        { uiMSG().error( "No usable input" ); return true; }

	BufferStringSet& inpnms = nlaserv->inputNames();
	for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
	    inpnms += new BufferString( attrinf.attrnms.get(idx) );
	for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
	    inpnms += new BufferString( attrinf.ioobjids.get(idx) );
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

	if ( !attrserv->curDescSet() ) { pErrMsg("Huh"); return true; }
	const NLACreationDesc& crdesc = nlaserv->creationDesc();
	ObjectSet< TypeSet<BinIDValue> > bivsets;
	if ( crdesc.doextraction )
	{
	    nlaserv->getBinIDValues( crdesc, bivsets );
	    if ( !bivsets.size() )
		{ uiMSG().error("No valid data locations found"); return true; }
	}
	ObjectSet<FeatureSet> fss;
	bool bres = !attrserv->extractFeatures(crdesc,bivsets,fss);
	deepErase( bivsets );
	if ( !bres )
	    return true;

	FeatureSet& fstr = nlaserv->fsTrain();
	FeatureSet fswrite; attrserv->curDescSet()->fillPar( fswrite.pars() );
	const char* res = crdesc.transferData( fss, fstr, nlaserv->fsTest(),
					     &fswrite );
	if ( res && *res )
	    uiMSG().warning( res );
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass )
    {
	const FeatureSet& fsmc = nlaserv->fsMCA();
	TypeSet<BinIDZValue> mcpicks;
	for ( int idx=0; idx<fsmc.size(); idx++ )
	{
	    const FeatureVec& fv = *fsmc[idx];
	    const float conf = fv[2];
	    if ( mIsUndefined(conf) )
		continue;

	    if ( fv[0] != fv[1] )
		mcpicks += BinIDZValue( fv.fvPos().inl, fv.fvPos().crl,
					fv.fvPos().ver, conf );
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
	const NLADesign& design = nlaserv->getModel().design();
	AttribDescSet* attrset = new AttribDescSet;
	if ( !attrserv->createAttributeSet( design.inputs, attrset ) )
	    return false;

	attrset->fillPar( nlaserv->modelPars() );
	attrserv->replaceSet( nlaserv->modelPars() );
	delete attrset;
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
    }
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed )
    {
	sceneMgr().disabRightClick(false);
	menuMgr().dtectTB()->setSensitive( true );
	menuMgr().enableActButton( true );
	menuMgr().enableMenuBar( true );
    }
    else if ( evid==uiAttribPartServer::evEvaluateAttr )
    {
	int visid = visserv->getEventObjId();
	AttribSelSpec as( "Evaluation" );
	visserv->setSelSpec( visid, as );
	if ( !getNewData( visid, false ) )
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


/*
bool uiODApplMgr::getNewCubeData( int visid, bool colordata )
{
}


bool uiODApplMgr::getNewSurfData( int visid, bool colordata )
{
}


bool uiODApplMgr::getNewRandomLineData( int visid, bool colordata )
{
}
*/


void uiODApplMgr::handleStoredSurfaceData( int visid )
{
    BufferString attrnm;
    float shift = 0;
    ObjectSet< TypeSet<BinIDZValues> > data;
    if ( !emserv->getDataVal(*visserv->getMultiID(visid),data,attrnm,shift) )
	return;

    const ObjectSet< const TypeSet< const BinIDZValues > >& to_pass =
	reinterpret_cast< const ObjectSet< 
			const TypeSet< const BinIDZValues > >& >( data );
    visserv->setRandomPosData( visid, false, &to_pass );

    deepErase( data );
    mDynamicCastGet( visSurvey::SurfaceDisplay*, sd, visserv->getObject(visid));
    sd->setShift( shift );

    AttribSelSpec myas( attrnm, -1 );
    visserv->setSelSpec( visid, myas );
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
