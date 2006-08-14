/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodapplmgr.cc,v 1.146 2006-08-14 09:20:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"

#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uiattribpartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uiempartserv.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "vispicksetdisplay.h"
#include "visrandomtrackdisplay.h"
#include "vispolylinedisplay.h"
#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"

#include "emseedpicker.h"
#include "emtracker.h"
#include "mpeengine.h"
#include "externalattrib.h"
#include "attribdescset.h"
#include "attribdatacubes.h"
#include "attribsel.h"
#include "seisbuf.h"
#include "posvecdataset.h"
#include "pickset.h"
#include "survinfo.h"
#include "errh.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
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
#include "uishortcuts.h"

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
    	, getOtherFormatData( this )
{
    pickserv = new uiPickPartServer( applservice );
    visserv = new uiVisPartServer( applservice );
    attrserv = new uiAttribPartServer( applservice );
    seisserv = new uiSeisPartServer( applservice );
    emserv = new uiEMPartServer( applservice );
    wellserv = new uiWellPartServer( applservice );
    wellattrserv = new uiWellAttribPartServer( applservice );
    mpeserv = new uiMPEPartServer( applservice, attrserv->curDescSet() );
}


uiODApplMgr::~uiODApplMgr()
{
    delete mpeserv;
    delete pickserv;
    delete nlaserv;
    delete attrserv;
    delete seisserv;

    appl.removeDockWindow( visserv->getTrackTB() );
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
    delete mpeserv; mpeserv =
	new uiMPEPartServer( applservice,  attrserv->curDescSet() );
    visserv->deleteAllObjects();
    emserv->removeHistory();
}

//TODO verify that the descsets are both saved
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
	MPE::engine().setActiveVolume( MPE::engine().getDefaultActiveVolume() );
	return 2;
    }
}


bool uiODApplMgr::editNLA()
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
	case Imp:	seisserv->importSeis( opt );	break;
	case Exp:	seisserv->exportSeis( opt );	break;
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
	if ( at == Man ) attrserv->manageAttribSets();
    break;
    case Pick:
	if ( at == Man ) pickserv->managePickSets();
    break;
    }
}

void uiODApplMgr::importLMKFault()
{ emserv->importLMKFault(); }

void uiODApplMgr::impexpPickSet( bool imp )
{ pickserv->impexpSet(imp); }


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
{
    enableMenusAndToolBars( false );
    enableSceneManipulation( false );

    attrserv->editSet(); 
}


void uiODApplMgr::createVol()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    attrserv->outputVol( nlaid );
}


void uiODApplMgr::createHorCubeOutput()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    uiAttrTrcSelOut dlg( &appl, *attrserv->curDescSet(), 
		         nlaserv ? &nlaserv->getModel() : 0, nlaid, true );
    dlg.go();
}


void uiODApplMgr::create2HorCubeOutput()
{
    MultiID nlaid;
    if ( nlaserv )
	nlaid = nlaserv->modelId();
    uiAttrTrcSelOut dlg( &appl, *attrserv->curDescSet(), 
		         nlaserv ? &nlaserv->getModel() : 0, nlaid, false );
    dlg.go();
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
void uiODApplMgr::manageShortcuts() { uiShortcutsDlg dlg( &appl ); dlg.go(); }


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


bool uiODApplMgr::selectAttrib( int id, int attrib )
{
    if ( id < 0 ) return false;
    const Attrib::SelSpec* as = visserv->getSelSpec( id, attrib );
    if ( !as ) return false;

    const char* key = visserv->getDepthDomainKey( visserv->getSceneID(id) );
    Attrib::SelSpec myas( *as );
    const bool selok = attrserv->selectAttrib( myas, key );
    if ( selok )
	visserv->setSelSpec( id, attrib, myas );

    return selok;
}


void uiODApplMgr::selectWells( ObjectSet<MultiID>& wellids )
{ wellserv->selectWells( wellids ); }


bool uiODApplMgr::storePickSets()
{ return pickserv->storeSets(); }
bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv->storeSet( ps ); }
bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv->storeSetAs( ps ); }
bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{ return attrserv->setPickSetDirs( ps, nlaserv ? &nlaserv->getModel() : 0 ); }
bool uiODApplMgr::pickSetsStored() const
{ return pickserv->pickSetsStored(); }


bool uiODApplMgr::getNewData( int visid, int attrib )
{
    if ( visid<0 ) return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    Attrib::SelSpec myas( *as );
    if ( myas.id()!=Attrib::DescID::undef() ) attrserv->updateSelSpec( myas );
    if ( myas.id()<-1 )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 

    bool res = false;
    switch ( visserv->getAttributeFormat(visid) )
    {
	case uiVisPartServer::Cube :
	{
	    const Attrib::DataCubes* cache =
				visserv->getCachedData( visid, attrib );

	    CubeSampling cs = visserv->getCubeSampling( visid, attrib );
	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		PtrMan<Attrib::ExtAttribCalc> calc = 
				Attrib::ExtAttrFact().createCalculator( myas );

		if ( !calc )
		{
		    BufferString errstr = "Selected attribute is not present ";
		    errstr += "in the set\n and cannot be created";
		    uiMSG().error( errstr );
		    return false;
		}

		RefMan<const Attrib::DataCubes> newdata =
				calc->createAttrib( cs, cache );
		res = newdata;
		visserv->setCubeData( visid, attrib, newdata );
		break;
	    }

	    attrserv->setTargetSelSpec( myas );
	    RefMan<const Attrib::DataCubes> newdata =
				attrserv->createOutput( cs, cache );

	    if ( !newdata )
	    {
		visserv->setCubeData( visid, attrib, 0 );
		return false;
	    }

	    const bool isclass = newdata->nrCubes()<1 ? false :
			attrserv->isDataClassified( newdata->getCube(0) );
	    if ( isclass ) 
		visserv->setClipRate( visid, attrib, 0 );
	    visserv->setClassification( visid, attrib, isclass );
	    visserv->setCubeData( visid, attrib, newdata );
	    res = true;
	    break;
	}
	case uiVisPartServer::Traces :
	{
	    const Interval<float> zrg = visserv->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv->getDataTraceBids( visid, bids );
	    BinIDValueSet bidset(2,false);
	    for ( int idx=0; idx<bids.size(); idx++ )
		bidset.add( bids[idx], zrg.start, zrg.stop );
	    SeisTrcBuf data( true );
	    attrserv->setTargetSelSpec( myas );
	    if ( !attrserv->createOutput(bidset,data) )
		return false;
	    visserv->setTraceData( visid, attrib, data );
	    return true;
	}
	case uiVisPartServer::RandomPos :
	{
	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		const MultiID surfmid = visserv->getMultiID(visid);
		const EM::ObjectID emid = emserv->getObjectID(surfmid);
		const int auxdatanr = emserv->loadAuxData(emid,myas.userRef());
		if ( auxdatanr<0 )
		    uiMSG().error( "Cannot find stored data" );
		else
		{
		    BufferString attrnm;
		    ObjectSet<BinIDValueSet> vals;
		    emserv->getAuxData( emid, auxdatanr, attrnm, vals );
		    visserv->setRandomPosData( visid, attrib, &vals );
		}
		
		return  auxdatanr>=0;
	    }

	    ObjectSet<BinIDValueSet> data;
	    visserv->getRandomPos( visid, data );
	    attrserv->setTargetSelSpec( myas );
	    if ( !attrserv->createOutput(data) )
	    {
		deepErase( data );
		return false;
	    }

	    visserv->setRandomPosData( visid, attrib, &data );

	    deepErase( data );
	    return true;
	}
	case uiVisPartServer::OtherFormat :
	{
	    getOtherFormatData.trigger( visid );
	    return true;
	}
	default :
	{
	    pErrMsg("Invalid format");
	    return false;
	}
    }

    setHistogram( visid, attrib );
    return res;
}


bool uiODApplMgr::evaluateAttribute( int visid, int attrib )
{
    /* Perhaps better to merge this with uiODApplMgr::getNewData(), 
       for now it works */
    uiVisPartServer::AttribFormat format = visserv->getAttributeFormat( visid );
    if ( format == uiVisPartServer::Cube )
    {
	const CubeSampling cs = visserv->getCubeSampling( visid );
	RefMan<const Attrib::DataCubes> newdata = attrserv->createOutput( cs );
	visserv->setCubeData( visid, attrib, newdata );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	ObjectSet<BinIDValueSet> data;
	visserv->getRandomPos( visid, data );

	attrserv->createOutput( data );
	visserv->setRandomPosData( visid, attrib, &data );
	deepErase( data );
    }
    else
    {
	uiMSG().error( "Cannot evaluate attributes on this object" );
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
    else if ( aps == mpeserv )
	return handleMPEServEv(evid);

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


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject )
    {
	const int trackerid = mpeserv->activeTrackerID();
	const EM::ObjectID emid = mpeserv->getEMObjectID(trackerid);
	TypeSet<int> sceneids;
	visserv->getChildIds( -1, sceneids );
	if ( !sceneids.size() ) return false;

	const int sdid = sceneMgr().addEMItem( emid, sceneids[0] );
	if ( sdid==-1 )
	    return false;

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject )
    {
	const int trackerid = mpeserv->activeTrackerID();
	const EM::ObjectID emid = mpeserv->getEMObjectID(trackerid);
	const MultiID mid = emserv->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv->getChildIds( -1, sceneids );

	TypeSet<int> ids;
	visserv->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv->removeObject( ids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(ids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evStartSeedPick )
    {
	//Turn off everything

	visserv->turnSeedPickingOn( true );
	sceneMgr().setToViewMode( false );
    }
    else if ( evid==uiMPEPartServer::evEndSeedPick )
    {
	visserv->turnSeedPickingOn( false );
    }
    else if ( evid==uiMPEPartServer::evWizardClosed )
    {
	enableMenusAndToolBars( true );
	enableTree( true );
    }
    else if ( evid==uiMPEPartServer::evGetAttribData )
    {
	const Attrib::SelSpec* as = mpeserv->getAttribSelSpec();
	if ( !as ) return false;
	const CubeSampling cs = mpeserv->getAttribVolume(*as);
	const Attrib::DataCubes* cache = mpeserv->getAttribCache(*as);
	attrserv->setTargetSelSpec( *as );
	RefMan<const Attrib::DataCubes> newdata =
	    				attrserv->createOutput( cs, cache );
	mpeserv->setAttribData(*as, newdata );
    }
    else if ( evid==uiMPEPartServer::evCreate2DSelSpec )
    {
	const Attrib::DescID attribid = attrServer()->createStored2DAttrib(
		mpeserv->get2DLineSet(), mpeserv->get2DAttribName() );

	if ( attribid<0 ) return false;

	const Attrib::SelSpec as( mpeserv->get2DAttribName(), attribid );
	mpeserv->set2DSelSpec( as );
    }
    else if ( evid==uiMPEPartServer::evShowToolbar )
	visserv->showMPEToolbar();
    else if ( evid==uiMPEPartServer::evInitFromSession )
	visserv->initMPEStuff();
    else
	pErrMsg("Unknown event from mpeserv");

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine )
    {
	TypeSet<Coord> coords;
	wellserv->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
	enableTree( false );
	enableMenusAndToolBars( false );
    }
    if ( evid == uiWellPartServer::evCreateRdmLine )
    {
	TypeSet<Coord> coords;
	wellserv->getRdmLineCoordinates( coords );
	cleanPreview();

	TypeSet<BinID> bidset;
	for ( int idx=0; idx<coords.size(); idx++ )
	{
	    BinID bid = SI().transform( coords[idx] );
	    if ( bidset.indexOf(bid) < 0 )
		bidset += bid;
	}

	const int rdmlineid = visserv->getSelObjectId();
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv->getObject(rdmlineid));
	rtd->setKnotPositions( bidset );
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    if ( evid == uiWellPartServer::evCleanPreview )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon )
    {
	TypeSet<int> sceneids;
	visserv->getChildIds( -1, sceneids );
	if ( !sceneids.size() ) return false;

	const EM::ObjectID emid = emserv->selEMID();
	sceneMgr().addEMItem( emid, sceneids[0] );
	sceneMgr().updateTrees();
	return true;
    }

    else
	pErrMsg("Unknown event from emserv");

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetHorInfo )
    {
	emserv->getSurfaceInfo( pickserv->horInfos() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef )
    {
	TypeSet<EM::ObjectID> horids;
	const ObjectSet<MultiID>& storids = pickserv->selHorIDs();
	for ( int idx=0; idx<storids.size(); idx++ )
	    horids += emserv->getObjectID(*storids[idx]);
	
	emserv->getSurfaceDef( horids, pickserv->genDef(),
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
	return getNewData( visid, visserv->getEventAttrib() );
    else if ( evid == uiVisPartServer::evInteraction )
	sceneMgr().setItemInfo( visid );
    else if ( evid == uiVisPartServer::evMouseMove ||
	      evid==uiVisPartServer::evPickingStatusChange )
	sceneMgr().updateStatusBar();
    else if ( evid == uiVisPartServer::evSelectAttrib )
	return selectAttrib( visid, visserv->getEventAttrib() );
    else if ( evid == uiVisPartServer::evViewAll )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos )
	sceneMgr().toHomePos(0);
    else if ( evid == uiVisPartServer::evShowSetupDlg )
    {
	const int selobjvisid = visserv->getSelObjectId();
	const MultiID selobjmid = visserv->getMultiID(selobjvisid);
	const EM::ObjectID& emid = emserv->getObjectID(selobjmid);
	const int trackerid = mpeserv->getTrackerID(emid);
	MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
	const MPE::EMSeedPicker* seedpicker = tracker ? 
					      tracker->getSeedPicker(false) : 0;
	const EM::SectionID sid = seedpicker ? seedpicker->getSectionID() : -1;
	mpeserv->showSetupDlg( emid, sid, true );
	visserv->updateMPEToolbar();
    }
    else if ( evid == uiVisPartServer::evLoadPostponedData )
	mpeserv->loadPostponedVolume();
    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evIs2D )
    {
	const Attrib::DescSet* ads = attrserv->curDescSet();
	return ads ? ads->is2D() : false;
    }
    else if ( evid == uiNLAPartServer::evPrepareWrite )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet* ads = attrserv->curDescSet();
	if ( !ads ) return false;
	IOPar& iopar = nlaserv->modelPars();
	iopar.clear();
	BufferStringSet inputs = nlaserv->modelInputs();
	const Attrib::DescSet* cleanads = ads->optimizeClone( inputs );
	cleanads->fillPar( iopar );
	delete cleanads;
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
	BufferStringSet linekeys;
	nlaserv->getNeededStoredInputs( linekeys );
	for ( int idx=0; idx<linekeys.size(); idx++ )
            attrserv->addToDescSet( linekeys.get(idx) );
    }
    else if ( evid == uiNLAPartServer::evGetData )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	if ( !attrserv->curDescSet() ) { pErrMsg("Huh"); return false; }
	ObjectSet<BinIDValueSet> bivss;
	const bool dataextraction = nlaserv->willDoExtraction();
	if ( dataextraction )
	{
	    nlaserv->getBinIDValueSets( bivss );
	    if ( !bivss.size() )
		{ uiMSG().error("No valid data locations found"); return false;}
	}
	ObjectSet<PosVecDataSet> vdss;
	bool extrres = attrserv->extractData( nlaserv->creationDesc(),
					      bivss, vdss );
	deepErase( bivss );
	if ( extrres )
	{
	    if ( dataextraction )
		attrserv->curDescSet()->fillPar( nlaserv->storePars() );
	    const char* res = nlaserv->prepareInputData( vdss );
	    if ( res && *res && strcmp(res,uiNLAPartServer::sKeyUsrCancel) )
		uiMSG().warning( res );
	    if ( !dataextraction ) // i.e. if we have just read a PosVecDataSet
		attrserv->replaceSet( vdss[0]->pars() );
	}
	deepErase(vdss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass )
    {
	const BinIDValueSet& bvsmc = nlaserv->vdsMCA().data();
	BinIDValueSet mcpicks( 2, true );
	BinID bid; float vals[bvsmc.nrVals()];
	BinIDValueSet::Pos pos;
	while ( bvsmc.next(pos) )
	{
	    bvsmc.get( pos, bid, vals );
	    const float conf = vals[3];
	    if ( mIsUdf(conf) )
		continue;

	    const int actualclass = mNINT(vals[1]);
	    const int predclass = mNINT(vals[2]);
	    if ( actualclass != predclass )
		mcpicks.add( bid, vals[0], conf );
	}
	pickserv->setMisclassSet( mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet )
    {
	Attrib::DescSet attrset;
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
	Attrib::SelSpec as;
	attrserv->getDirectShowAttrSpec( as );
	const int visid = visserv->getEventObjId();
	const int attrib = visserv->getSelAttribNr();
	if ( attrib<0 ) return false;
	visserv->setSelSpec( visid, attrib, as );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet )
    {
	mpeserv->setCurrentAttribDescSet( attrserv->curDescSet() );
    }
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed )
    {
	enableMenusAndToolBars( true );
	enableSceneManipulation( true );
    }
    else if ( evid==uiAttribPartServer::evEvalAttrInit )
    {
	const uiVisPartServer::AttribFormat format = 
	    	visserv->getAttributeFormat( visserv->getEventObjId() );
	const bool alloweval = format==uiVisPartServer::Cube || 
	    		       format==uiVisPartServer::RandomPos;
	const bool allowstorage = format==uiVisPartServer::RandomPos;
	attrserv->setEvaluateInfo( alloweval, allowstorage );
    }
    else if ( evid==uiAttribPartServer::evEvalCalcAttr )
    {
	const int visid = visserv->getEventObjId();
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttrib() );
	const int attrib = visserv->getSelAttribNr();
	if ( attrib<0 ) return false;
	visserv->setSelSpec( visid, attrib, as );
	if ( !evaluateAttribute(visid,attrib) )
	    return false;
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalShowSlice )
    {
	const int visid = visserv->getEventObjId();
	const int sliceidx = attrserv->getSliceIdx();
	const int attrnr = visserv->getSelAttribNr()==-1
	    ? 0 : visserv->getSelAttribNr();
	visserv->selectTexture( visid, attrnr, sliceidx );
	modifyColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices )
    {
	const int visid = visserv->getEventObjId();
	const uiVisPartServer::AttribFormat format = 
	    				visserv->getAttributeFormat( visid );
	if ( format!=uiVisPartServer::RandomPos ) return false;
	const MultiID mid = visserv->getMultiID(visid);
	const EM::ObjectID emid = emserv->getObjectID(mid);
	emserv->storeAuxData( emid );
    }
    else
	pErrMsg("Unknown event from attrserv");

    return true;
}


void uiODApplMgr::pageUpDownPressed( bool up )
{
    const int visid = visserv->getEventObjId();
    const int attrib = visserv->getSelAttribNr();
    int texture = visserv->selectedTexture( visid, attrib );
    if ( texture<visserv->nrTextures(visid,attrib)-1 && up )
	texture++;
    else if ( texture && !up )
	texture--;

    visserv->selectTexture( visid, attrib, texture );
    modifyColorTable( visid, attrib );
    sceneMgr().updateTrees();
}


void uiODApplMgr::modifyColorTable( int visid, int attrib )
{
    appl.colTabEd().setColTab( visserv->getColTabId(visid,attrib) );
    setHistogram( visid, attrib );
}


void uiODApplMgr::coltabChg( CallBacker* )
{
    const int visid = visserv->getEventObjId();
    int attrib = visserv->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;
    setHistogram( visid, attrib );
}


NotifierAccess* uiODApplMgr::colorTableSeqChange()
{
    return &appl.colTabEd().sequenceChange;
}


void uiODApplMgr::setHistogram( int visid, int attrib )
{ appl.colTabEd().setHistogram( visserv->getHistogram(visid,attrib) ); }


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<int> plids;
    TypeSet<int> sceneids;
    visSurvey::PolyLineDisplay* pl = visSurvey::PolyLineDisplay::create();
    pl->fillPolyLine( coords );
    mDynamicCastGet(visBase::DataObject*,doobj,pl);
    visserv->getChildIds( -1, sceneids );
    
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv->addObject( doobj, sceneids[idx], true );
	plids.addIfNew( doobj->id() );
    }
    wellserv->setPreviewIds(plids);
}


void uiODApplMgr::cleanPreview()
{
    TypeSet<int> sceneids;
    visserv->getChildIds( -1, sceneids );
    TypeSet<int>& previds = wellserv->getPreviewIds();
    if ( previds.size() == 0 ) return;
    for ( int idx=0; idx<sceneids.size(); idx++ )
	visserv->removeObject( previds[0],sceneids[idx] );

    previds.erase();
}
