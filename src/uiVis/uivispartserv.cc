/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.188 2004-01-29 10:23:47 nanne Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribslice.h"
#include "errh.h"
#include "pickset.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "visselman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vissurvstickset.h"
#include "vissurvsurf.h"
#include "vissurvwell.h"
#include "visvolumedisplay.h"
#include "visvolrender.h"
#include "vistexture3viewer.h"
#include "visrandomtrackdisplay.h"
#include "uiexecutor.h"
#include "uifiledlg.h"
#include "uimaterialdlg.h"
#include "uislicesel.h"
#include "uizscaledlg.h"
#include "uiworkareadlg.h"
#include "uimenu.h"
#include "colortab.h"
#include "bufstringset.h"
#include "surfaceinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "uivismenu.h"


const int uiVisPartServer::evUpdateTree = 	0;
const int uiVisPartServer::evSelection = 	1;
const int uiVisPartServer::evDeSelection = 	2;
const int uiVisPartServer::evGetNewData = 	3;
const int uiVisPartServer::evMouseMove = 	4;
const int uiVisPartServer::evInteraction = 	5;
const int uiVisPartServer::evSelectAttrib = 	6;
const int uiVisPartServer::evSelectColorAttrib= 7;
const int uiVisPartServer::evGetColorData = 	8;

const char* uiVisPartServer::appvelstr = "AppVel";
const char* uiVisPartServer::workareastr = "Work Area";


#define mDynamicCastAllConst() \
    const visBase::DataObject* obj = visBase::DM().getObj( id ); \
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,obj) \
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,obj) \
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,obj) \
    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,obj) \
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,obj) \

#define mDynamicCastAll() \
    visBase::DataObject* obj = visBase::DM().getObj( id ); \
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj) \
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj) \
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj) \
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj) \
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,obj) \


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
{
    vismenu = new uiVisMenu( appserv().parent(), this );

    visBase::DM().selMan().selnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.notify( 
  	mCB(this,uiVisPartServer,deselectObjCB) );
}


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifer.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );
    delete &eventmutex;
}


const char* uiVisPartServer::name() const  { return "Visualisation"; }


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return 0;
    return obj->name();
}


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    newscene->ref();
    scenes += newscene;
    return newscene->id();
}


void uiVisPartServer::removeScene( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
	scene->unRef();
	scenes -= scene;
	return;
    }
}


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::DataObject*, so, visBase::DM().getObj( id ) )
    if ( !so ) return;

    scene->addObject( so );
    sendEvent( evUpdateTree );
}


void uiVisPartServer::findObject( const type_info& ti, TypeSet<int>& res )
{
    visBase::DM().getIds( ti, res );
}


visBase::DataObject* uiVisPartServer::getObject( int id )
{
    mDynamicCastGet(visBase::DataObject*, so, visBase::DM().getObj(id) );
    return so;
}


void uiVisPartServer::addObject( visBase::DataObject* so, int sceneid,
			         bool saveinsessions  )
{
    mDynamicCastGet(visSurvey::Scene*, scene, visBase::DM().getObj(sceneid) );
    scene->addObject( so );
    //TODO: Handle saveinsessions
}


void uiVisPartServer::removeObject( visBase::DataObject* so, int sceneid )
{
    removeObject( so->id(), sceneid );
}


NotifierAccess& uiVisPartServer::removeAllNotifier()
{
    return visBase::DM().removeallnotify;
}


int uiVisPartServer::addInlCrlTsl( int sceneid, int type )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::PlaneDataDisplay* pdd = visSurvey::PlaneDataDisplay::create();
    visSurvey::PlaneDataDisplay::Type pddtype =
	!type ? visSurvey::PlaneDataDisplay::Inline
	      : ( type == 1 ? visSurvey::PlaneDataDisplay::Crossline
		            : visSurvey::PlaneDataDisplay::Timeslice );
    pdd->setType( pddtype );
    scene->addObject( pdd );

    setUpConnections( pdd->id() );
    return pdd->id();
}


int uiVisPartServer::addVolView( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::VolumeDisplay* vd = visSurvey::VolumeDisplay::create();
    scene->addObject( vd );

    setUpConnections( vd->id() );
    return vd->id();
}


int uiVisPartServer::addRandomLine( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::RandomTrackDisplay* rtd = 
			visSurvey::RandomTrackDisplay::create();
    scene->addObject( rtd );

    setUpConnections( rtd->id() );
    return rtd->id();
}


int uiVisPartServer::addSurface( int sceneid, const MultiID& emhorid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::SurfaceDisplay* sd = visSurvey::SurfaceDisplay::create();
    sd->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );

    PtrMan<Executor> exec = sd->createSurface( emhorid );
    if ( !exec )
    {
	sd->ref(); sd->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    uiExecutor uiexec (appserv().parent(), *exec );
    if ( !uiexec.execute() )
    {
	sd->ref(); sd->unRef();
	return -1;
    }

    scene->addObject( sd );
    sd->setZValues();
    sd->turnOnWireFrame( false );

    setUpConnections( sd->id() );
    return sd->id();
}


bool uiVisPartServer::loadcreateSurface( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( !sd ) return false;

    const MultiID& emsurfid = sd->surfaceId();
    PtrMan<Executor> exec0 = sd->loadSurface( emsurfid );
    if ( !exec0 ) return false;
    uiExecutor dlg0( appserv().parent(), *exec0 );
    if ( !dlg0.go() ) return false;

    PtrMan<Executor> exec1 = sd->createSurface( emsurfid );
    if ( !exec1 ) return false;
    uiExecutor dlg1( appserv().parent(), *exec1 );
    if ( !dlg1.go() ) return false;

    return true;
}


void uiVisPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	TypeSet<int> visids;
	getChildIds( sceneids[ids], visids );
	for ( int idv=0; idv<visids.size(); idv++ )
	{
	    int visid = visids[idv];
	    if ( isHorizon( visid ) )
	    {
		BufferString attrnm( "" );
		const AttribSelSpec* as = getSelSpec( visid );
		if ( as ) attrnm = as->userRef();
		hinfos += new SurfaceInfo( getObjectName(visid), 
					   getMultiID(visid), visid, attrnm );
	    }
	}
    }
}


void uiVisPartServer::shiftHorizon( int id, float shift )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->setShift( shift );
}


int uiVisPartServer::addWell( int sceneid, const MultiID& multiid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
    wd->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );

    if ( !wd->setWellId( multiid ) )
    {
	wd->ref(); wd->unRef();
	pErrMsg( wd->errMsg() );
	return -1;
    }

    scene->addObject( wd );

    setUpConnections( wd->id() );
    return wd->id();
}


void uiVisPartServer::displayLog( int id, int selidx, int lognr ) 
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( wd ) wd->displayLog( selidx, lognr );
}


void uiVisPartServer::refreshMarkers()
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> visids;
	getChildIds( sceneids[idx], visids );
	for ( int idv=0; idv<visids.size(); idv++ )
	{
	    visBase::DataObject* dobj = visBase::DM().getObj( visids[idv] );
	    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
	    if ( wd ) wd->addMarkers();
	}
    }
}


int uiVisPartServer::addPickSet( int sceneid, const PickSet& pickset )
{
    TypeSet<int> sceneids;
    bool doshare = false;
    if ( sceneid < 0 )
    {
	getChildIds( -1, sceneids );
	if ( sceneids.size() ) sceneid = sceneids[0];
	doshare = true;
    }

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();

    psd->setColor( pickset.color );
    psd->setName( pickset.name() );
    setPickSetData( psd->id(), pickset );

    scene->addObject( psd );
    setUpConnections( psd->id() );
    int psid = psd->id();
    if ( !doshare )
	return psid;

    for ( int idx=1; idx<sceneids.size(); idx++ )
	shareObject( sceneids[idx], psid );

    return psid;
}


void uiVisPartServer::setPickSetData( int id, const PickSet& pickset )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dobj)
    if ( !psd ) return;

    psd->removeAll();
    bool hasdir = false;
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const PickLocation& loc = pickset[idx];
	psd->addPick( Coord3(loc.pos,loc.z), loc.dir );
	hasdir = loc.hasDir();
    }
    
    if ( hasdir ) //show Arrows
    {
	TypeSet<char*> types; psd->getTypeNames( types ); 
	psd->setType( types.size()-1 );
    }
}


void uiVisPartServer::getAllPickSets( BufferStringSet& pset )
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	TypeSet<int> visids;
	getChildIds( sceneids[ids], visids );
	for ( int idv=0; idv<visids.size(); idv++ )
	{
	    if ( isPickSet( visids[idv] ) )
		pset.add( getObjectName( visids[idv] ) );
	}
    }
}


void uiVisPartServer::getPickSetData( int id, PickSet& pickset ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    if ( !psd ) return;

    pickset.color = psd->getMaterial()->getColor();
    pickset.color.setTransparency( 0 );
    for ( int idx=0; idx<psd->nrPicks(); idx++ )
	pickset += PickLocation( psd->getPick(idx), psd->getDirection(idx) );
}


int uiVisPartServer::addStickSet(int sceneid, const MultiID& multiid )
{
    TypeSet<int> sceneids;

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::StickSetDisplay* ssd = visSurvey::StickSetDisplay::create();
    ssd->setStickSet( multiid );

    scene->addObject( ssd );

    return ssd->id();
}


MultiID uiVisPartServer::getMultiID( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) return sd->surfaceId();

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    if ( wd ) return wd->wellId();

    return MultiID();
}


int uiVisPartServer::getObjectId(int scene, const MultiID& mid ) const
{
    TypeSet<int> ids;
    getChildIds(scene, ids);

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	if ( getMultiID(ids[idx])==mid )
	{
	    return ids[idx];
	}
    }

    return -1;
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );
    if ( !viewmode )
	toggleDraggers();
}


void uiVisPartServer::makeSubMenu( uiPopupMenu& mnu, int sceneid, int id )
{
    vismenu->makeSubMenu( mnu, sceneid, id );
}


bool uiVisPartServer::handleSubMenuSel( int mnu, int sceneid, int id )
{
    return vismenu->handleSubMenuSel( mnu, sceneid, id );
}


void uiVisPartServer::getChildIds( int id, TypeSet<int>& childids ) const
{
    childids.erase();

    if ( id==-1 )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	    childids += scenes[idx]->id();

	return;
    }

    const visSurvey::Scene* scene = getScene( id );
    if ( scene )
    {
	for ( int idx=0; idx<scene->size(); idx++ )
	{
	    childids += scene->getObject( idx )->id();
	}

	return;
    }

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd )
    {
	int inl, crl, tsl;
	vd->getPlaneIds( inl, crl, tsl );
	childids += inl;
	childids += crl;
	childids += tsl;
	childids += vd->getVolRenId();
    }
}


BufferString uiVisPartServer::getTreeInfo( int id ) const
{
    mDynamicCastAllConst();

    BufferString res;
    if ( pdd )
    {
	const visSurvey::PlaneDataDisplay::Type type = pdd->getType();
	if ( type==visSurvey::PlaneDataDisplay::Inline )
	    res = pdd->getCubeSampling(true).hrg.start.inl;
	else if ( type==visSurvey::PlaneDataDisplay::Crossline )
	    res = pdd->getCubeSampling(true).hrg.start.crl;
	else
	{
	    float val = pdd->getCubeSampling(true).zrg.start;
	    res = SI().zIsTime() ? mNINT(val * 1000) : val;
	}
    }

    if ( psd )
        res = psd->nrPicks();

    if ( sd )
	res = sd->getShift();

    mDynamicCastGet(const visBase::MovableTextureSlice*,mts,obj)
    if ( mts )
    {
	TypeSet<int> volids;
	visBase::DM().getIds( typeid(visSurvey::VolumeDisplay), volids );
	for ( int idx=0; idx<volids.size(); idx++ )
	{
	    visBase::DataObject* dobj = visBase::DM().getObj( volids[idx] );
	    mDynamicCastGet(visSurvey::VolumeDisplay*,vd_,dobj)
	    if ( !vd_ ) continue;
	    int inlid, crlid, tslid;
	    vd_->getPlaneIds( inlid, crlid, tslid );
	    if ( id == inlid || id == crlid )
		res += vd_->getPlanePos( id );
	    else if ( id == tslid )
	    {
		float val = vd_->getPlanePos( id );
		res += SI().zIsTime() ? mNINT(val * 1000) : val;
	    }
	}
    }
		
    return res;
}



BufferString uiVisPartServer::getDisplayName( int id ) const
{
    const AttribSelSpec* as = getSelSpec( id );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( isHorizon(id) || isFault(id) )
    {
	bool hasattrnm = dispname[0];
	if ( hasattrnm ) dispname += " (";
	dispname += getObjectName( id );
	if ( hasattrnm ) dispname += ")";
    }
    else if ( as && !dispname[0] )
	dispname = "<right-click>";
    else if ( !as )
	dispname = getObjectName( id );

    return dispname;
}


bool uiVisPartServer::isInlCrlTsl( int id, int type ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd && type < 0 ) return true;

    visSurvey::PlaneDataDisplay::Type pddtype =
	!type ? visSurvey::PlaneDataDisplay::Inline
	      : ( type == 1 ? visSurvey::PlaneDataDisplay::Crossline
		            : visSurvey::PlaneDataDisplay::Timeslice );
    return pdd && pdd->getType() == pddtype;
}


bool uiVisPartServer::isVolView( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    return vd;
}


bool uiVisPartServer::isRandomLine( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj)
    return rtd;
}


bool uiVisPartServer::isPickSet( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)
    return psd;
}


bool uiVisPartServer::isWell( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj)
    return wd;
}


bool uiVisPartServer::isHorizon( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    return sd ? sd->isHorizon() : false;
}


bool uiVisPartServer::isFault( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    return sd ? !sd->isHorizon() : false;
}


bool uiVisPartServer::isStickSet( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::StickSetDisplay*,sd,dobj)
    return sd;
}


const CubeSampling* uiVisPartServer::getCubeSampling( int id ) const
{
    mDynamicCastAllConst();
    if ( pdd ) return &pdd->getCubeSampling(true);
    if ( vd ) return &vd->getCubeSampling();

    return 0;
}


const AttribSliceSet* uiVisPartServer::getCachedData( int id, 
						      bool colordata ) const
{
    mDynamicCastAllConst()
    if ( pdd ) return pdd->getCachedData( colordata );
    if ( vd ) return vd->getCachedData( colordata );
    return 0;
}


bool uiVisPartServer::setCubeData( int id, AttribSliceSet* sliceset,
       				   bool colordata )
{
    if ( !sliceset ) return false;

    mDynamicCastAll();
    if ( pdd ) return pdd->putNewData( sliceset, colordata );
    if ( vd ) return vd->putNewData( sliceset, colordata );

    delete sliceset;
    return false;
}


void uiVisPartServer::showTexture( int id, int textureidx )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( pdd ) pdd->showTexture( textureidx );
    if ( sd ) sd->showTexture( textureidx );
}


void uiVisPartServer::getRandomPosDataPos( int id,
			   ObjectSet< TypeSet<BinIDZValues> >& bidzvset,
			   bool inclvals ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	sd->getAttribPositions( bidzvset, inclvals, 0 );
}


void uiVisPartServer::setRandomPosData( int id,
		    const ObjectSet<const TypeSet<const BinIDZValues> >* nd,
       		    bool colordata )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->putNewData( nd, colordata );
}


void uiVisPartServer::getRandomTrackPositions( int id, 
						TypeSet<BinID>& bids ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) rtd->getDataPositions( bids );
}
	

const Interval<float> uiVisPartServer::getRandomTraceZRange( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    return rtd ? rtd->getManipDepthInterval() : Interval<float>(0,0);
}


void uiVisPartServer::setRandomTrackData( int id, ObjectSet<SeisTrc>* data,
       					  bool colordata )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) rtd->putNewData( data, colordata );
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos : inlcrlmousepos; }


BufferString uiVisPartServer::getMousePosVal() const
{
    return mIsUndefined(mouseposval) ? BufferString("") 
				     : BufferString(mouseposval);
}


void uiVisPartServer::setSelSpec( const AttribSelSpec& as )
{ attribspec = as; }


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    mDynamicCastAllConst();

    BufferString res;
    if ( pdd )
    {
	const visSurvey::PlaneDataDisplay::Type type = pdd->getType();
	if ( type==visSurvey::PlaneDataDisplay::Inline )
	{
	    res = "Inline: ";
	    res += pdd->getCubeSampling(true).hrg.start.inl;
	}
	else if ( type==visSurvey::PlaneDataDisplay::Crossline )
	{
	    res = "Crossline: ";
	    res += pdd->getCubeSampling(true).hrg.start.crl;
	}
	else
	{
	    res = SI().zIsTime() ? "Time: " : "Depth: ";
	    float val = pdd->getCubeSampling(true).zrg.start;
	    res += SI().zIsTime() ? mNINT(val * 1000) : val;
	}
    }

    if ( psd )
    {
	res = "Nr of picks: ";
	res += psd->nrPicks();
    }

    if ( rtd )
    {
	int knotidx = rtd->getSelKnotIdx();
	if ( knotidx >= 0 )
	{
	    BinID binid  = rtd->getManipKnotPos( knotidx );
	    res = "Node "; res += knotidx;
	    res += " Inl/Crl: ";
	    res += binid.inl; res += "/"; res += binid.crl;
	}
    }

    if ( sd )
    {
	float shift = sd->getShift();
	if ( shift )
	{
	    res = "Horizon shift: ";
	    res += shift; res += " "; res += SI().getZUnit();
	}
    }

    mDynamicCastGet(const visBase::MovableTextureSlice*,mts,obj)
    if ( mts )
    {
	int dim = mts->dim();
	res = !dim ? "Inline: " : ( dim == 1 ? "Crossline: " : "Time: " );
	res += getTreeInfo( id );
    }

    return res;
}


int uiVisPartServer::getColTabId( int id ) const
{
    mDynamicCastAllConst();
    if ( pdd ) return pdd->getColorTab().id();
    if ( vd ) return vd->getColorTab().id();
    if ( rtd ) return rtd->getColorTab().id();
    if ( sd ) return sd->getColorTab().id();

    return -1;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( getColTabId(id) );
    mDynamicCastGet(visBase::VisColorTab*,coltab,obj)
    if ( coltab ) coltab->setClipRate( cr );
}


const TypeSet<float>* uiVisPartServer::getHistogram( int id ) const
{
    mDynamicCastAllConst();
    if ( pdd ) return &pdd->getHistogram();
    if ( vd ) return &vd->getHistogram();
    if ( rtd ) return &rtd->getHistogram();
    if ( sd ) return &sd->getHistogram();

    return 0;
}


int uiVisPartServer::getEventObjId() const { return eventobjid; }


const AttribSelSpec* uiVisPartServer::getSelSpec( int id ) const
{
    mDynamicCastAllConst();
    if ( pdd ) return &pdd->getAttribSelSpec();
    if ( sd ) return &sd->getAttribSelSpec();
    if ( vd ) return &vd->getAttribSelSpec();
    if ( rtd ) return &rtd->getAttribSelSpec();

    return 0;
}


bool uiVisPartServer::deleteAllObjects()
{

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	scenes[idx]->mouseposchange.remove(
	    			mCB(this,uiVisPartServer,mouseMoveCB) );
        scenes[idx]->unRef();
    }

    scenes.erase();

    return visBase::DM().reInit();
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;
    toggleDraggers();
}


void uiVisPartServer::toggleDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes[sceneidx];

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && !viewmode;

	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
	    if ( pdd ) pdd->showDraggers(isdraggeron);
	    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	    if ( vd ) vd->showBox(isdraggeron);
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj)
	    if ( rtd ) rtd->showDragger(isdraggeron);
	}
    }
}
    


void uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.go();
}


bool uiVisPartServer::setWorkingArea()
{
    uiWorkAreaDlg dlg( appserv().parent() );
    if ( !dlg.go() ) return false;

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObj( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene ) scene->updateRange();
	TypeSet<int> objids;
	getChildIds( sceneid, objids );
	for ( int ido=0; ido<objids.size(); ido++ )
	{
	    visBase::DataObject* dobj = visBase::DM().getObj( objids[ido] );
	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
	    if ( pdd ) pdd->setGeometry( true );
	}
    }

    return true;
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    BufferString res;
    if ( par.get( workareastr, res ) )
    {
	FileMultiString fms(res);
	BinIDRange hrg; Interval<double> zrg;
	hrg.start.inl = atoi(fms[0]); hrg.stop.inl = atoi(fms[1]);
	hrg.start.crl = atoi(fms[2]); hrg.stop.crl = atoi(fms[3]);
	zrg.start = (double)atof(fms[4]); zrg.stop = (double)atof(fms[5]);
	const_cast<SurveyInfo&>(SI()).setWorkRange( hrg );
	const_cast<SurveyInfo&>(SI()).setWorkZRange( zrg );
    }

    if ( !visBase::DM().usePar( par ) )
	return false;

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    TypeSet<int> hasconnections;

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
	    	(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );

	newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB));
	scenes += newscene;
	newscene->ref();

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    int childid = children[idy];
	    if ( isHorizon(childid) || isFault(childid) )
	    {
		if ( !loadcreateSurface(childid) )
		{
		    int objidx = newscene->getFirstIdx( childid );
		    newscene->removeObject( objidx );
		    continue;
		}
	    }

	    calculateAttrib( childid, false );
	    calculateColorAttrib( childid, false );

	    if ( hasconnections.indexOf( childid ) >= 0 ) continue;

	    setUpConnections( childid );
	    hasconnections += childid;

	    turnOn( childid, isOn(childid) );
	}
    }


    float appvel;
    if ( par.get( appvelstr, appvel ) )
	visSurvey::SPM().setZScale( appvel/1000 );

    return true;
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes.size(); idx++ )
	storids += scenes[idx]->id();

    par.set( appvelstr, visSurvey::SPM().getZScale()*1000 );

    BinIDRange hrg = SI().range();
    StepInterval<double> zrg = SI().zRange();
    FileMultiString fms;
    fms += hrg.start.inl; fms += hrg.stop.inl; fms += hrg.start.crl;
    fms += hrg.stop.crl; fms += zrg.start; fms += zrg.stop;
    par.set( workareastr, fms );

    visBase::DM().fillPar(par, storids);
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VolRender*,vr,obj)
    if ( yn && vr && !vr->isInited() )
    {
	PtrMan<Executor> exec = vr->init();
	uiExecutor uiexec(appserv().parent(), *exec );
	uiexec.go();
    }

    mDynamicCastGet(visBase::VisualObject*,so,obj)
    if ( so ) so->turnOn( yn );

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* scene =
		(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );
	if ( scene ) scene->filterPicks();
    }
}


bool uiVisPartServer::isOn( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet( const visBase::VisualObject*,so,obj)
    return so ? so->isOn() : false;
}


bool uiVisPartServer::canDuplicate( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)

    return ( vd || pdd );
}    


int uiVisPartServer::duplicateObject( int id, int sceneid )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)

    int newid = -1;
    if ( pdd )
    {
	newid = addInlCrlTsl( sceneid, 0 );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,newpdd,newobj)

	newpdd->setType( pdd->getType() );
	newpdd->setCubeSampling( pdd->getCubeSampling() );
	newpdd->setResolution( pdd->getResolution() );
	const char* ctnm = pdd->getColorTab().colorSeq().colors().name();
	newpdd->getColorTab().colorSeq().loadFromStorage( ctnm );
    }
    else if ( vd )
    {
	newid = addVolView( sceneid );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::VolumeDisplay*,newvd,newobj)

	newvd->setCubeSampling( vd->getCubeSampling() );
	const char* ctnm = vd->getColorTab().colorSeq().colors().name();
	newvd->getColorTab().colorSeq().loadFromStorage( ctnm );
    }

    return newid;
}


#define mGetScene( prepostfix ) \
prepostfix visSurvey::Scene* \
uiVisPartServer::getScene( int sceneid ) prepostfix \
{ \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
    { \
	if ( scenes[idx]->id()==sceneid ) \
	{ \
	    return scenes[idx]; \
	} \
    } \
 \
    return 0; \
}

mGetScene( );
mGetScene( const ); 


void uiVisPartServer::removeObject( int id, int sceneid )
{
    removeConnections( id );

    visSurvey::Scene* scene = getScene( sceneid );
    int idx = scene->getFirstIdx( id );
    scene->removeObject( idx );

    Threads::MutexLocker lock( eventmutex );
    sendEvent( evUpdateTree );
}


bool uiVisPartServer::hasAttrib( int id ) const
{
    mDynamicCastAllConst();
    return ( vd || rtd || pdd || (sd && sd->usesTexture()) );
}
    

bool uiVisPartServer::selectAttrib( int id )
{
    eventmutex.lock();
    bool selected = sendEvent( evSelectAttrib );
    eventmutex.unlock();

    if ( !selected ) return false;

    const AttribSelSpec myattribspec( attribspec );
    setSelSpec( id, myattribspec );
    eventmutex.lock();
    eventobjid = id;
    sendEvent( evUpdateTree );
    eventmutex.unlock();

    return true;
}


void uiVisPartServer::setSelSpec( int id, const AttribSelSpec& myattribspec )
{
    mDynamicCastAll();
    if ( vd ) vd->setAttribSelSpec( myattribspec );
    if ( rtd ) rtd->setAttribSelSpec( myattribspec );
    if ( pdd ) pdd->setAttribSelSpec( myattribspec );
    if ( sd ) sd->setAttribSelSpec( myattribspec );
}


bool uiVisPartServer::calculateAttrib( int id, bool newselect )
{
    mDynamicCastAll();

    const AttribSelSpec* as = getSelSpec( id );
    if ( !as ) return false;
    bool res = true;
    if ( newselect || ( as->id() < 0 && !sd ) )
	res = selectAttrib( id );
    else if ( sd && as->id() < 0 )
    {
	const char* usrref = as->userRef();
	if ( !usrref || !(*usrref) )
	{
	    sd->setZValues();
	    return true;
	}
    }
	
    if ( !res ) return res;

    res = false;
    if ( vd || pdd || rtd || sd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	res = sendEvent( evGetNewData );
    }

    if ( res ) acceptManipulation( id );
    return res;
}


bool uiVisPartServer::hasColorAttrib( int id ) const
{
    mDynamicCastAllConst();
    return ( vd || rtd || pdd || (sd && sd->usesTexture()) );
}


const ColorAttribSel* uiVisPartServer::getColorSelSpec( int id ) const
{
    mDynamicCastAllConst();
    if ( pdd ) return &pdd->getColorSelSpec();
    if ( rtd ) return &rtd->getColorSelSpec();
    if ( vd ) return &vd->getColorSelSpec();
    if ( sd ) return &sd->getColorSelSpec();

    return 0;
}


void uiVisPartServer::setColorSelSpec( int id, const ColorAttribSel& myas )
{
    mDynamicCastAll();
    if ( pdd ) pdd->setColorSelSpec( myas );
    if ( rtd ) rtd->setColorSelSpec( myas );
    if ( vd ) vd->setColorSelSpec( myas );
    if ( sd ) sd->setColorSelSpec( myas );
}


void uiVisPartServer::setColorSelSpec( const ColorAttribSel& myas )
{
    colorspec = myas;
}


void uiVisPartServer::resetColorDataType( int id )
{
    mDynamicCastAll();
    if ( pdd ) pdd->getColorSelSpec().datatype = 0;
    if ( rtd ) rtd->getColorSelSpec().datatype = 0;
    if ( vd ) vd->getColorSelSpec().datatype = 0;
    if ( sd ) sd->getColorSelSpec().datatype = 0;
}


bool uiVisPartServer::calculateColorAttrib( int id, bool newselect )
{
    mDynamicCastAll();
    if ( !vd && !pdd && !rtd && !sd )
	return false;

    const ColorAttribSel* colas = getColorSelSpec( id );
    const int attribid = colas->as.id();
    if ( !colas ) return false;
    if ( !newselect && attribid < 0 )
	return false;

    bool res = true;
    if ( newselect || ( attribid < 0 ) )
	res = selectColorAttrib( id );
	
    if ( !res ) return res;
    if ( !colorspec.datatype )
    {
	removeColorData( id );
	return true;
    }

    Threads::MutexLocker lock( eventmutex );
    eventobjid = id;
    res = sendEvent( evGetColorData );
    return res;
}


void uiVisPartServer::removeColorData( int id )
{
    mDynamicCastAll();
    if ( pdd ) pdd->putNewData( 0, true );
    else if ( rtd ) rtd->putNewData( 0, true );
    else if ( sd ) sd->putNewData( 0, true );
    else if ( vd ) vd->putNewData( 0, true );
}


bool uiVisPartServer::selectColorAttrib( int id )
{
    eventmutex.lock();
    bool selected = sendEvent( evSelectColorAttrib );
    eventmutex.unlock();

    if ( !selected ) return false;

    const ColorAttribSel myattribspec( colorspec );
    setColorSelSpec( id, myattribspec );
    eventobjid = id;

    return true;
}


bool uiVisPartServer::isMovable( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)

    return ( pdd || vd );
}


bool uiVisPartServer::setPosition( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( !pdd && !vd ) return false;

    const CubeSampling initcs = pdd ? pdd->getCubeSampling( true )
				    : vd->getCubeSampling();
    uiSliceSel dlg( appserv().parent(), initcs,
		    mCB(this,uiVisPartServer,updatePlanePos), (bool)vd );
    if ( dlg.go() )
    {
	bool res;
	CubeSampling cs = dlg.getCubeSampling();
	if ( pdd )
	    pdd->setCubeSampling( cs );
	else if ( vd )
	    vd->setCubeSampling( cs );

	res = calculateAttrib( id, false );
	calculateColorAttrib( id, false );

	sendEvent( evInteraction );
	return res;
    }

    return true;
}


void uiVisPartServer::updatePlanePos( CallBacker* cb )
{
    int id = getSelObjectId();
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( !pdd ) return;
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    calculateAttrib( id, false );
    calculateColorAttrib( id, false );
    sendEvent( evInteraction );
}
  

bool uiVisPartServer::hasMaterial( int id ) const
{
    mDynamicCastAllConst();
    return ( pdd || vd || rtd || sd );
}


bool uiVisPartServer::setMaterial( int id )
{
    mDynamicCastAll();
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    if ( !vo ) return false;

    visBase::Material* mat = sd ? sd->getMaterial() : vo->getMaterial();
    uiMaterialDlg dlg( appserv().parent(), mat, true,
		       true, false, false, false, true );
    dlg.go();

    return true;
}


bool uiVisPartServer::hasColor( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj)
    return ( sd && !sd->usesTexture() ); 
}


bool uiVisPartServer::dumpOI( int id ) const
{
    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(), "*.iv",
	    		  "Select output file" );
    if ( filedlg.go() )
    {
	visBase::DataObject* obj = visBase::DM().getObj( id );
	if ( !obj ) return false;
	return obj->dumpOIgraph( filedlg.fileName() );
    }

    return false;
}


bool uiVisPartServer::resetManipulation( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( pdd ) pdd->resetManip();
    if ( vd ) vd->resetManip();
    return vd || pdd;
}


bool uiVisPartServer::isManipulated( int id ) const
{
    mDynamicCastAll();
    if ( pdd ) 
	return pdd->getCubeSampling( true )!=pdd->getCubeSampling( false );

    if ( vd )
	return vd->manipCenter()!=vd->center() || vd->manipWidth()!=vd->width();

    if ( rtd )
	return rtd->isManipulated();

    return false;
}


void uiVisPartServer::acceptManipulation( int id )
{
    mDynamicCastAll();
    if ( pdd )
    {
	pdd->setCubeSampling( pdd->getCubeSampling(true));
	pdd->resetManip();
    }

    else if ( vd )
    {
	vd->setCubeSampling( vd->getCubeSampling(true) );
	vd->resetManip();
    }

    else if ( rtd )
	rtd->acceptManip();
}


void uiVisPartServer::setUpConnections( int id )
{
    mDynamicCastAll();
    CallBack cb = mCB(this,uiVisPartServer,interactionCB);
    if ( vd )
	vd->slicemoving.notify( cb );
    else if ( pdd )
	pdd->getMovementNotification()->notify( cb );
    else if ( sd )
	sd->getMovementNotification()->notify( cb ); 
    else if ( psd )
	psd->changed.notify( cb );
    else if ( rtd )
    {
	rtd->knotmoving.notify( cb );
	rtd->rightclick.notify( mCB(vismenu,uiVisMenu,createPopupMenu) );
    }
}


void uiVisPartServer::removeConnections( int id )
{
    mDynamicCastAll();
    CallBack cb = mCB(this,uiVisPartServer,interactionCB);
    if ( vd )
	vd->slicemoving.remove( cb );
    else if ( pdd )
	pdd->getMovementNotification()->remove( cb );
    else if ( sd )
	sd->getMovementNotification()->remove( cb );
    else if ( psd )
	psd->changed.remove( cb );
    else if ( rtd )
    {
	rtd->knotmoving.remove( cb );
	rtd->rightclick.remove( mCB(this,uiVisMenu,createPopupMenu) );
    }
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( !viewmode )
	toggleDraggers();

    Threads::MutexLocker lock( eventmutex );
    eventobjid = sel;
    sendEvent( evSelection );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    if ( isManipulated(oldsel) )
    {
	if ( hasAttrib(oldsel) )
	    calculateAttrib( oldsel, false );
	if ( hasColorAttrib(oldsel) )
	    calculateColorAttrib( oldsel, false );
    }

    if ( !viewmode )
	toggleDraggers();


    Threads::MutexLocker lock( eventmutex );
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::interactionCB( CallBacker* cb )
{
    Threads::MutexLocker lock( eventmutex );
    mDynamicCastGet(visBase::DataObject*,dataobj,cb)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dataobj)
    if ( dataobj && !vd )
	eventobjid = dataobj->id();
    else
	eventobjid = getSelObjectId();

    sendEvent( evInteraction );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb)
    if ( !cb ) return;

    int selid = getSelObjectId();
    const visBase::DataObject* dobj = visBase::DM().getObj( selid );
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj)

    if ( selid==-1 || psd )
    {
	Threads::MutexLocker lock( eventmutex );
	xytmousepos = scene->getMousePos(true);
	inlcrlmousepos = scene->getMousePos(false);
	mouseposval = scene->getMousePosValue();
	sendEvent( evMouseMove );
    }
}

