/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodapplmgr.cc,v 1.166 2006-12-21 16:41:19 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"

#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "vispicksetdisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"

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


//TODO : would that mean we need 2 mpeserv?
uiODApplMgr::uiODApplMgr( uiODMain& a )
	: appl(a)
	, applservice(*new uiODApplService(&a,*this))
    	, nlaserv(0)
    	, getOtherFormatData( this )
	, otherformatvisid_( -1 )
	, otherformatattrib_( -1 )
{
    pickserv = new uiPickPartServer( applservice );
    visserv = new uiVisPartServer( applservice );
    attrserv = new uiAttribPartServer( applservice );
    seisserv = new uiSeisPartServer( applservice );
    emserv = new uiEMPartServer( applservice );
    emattrserv = new uiEMAttribPartServer( applservice );
    wellserv = new uiWellPartServer( applservice );
    wellattrserv = new uiWellAttribPartServer( applservice );
	//TODO false set to make it compile change!!!!!!!
    mpeserv = new uiMPEPartServer( applservice, attrserv->curDescSet(false) );

    IOM().surveyToBeChanged.notify( mCB(this,uiODApplMgr,surveyToBeChanged) );
}


uiODApplMgr::~uiODApplMgr()
{
    IOM().surveyToBeChanged.remove( mCB(this,uiODApplMgr,surveyToBeChanged) );
    delete mpeserv;
    delete pickserv;
    delete nlaserv;
    delete attrserv;
    delete seisserv;

    appl.removeDockWindow( visserv->getTrackTB() );
    delete visserv;

    delete emserv;
    delete emattrserv;
    delete wellserv;
    delete wellattrserv;
    delete &applservice;
}


//TODO : would that mean we need 2 mpeserv?
void uiODApplMgr::resetServers()
{
    if ( nlaserv ) nlaserv->reset();
    delete attrserv; delete mpeserv;
    attrserv = new uiAttribPartServer( applservice );
	//TODO false set to make it compile change!!!!!!!
    mpeserv = new uiMPEPartServer( applservice,  attrserv->curDescSet(false) );
    visserv->deleteAllObjects();
    emserv->removeHistory();
}

//TODO : would that mean we need 2 mpeserv?
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
	sceneMgr().addScene();
	attrserv = new uiAttribPartServer( applservice );
	//TODO false set to make it compile change!!!!!!!
	mpeserv = new uiMPEPartServer( applservice,  attrserv->curDescSet(false) );
	MPE::engine().setActiveVolume( MPE::engine().getDefaultActiveVolume() );
	return 2;
    }
}


void uiODApplMgr::surveyToBeChanged( CallBacker* cb )
{
    bool anythingasked = false;
    appl.askStore( anythingasked );

    if ( nlaserv ) nlaserv->reset();
    delete attrserv; attrserv = 0;
    delete mpeserv; mpeserv = 0;
    if ( appl.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );
}


bool uiODApplMgr::editNLA( bool is2d )
{
    if ( !nlaserv ) return false;

    nlaserv->set2DEvent( is2d );
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
	case Imp:
	    if ( opt == 0 )
		emserv->importHorizon();
	    else if ( opt == 1 )
		emserv->importHorizonAttribute();
	    break;
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
	switch ( at )
	{
	case Imp:	pickserv->impexpSet( true );	break;
	case Exp:	pickserv->impexpSet( false );	break;
	case Man:	pickserv->managePickSets();	break;
	}
    break;
    case Wvlt:
	switch ( at )
	{
	case Imp:	seisserv->importWavelets();	break;
	default:	seisserv->manageWavelets();	break;
	}
    break;
    }
}

void uiODApplMgr::importLMKFault()
{ emserv->importLMKFault(); }


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
    editAttribSet( SI().has2D() );
}


void uiODApplMgr::editAttribSet( bool is2d )
{
    enableMenusAndToolBars( false );
    enableSceneManipulation( false );

    attrserv->editSet( is2d ); 
}


//TODO give both?
void uiODApplMgr::createHorOutput( int tp )
{
	//TODO false set to make it compile change!!!!!!!
    emattrserv->setDescSet( attrserv->curDescSet(false) );
    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor : 
	  	  uiEMAttribPartServer::BetweenHors );
    emattrserv->createHorizonOutput( type );
}


void uiODApplMgr::createVol( bool is2d )
{
    MultiID nlaid;
    if ( nlaserv )
    {
	nlaserv->set2DEvent( is2d );
	nlaid = nlaserv->modelId();
    }
    attrserv->outputVol( nlaid, is2d );
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
	//TODO false set to make it compile change!!!!!!!
    const bool selok = attrserv->selectAttrib( myas, key, false );
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
	    otherformatvisid_ = visid;
	    otherformatattrib_ = attrib;
	    getOtherFormatData.trigger();
	    otherformatvisid_ = -1;
	    otherformatattrib_ = -1;
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
	bool isnlamod2d = id == uiAttribPartServer::objNLAModel2D;
	bool isnlamod3d = id == uiAttribPartServer::objNLAModel3D;
	if ( isnlamod2d || isnlamod3d  )
	{
	    if ( nlaserv )
		nlaserv->set2DEvent( isnlamod2d );
	    return nlaserv ? (void*)(&nlaserv->getModel()) : 0;
	}
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
	const int sceneid = mpeserv->getCurSceneID();
	const int sdid = sceneMgr().addEMItem( emid, sceneid );
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
    else if ( evid==uiMPEPartServer::evMPEDispIntro )
	visserv->introduceMPEDisplay();
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
	if ( sceneids.isEmpty() ) return false;

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
    else if ( evid == uiVisPartServer::evToggleBlockDataLoad )
	mpeserv->blockDataLoading( !mpeserv->isDataLoadingBlocked() );
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
	const Attrib::DescSet* ads = attrserv->curDescSet(nlaserv->is2DEvent());
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
	bool saved = attrserv->setSaved(nlaserv->is2DEvent());
        const char* msg = "Current attribute set is not saved.\nSave now?";
        if ( !saved && uiMSG().askGoOn( msg ) )
	    attrserv->saveSet(nlaserv->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv->replaceSet( nlaserv->modelPars(), nlaserv->is2DEvent() );
	wellattrserv->setNLAModel( &nlaserv->getModel() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv->getPossibleOutputs( nlaserv->is2DEvent(),
				      nlaserv->inputNames() );
	if ( nlaserv->inputNames().size() == 0 )
	    { uiMSG().error( "No usable input" ); return false; }
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput )
    {
	BufferStringSet linekeys;
	nlaserv->getNeededStoredInputs( linekeys );
	for ( int idx=0; idx<linekeys.size(); idx++ )
            attrserv->addToDescSet( linekeys.get(idx), nlaserv->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evGetData )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	if ( !attrserv->curDescSet(nlaserv->is2DEvent()) ) 
	{ pErrMsg("Huh"); return false; }
	ObjectSet<BinIDValueSet> bivss;
	const bool dataextraction = nlaserv->willDoExtraction();
	if ( dataextraction )
	{
	    nlaserv->getBinIDValueSets( bivss );
	    if ( bivss.isEmpty() )
		{ uiMSG().error("No valid data locations found"); return false;}
	}
	ObjectSet<PosVecDataSet> vdss;
	bool extrres = attrserv->extractData( nlaserv->creationDesc(),
					      bivss, vdss, 
					      nlaserv->is2DEvent() );
	deepErase( bivss );
	if ( extrres )
	{
	    if ( dataextraction )
	    {
		IOPar& iop = nlaserv->storePars();
		attrserv->curDescSet(nlaserv->is2DEvent())->fillPar( iop );
		if ( iop.name().isEmpty() )
		    iop.setName( "Attributes" );
	    }
	    const char* res = nlaserv->prepareInputData( vdss );
	    if ( res && *res && strcmp(res,uiNLAPartServer::sKeyUsrCancel) )
		uiMSG().warning( res );
	    if ( !dataextraction ) // i.e. if we have just read a PosVecDataSet
		attrserv->replaceSet( vdss[0]->pars(), nlaserv->is2DEvent() );
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
	Attrib::DescSet attrset(nlaserv->is2DEvent());
	if ( !attrserv->createAttributeSet(nlaserv->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv->modelPars() );
	attrserv->replaceSet( nlaserv->modelPars(), nlaserv->is2DEvent() );
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
	//TODO false set to make it compile change!!!!!!
	mpeserv->setCurrentAttribDescSet( attrserv->curDescSet(false) );
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
	const int attrnr =
	    visserv->getSelAttribNr()==-1 ? 0 : visserv->getSelAttribNr();
	visserv->selectTexture( visid, attrnr, sliceidx );
	modifyColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices )
    {
	const int visid = visserv->getEventObjId();
	const int attrnr =
	    visserv->getSelAttribNr()==-1 ? 0 : visserv->getSelAttribNr();
	const uiVisPartServer::AttribFormat format = 
	    				visserv->getAttributeFormat( visid );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	ObjectSet<const BinIDValueSet> data;
	visserv->getRandomPosCache( visid, attrnr, data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv->getMultiID( visid );
	const EM::ObjectID emid = emserv->getObjectID( mid );
	const TypeSet<Attrib::SelSpec>& specs = attrserv->getTargetSelSpecs();
	const int nrvals = data[0]->nrVals()-1;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv->setAuxData( emid, data, specs[idx].userRef(), idx );
	    emserv->storeAuxData( emid );
	}
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
