/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.126 2003-02-11 09:57:25 nanne Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribslice.h"
#include "draw.h"
#include "errh.h"
#include "pickset.h"
#include "ptrman.h"
#include "settings.h"
#include "separstr.h"
#include "survinfo.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "visgridsurf.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "visselman.h"
#include "vissurvinterpret.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vissurvsurf.h"
#include "vissurvwell.h"
#include "visvolumedisplay.h"
#include "visrandomtrackdisplay.h"
#include "uiexecutor.h"
#include "uifiledlg.h"
#include "uimaterialdlg.h"
#include "uimenu.h"
#include "uipickszdlg.h"
#include "uisellinest.h"
#include "uislicesel.h"
#include "uizscaledlg.h"
#include "uiworkareadlg.h"
#include "uicolor.h"
#include "uidset.h"
#include "colortab.h"
#include "surfaceinfo.h"


const int uiVisPartServer::evUpdateTree = 0;
const int uiVisPartServer::evSelection = 1;
const int uiVisPartServer::evDeSelection = 2;
const int uiVisPartServer::evGetNewCubeData = 3;
const int uiVisPartServer::evGetNewRandomPosData = 4;
const int uiVisPartServer::evMouseMove = 5;
const int uiVisPartServer::evGetRandomTracePosData = 6;
const int uiVisPartServer::evInteraction = 7;
const int uiVisPartServer::evSelectAttrib = 8;

const char* uiVisPartServer::appvelstr = "AppVel";
const char* uiVisPartServer::workareastr = "Work Area";


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
    , interpreterdisplay( 0 )
{
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
	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB));
	scene->unRef();
	scenes -= scene;
	return;
    }
}


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::SceneObject*, so, visBase::DM().getObj( id ) );
    if ( !so ) return;

    scene->addObject( so );
    sendEvent( evUpdateTree );
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

    visSurvey::VolumeDisplay* vol = visSurvey::VolumeDisplay::create();
    scene->addObject( vol );

    setUpConnections( vol->id() );
    return vol->id();
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

    visSurvey::SurfaceDisplay* horizon = visSurvey::SurfaceDisplay::create();
    horizon->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );

    PtrMan<Executor> exec = horizon->setSurfaceId( emhorid );
    if ( !exec )
    {
	horizon->ref(); horizon->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    uiExecutor uiexec (appserv().parent(), *exec );
    if ( !uiexec.execute() )
    {
	horizon->ref(); horizon->unRef();
	return -1;
    }

    horizon->setZValues();
    scene->addObject( horizon ); //Must be done before loading surface
    				 //Otherwise the transform won't be set


    setUpConnections( horizon->id() );
    return horizon->id();
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
		hinfos += new SurfaceInfo( getObjectName(visid), "", visid,
		       attrnm );
	    }
	}
    }
}


int uiVisPartServer::addWell( int sceneid, const MultiID& emwellid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
    if ( !wd->setWellId( emwellid ) )
    {
	wd->ref(); wd->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    if ( wd->depthIsT() )
	scene->addObject( wd );
    else
	scene->addObject( wd );

    setUpConnections( wd->id() );
    return wd->id();
}


int uiVisPartServer::addPickSet(int sceneid, const PickSet& pickset )
{
    if ( sceneid < 0 )
    {
	TypeSet<int> sceneids;
	getChildIds( -1, sceneids );
	if ( sceneids.size() ) sceneid = sceneids[0];
    }
	    
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;

    visSurvey::PickSetDisplay* ps = visSurvey::PickSetDisplay::create();

    ps->setColor( pickset.color );
    ps->setName( pickset.name() );
    setPickSetData( ps->id(), pickset );

    scene->addObject( ps );
    setUpConnections( ps->id() );
    return ps->id();
}


void uiVisPartServer::setPickSetData( int id, const PickSet& pickset )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,dobj);
    if ( !ps ) return;

    ps->removeAll();
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	Coord crd( pickset[idx].pos );
	ps->addPick( Coord3(crd.x,crd.y,pickset[idx].z) );
    }
}


void uiVisPartServer::getAllPickSets( UserIDSet& pset )
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
    mDynamicCastGet(const visSurvey::PickSetDisplay*,visps,dobj);
    if ( !visps ) return;

    pickset.color = visps->getMaterial()->getColor();
    pickset.color.setTransparency( 0 );
    for ( int idx=0; idx<visps->nrPicks(); idx++ )
    {
	Coord3 pos = visps->getPick( idx );
	pickset += PickLocation( pos.x, pos.y, pos.z );
    }
}


MultiID uiVisPartServer::getMultiID( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,surface,dobj);
    if ( surface ) return surface->surfaceId();

     mDynamicCastGet(const visSurvey::WellDisplay*, well, dobj );
     if ( well ) return well->wellId();

     return MultiID();
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


#define mSelAttrib		1024
#define mPosition		1025
#define mProperties		1026
#define mRemove			1027
#define mShowAll		1028
#define mResolution		1029
#define mResolutionStart	1030
//Leave 100 spaces for the versions
#define mResolutionStop		1129
#define mSingleColor		1130
#define mColor			1131
#define mLineStyle		1132
#define mShowWellName		1133
#define mShare			1134
#define mShareStart		1135
////Leave 100 spaces for submnus
#define mShareStop		1235
#define mAnnotText		1236
#define mAnnotScale		1237
#define mSurveyBox		1238
#define mDumpIV			1239
#define mResetManip		1240
#define mSetPickSz		1241
#define mDuplicate		1242
#define mAddKnot		1243
#define mInsertKnot		1244
#define mInsertKnotStart	1245
////Leave 100 spaces for submnus
#define mInsertKnotStop		1346


void uiVisPartServer::makeSubMenu( uiPopupMenu& mnu, int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    if ( hasAttrib( id ) )
	 mnu.insertItem( new uiMenuItem("Select attribute ..."), mSelAttrib);
    if ( isMovable( id ) )
	mnu.insertItem( new uiMenuItem("Position ..."), mPosition );
    if ( isManipulated( id ) )
	mnu.insertItem( new uiMenuItem("Reset manipulation"), mResetManip );
    if ( hasColor( id ) )
	mnu.insertItem( new uiMenuItem("Color ..."), mColor );
    if ( hasMaterial( id ) )
	mnu.insertItem( new uiMenuItem("Properties ..."), mProperties );
    if ( hasDuplicate( id ) )
	mnu.insertItem( new uiMenuItem("Duplicate"), mDuplicate );

    if ( sceneid==id )
    {
	const bool showcube = scene->isAnnotShown();

	uiMenuItem* anntxt = new uiMenuItem("Annotation text");
	mnu.insertItem( anntxt, mAnnotText );
	anntxt->setChecked( showcube && scene->isAnnotTextShown() );
	anntxt->setEnabled( showcube );

	uiMenuItem* annscale = new uiMenuItem("Annotation scale");
	mnu.insertItem( annscale, mAnnotScale );
	annscale->setChecked( showcube && scene->isAnnotScaleShown() );
	annscale->setEnabled( showcube );

	uiMenuItem* annsurv = new uiMenuItem("Survey box");
	mnu.insertItem( annsurv, mSurveyBox );
	annsurv->setChecked( showcube );

	bool doi = false;
	Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), doi );
	if ( doi )
	    mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );
    }

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd )
    {
	uiPopupMenu* resmnu = new uiPopupMenu( appserv().parent(),"Resolution");
	const int curres = pdd->getResolution();
	const int nrres = pdd->getNrResolutions();
	for ( int idx=0; idx<nrres; idx++ )
	{
	    uiMenuItem* resitm = new uiMenuItem(pdd->getResName(idx));
	    resmnu->insertItem( resitm, mResolutionStart+idx );
	    resitm->setChecked( curres==idx );
	}
	mnu.insertItem( resmnu );
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,pickset,dobj);
    if ( pickset )
    {
	uiMenuItem* showitm = new uiMenuItem("Show all");
	mnu.insertItem( showitm, mShowAll );
	showitm->setChecked( pickset->allShown() );

	mnu.insertItem( new uiMenuItem("Properties ..."), mSetPickSz );
    }

    mDynamicCastGet(const visSurvey::SurfaceDisplay*, surf, dobj );
    if ( surf )
    {
	uiMenuItem* colitm = new uiMenuItem("Use single color");
	mnu.insertItem( colitm, mSingleColor );
	colitm->setChecked( !surf->usesTexture() );

	uiPopupMenu* resmnu = new uiPopupMenu( appserv().parent(),"Resolution");
	const int curres = surf->getResolution();
	const int nrres = surf->getNrResolutions();
	for ( int idx=0; idx<nrres; idx++ )
	{
	    colitm = new uiMenuItem(surf->getResName(idx));
	    resmnu->insertItem( colitm, mResolutionStart+idx );
	    colitm->setChecked( curres==idx );
	}
	mnu.insertItem( resmnu );
    }

    mDynamicCastGet(const visSurvey::WellDisplay*, well, dobj );
    if ( well )
    {
	mnu.insertItem( new uiMenuItem("Linestyle ..."), mLineStyle );
	uiMenuItem* welltxt = new uiMenuItem("Show well name");
	mnu.insertItem( welltxt, mShowWellName );
	welltxt->setChecked( well->isWellTextShown() );
    }

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*, rtd, dobj );
    if ( rtd )
    {
	mnu.insertItem( new uiMenuItem("Add knot"), mAddKnot );
	uiPopupMenu* insertknotmnu = new uiPopupMenu( appserv().parent(),
				    "Insert knot before...");
	for ( int idx=0; idx<rtd->nrKnots(); idx++ )
	{
	    BufferString knotname = "knot ";
	    knotname += idx;
	    uiMenuItem* itm = new uiMenuItem(knotname);
	    insertknotmnu->insertItem( itm, mInsertKnotStart+idx );
	}

	mnu.insertItem( insertknotmnu, mInsertKnot );
    }

#ifdef __debug__
    if ( scenes.size()>1 )
    {
	uiPopupMenu* sharemnu = new uiPopupMenu( appserv().parent(),
				    "Share with...");
	for ( int idx=0; idx<scenes.size(); idx++ )
	{
	    if ( scenes[idx]->getFirstIdx( id )==-1 )
	    {
		uiMenuItem* itm = new uiMenuItem(scenes[idx]->name());
		sharemnu->insertItem( itm, mShareStart+idx );
	    }
	}

	mnu.insertItem( sharemnu, mShare );
    }
#endif

    if ( scene->getFirstIdx(id)!=-1 )
	mnu.insertItem( new uiMenuItem("Remove"), mRemove );

    return;
}


bool uiVisPartServer::handleSubMenuSel( int mnu, int sceneid, int id)
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return false;

    if ( mnu==mAnnotText )
    {
	scene->showAnnotText( !scene->isAnnotTextShown() );
	return true;
    }

    if ( mnu==mAnnotScale )
    {
	scene->showAnnotScale( !scene->isAnnotScaleShown() );
	return true;
    }

    if ( mnu==mSurveyBox )
    {
	scene->showAnnot( !scene->isAnnotShown() );
	return true;
    }

    if ( mnu==mDumpIV )
	return dumpOI( id );

    if ( mnu==mDuplicate )
	return duplicateObject( id, sceneid );
	
    if ( mnu==mSelAttrib )
	 return calculateAttrib(id,true);  //Will both select and calculate

    if ( mnu==mPosition )
	return setPosition( id );

    if ( mnu==mResetManip )
	return resetManipulation( id );

    if ( mnu==mProperties )
	return setMaterial( id );

    if ( mnu==mRemove )
    {
	removeObject( id, sceneid );
	return true;
    }

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    if ( mnu==mShowAll )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,pickset,dobj);
	bool showall = !pickset->allShown();
	pickset->showAll( showall );
	if ( !showall )
	{
	    for ( int idx=0; idx<scenes.size(); idx++ )
		scenes[idx]->filterPicks();
	}
	
	return true;
    }

    if ( mnu==mAddKnot )
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
	const int nrknots = rtd->nrKnots();
	const Coord lastknot = rtd->getKnotPos( nrknots-1 );
	const Coord secondlastknot = rtd->getKnotPos( nrknots-2 );
	const Coord diff = lastknot-secondlastknot;
	const Coord newknot = lastknot+diff;
	rtd->addKnot( newknot );
    }

    if ( mnu>=mInsertKnotStart && mnu<=mInsertKnotStop )
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
	int knotidx = mnu-mInsertKnotStart;

	Coord newknot;
	if ( !knotidx )
	{
	    const Coord lastknot = rtd->getKnotPos( 0 );
	    const Coord secondlastknot = rtd->getKnotPos( 1 );
	    const Coord diff = lastknot-secondlastknot;
	    Coord newknot = lastknot+diff;
	}
	else
	{
	    const Coord previousknot = rtd->getKnotPos( knotidx-1 );
	    const Coord nextknot = rtd->getKnotPos( knotidx );
	    newknot = Coord((nextknot.x+previousknot.x)/2,
		    	    (nextknot.y+previousknot.y)/2 );
	}

	rtd->insertKnot( knotidx, newknot );
    }

    if ( mnu>=mResolutionStart && mnu<=mResolutionStop )
    {
	const int resolution = mnu-mResolutionStart;
	mDynamicCastGet(visSurvey::SurfaceDisplay*,surf,dobj);
	if ( surf ) surf->setResolution( resolution );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj);
	if ( pdd ) pdd->setResolution( resolution );
	return true;
    }

    if ( mnu==mSingleColor )
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,surf,dobj);
	surf->useTexture( !surf->usesTexture() );
	return true;
    }

    if ( mnu==mColor )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,dobj)
	Color col = vo->getMaterial()->getColor();
	if ( select( col, appserv().parent(), "Color selection", false ) )
	    vo->getMaterial()->setColor( col );
	return true;
    }

    if ( mnu==mLineStyle )
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj);
	LineStyleDlg dlg( appserv().parent(), wd->lineStyle(), 0, false, true );
	if ( dlg.go() )
	    wd->setLineStyle( dlg.getLineStyle() );

	return true;
    }

    if ( mnu==mShowWellName )
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj);
	wd->showWellText( !wd->isWellTextShown() );
	return true;
    }

    if ( mnu>=mShareStart && mnu<=mShareStop )
    {
	const int sceneidx = mnu-mShareStart;
	mDynamicCastGet(visBase::SceneObject*,sceneobj,dobj);
	if ( !sceneobj ) 
	    pErrMsg( "oops" );
	shareObject( scenes[sceneidx]->id(), sceneobj->id() );
	return true;
    }

    if ( mnu==mSetPickSz )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,ps,dobj);
	uiPickSizeDlg dlg( appserv().parent(), ps );
	dlg.go();
	return true;
    }

    return true;
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
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    BufferString res;
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd )
    {
	const visSurvey::PlaneDataDisplay::Type type = pdd->getType();
	if ( type==visSurvey::PlaneDataDisplay::Inline )
	    res = pdd->getCubeSampling(false).hrg.start.inl;
	else if ( type==visSurvey::PlaneDataDisplay::Crossline )
	    res = pdd->getCubeSampling(false).hrg.start.crl;
	else
	    res = pdd->getCubeSampling(false).zrg.start;
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,ps,dobj);
    if ( ps )
        res = ps->nrPicks();

    return res;
}


bool uiVisPartServer::isInlCrlTsl( int id, int type ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
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
    mDynamicCastGet(const visSurvey::PickSetDisplay*,ps,dobj);
    return ps;
}


bool uiVisPartServer::isWell( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj);
    return wd;
}


bool uiVisPartServer::isHorizon( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,surface,dobj);
    return surface ? surface->isHorizon() : false;
}


bool uiVisPartServer::isFault( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,surface,dobj);
    return surface ? !surface->isHorizon() : false;
}


const CubeSampling* uiVisPartServer::getCubeSampling( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd ) return &pdd->getCubeSampling(true);

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj);
    if ( vd ) return &vd->getCubeSampling();

    mDynamicCastGet(visSurvey::SurfaceInterpreterDisplay*,si,dobj);
    if ( si ) return &si->getCubeSampling();

    return 0;
}


const AttribSliceSet* uiVisPartServer::getCachedData( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,obj);
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vv,obj);
    if ( pdd ) return pdd->getPrevData();
    if ( vv ) return vv->getPrevData();
    else return 0;
}


bool uiVisPartServer::setCubeData( int id, AttribSliceSet* sliceset )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    mDynamicCastGet(visSurvey::VolumeDisplay*,vv,obj);
    if ( pdd ) return pdd->putNewData(sliceset);
    if ( vv ) return vv->putNewData( sliceset );

    delete sliceset;
    return false;
}


void uiVisPartServer::getRandomPosDataPos( int id,
			   ObjectSet< TypeSet<BinIDZValue> >& bidzvset) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	sd->getAttribPositions( bidzvset, true, 0 );
}


void uiVisPartServer::setRandomPosData( int id,
		    const ObjectSet<const TypeSet<const BinIDZValue> >& nd )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->putNewData( nd );
}


void uiVisPartServer::getRandomTrackPositions( int id, 
						TypeSet<BinID>& bids ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd ) rtd->getDataPositions( bids );
}
	

const Interval<float> uiVisPartServer::getRandomTraceZRange( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
    return rtd ? rtd->getDepthInterval() : Interval<float>(0,0);
}


void uiVisPartServer::setRandomTrackData( int id, ObjectSet<SeisTrc>& data )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd ) rtd->putNewData( data );
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos : inlcrlmousepos; }


BufferString uiVisPartServer::getMousePosVal() const
{
    return mIsUndefined(mouseposval)
	? BufferString("") : BufferString(mouseposval);
}


void uiVisPartServer::setSelSpec( const AttribSelSpec& as )
{ attribspec = as; }


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    BufferString res;
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
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
	    res += pdd->getCubeSampling(true).zrg.start;
	}
    }

    mDynamicCastGet(const visSurvey::PickSetDisplay*,ps,dobj)
    if ( ps )
    {
	res = "Nr of picks: ";
	res += ps->nrPicks();
    }

    return res;
}


int uiVisPartServer::getColTabId( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return pdd->getColorTab().id();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return vd->getColorTab().id();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj)
    if ( rtd ) return rtd->getColorTab().id();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,surface,dobj)
    if ( surface ) return surface->getColorTab().id();

    return -1;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( getColTabId(id) );
    mDynamicCastGet(visBase::VisColorTab*,coltab,obj)
    if ( coltab ) coltab->setClipRate( cr );
}


int uiVisPartServer::getEventObjId() const { return eventobjid; }


const AttribSelSpec* uiVisPartServer::getSelSpec( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return &pdd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,obj);
    if ( sd ) return &sd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,obj);
    if ( vd ) return &vd->getAttribSelSpec();

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,obj);
    if ( rtd ) return &rtd->getAttribSelSpec();

    return 0;
}


bool uiVisPartServer::deleteAllObjects()
{
    if ( interpreterdisplay ) interpreterdisplay->unRef();

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	scenes[idx]->mouseposchange.remove(
	    			mCB( this, uiVisPartServer, mouseMoveCB ));
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
	    visBase::SceneObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && !viewmode;

	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
	    if ( pdd ) pdd->showDraggers(isdraggeron);
	    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	    if ( vd ) vd->showBox(isdraggeron);
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,obj);
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
	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj);
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
    {	
	pErrMsg( "Could not parse session");
	return false;
    }

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    TypeSet<int> hasconnections;

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
	    	(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );

	newscene->mouseposchange.notify(
				mCB( this, uiVisPartServer, mouseMoveCB ));
	scenes += newscene;
	newscene->ref();

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    calculateAttrib( children[idy], false );

	    if ( hasconnections.indexOf( children[idy] ) >= 0 ) continue;

	    setUpConnections( children[idy] );
	    hasconnections += children[idy];
	}
    }

    TypeSet<int> interpreters;
    visBase::DM().getIds(
	    typeid(visSurvey::SurfaceInterpreterDisplay), interpreters );
    interpreterdisplay = interpreters.size()
		?  dynamic_cast<visSurvey::SurfaceInterpreterDisplay*>(
		    visBase::DM().getObj( interpreters[0] ))
		: 0;

    if ( interpreterdisplay ) interpreterdisplay->ref();

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
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    if ( so ) so->turnOn( yn );
}


bool uiVisPartServer::isOn( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet( const visBase::VisualObject*,so,obj)
    return so ? so->isOn() : false;
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
    visSurvey::Scene* scene = getScene( sceneid );
    int idx = scene->getFirstIdx( id );
    scene->removeObject( idx );

    Threads::MutexLocker lock( eventmutex );
    sendEvent( evUpdateTree );
}


bool uiVisPartServer::hasAttrib( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj);
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd ) return true;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd ) return true;

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,surface,dobj);
    if ( surface && surface->usesTexture() ) return true;

    return false;
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
    sendEvent(evUpdateTree);
    eventmutex.unlock();

    return true;
}


void uiVisPartServer::setSelSpec( int id, const AttribSelSpec& myattribspec )
{

    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj);
    if ( vd ) vd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd ) rtd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd ) pdd->setAttribSelSpec( myattribspec );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,surface,dobj);
    if ( surface ) surface->setAttribSelSpec( myattribspec );
}


bool uiVisPartServer::calculateAttrib( int id, bool newselect )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );

    const AttribSelSpec* as = getSelSpec( id );
    if ( !as ) return false;
    bool res = true;
    if ( newselect || as->id() < 0 ) 
	res = selectAttrib( id );
    if ( !res ) return res;

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj);
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( vd || pdd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	return sendEvent( evGetNewCubeData );
    }

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	return sendEvent( evGetRandomTracePosData );
    }

    mDynamicCastGet(visSurvey::SurfaceDisplay*,surface,dobj);
    if ( surface )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = id;
	return sendEvent( evGetNewRandomPosData );
    }

    return false;
}



bool uiVisPartServer::isMovable( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj);
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dobj);
    if ( rtd ) return false;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd ) return true;

    return false;
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
	CubeSampling cs = dlg.getCubeSampling();
	if ( pdd )
	{
	    pdd->setCubeSampling( cs );
	    return calculateAttrib( id, false );
	}
	else if ( vd )
	{
	    vd->setCubeSampling( cs );
	    return calculateAttrib( id, false );
	}
    }

    return true;
}


void uiVisPartServer::updatePlanePos( CallBacker* cb )
{
    int id = getSelObjectId();
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( !pdd ) return;
    mDynamicCastGet(uiSliceSel*,dlg,cb);
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    calculateAttrib( id, false );
    sendEvent( evInteraction );
}
  

bool uiVisPartServer::hasMaterial( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::Scene*,scene,dobj);
    if ( scene ) return false;

    mDynamicCastGet(const visSurvey::PickSetDisplay*,ps,dobj);
    if ( ps ) return false;

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj);
    if ( wd ) return false;

    return true;
}


bool uiVisPartServer::setMaterial( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    if ( !vo ) return false;

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( pdd || vd || (sd && sd->usesTexture()) )
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true,
	       		   true, false, false, false, true );
	dlg.go();
    }
    else
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true, 
			   true, false, false, false, true );
	dlg.go();
    }

    return true;
}


bool uiVisPartServer::hasColor( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj);
    if ( wd ) return true;

    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj);
    if ( sd && !sd->usesTexture() ) return true;

    return false;
}


bool uiVisPartServer::hasDuplicate( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );

    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj);
    if ( vd ) return true;

    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj);
    if ( pdd ) return true;

    return false;
}    


bool uiVisPartServer::duplicateObject( int id, int sceneid )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)

    int newid = -1;
    if ( pdd )
    {
	newid = addInlCrlTsl( sceneid, 0 );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,newpdd,newobj);

	newpdd->setType( pdd->getType() );
	newpdd->setCubeSampling( pdd->getCubeSampling() );
    }
    else if ( vd )
    {
	newid = addVolView( sceneid );
	visBase::DataObject* newobj = visBase::DM().getObj( newid );
	mDynamicCastGet(visSurvey::VolumeDisplay*,newvd,newobj);

	newvd->setCubeSampling( vd->getCubeSampling() );
    }

    eventobjid = newid;
    sendEvent( evUpdateTree );
    return true;
}


bool uiVisPartServer::dumpOI( int id ) const
{
    uiFileDialog filedlg( appserv().parent(), false, GetHomeDir(), "*.iv",
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
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) 
	return pdd->getCubeSampling( true )!=pdd->getCubeSampling( false );

    mDynamicCastGet(visSurvey::VolumeDisplay*, vd, obj );
    if ( vd )
	return vd->manipCenter()!=vd->center() || vd->manipWidth()!=vd->width();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*, rtd, obj );
    if ( rtd )
    {
	for ( int idx=0; idx<rtd->nrKnots(); idx++ )
	{
	    if ( rtd->getKnotPos(idx)!=rtd->getManipKnotPos(idx) )
		return true;
	}

	return rtd->getDepthInterval()!=rtd->getManipDepthInterval();
    }

    return false;
}


void uiVisPartServer::acceptManipulation( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd )
    {
	pdd->setCubeSampling( pdd->getCubeSampling(true));
	pdd->resetManip();
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*, vd, obj );
    if ( vd )
    {
	vd->setCubeSampling( vd->getCubeSampling(true) );
	vd->resetManip();
    }
}


void uiVisPartServer::setUpConnections( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj);
    if ( vd )
    {
	vd->slicemoving.notify( mCB(this,uiVisPartServer,interactionCB) );
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd )
    {
	pdd->moving.notify( mCB(this,uiVisPartServer,interactionCB) );
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,pickset,obj);
    if ( pickset )
	pickset->changed.notify( mCB( this, uiVisPartServer, interactionCB ));

}


int uiVisPartServer::addSurfTrackerCube( int sceneid )
{
    if ( !interpreterdisplay )
    {
	interpreterdisplay = visSurvey::SurfaceInterpreterDisplay::create();
	interpreterdisplay->ref();
    }

    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene->getFirstIdx(interpreterdisplay) == -1 )
	scene->addObject( interpreterdisplay );

    return interpreterdisplay->id();
}


int uiVisPartServer::getSurfTrackerCubeId()
{
    return interpreterdisplay ? interpreterdisplay->id() : 0;
}


int uiVisPartServer::addSurfEditor( int sceneid, Geometry::GridSurface& surf )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return -1;


    visBase::EditableGridSurface* surfed=visBase::EditableGridSurface::create();
    surfed->setTransformation( visSurvey::SPM().getUTM2DisplayTransform() );
    PtrMan<Executor> exec = surfed->setSurface( surf );
    uiExecutor uiexec(appserv().parent(), *exec );
    if ( !uiexec.execute() )
	return -1;

    scene->addObject( surfed );

    setUpConnections( surfed->id() );
    return surfed->id();
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
    if ( isManipulated( oldsel ) && hasAttrib(oldsel))
    {
	if ( calculateAttrib( oldsel, false ) )
	{
	    acceptManipulation( oldsel );
	}
    }

    if ( !viewmode )
	toggleDraggers();


    Threads::MutexLocker lock( eventmutex );
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::interactionCB(CallBacker*)
{
    Threads::MutexLocker lock( eventmutex );
    eventobjid = getSelObjectId();
    sendEvent( evInteraction );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb);
    if ( !cb ) return;

    int selid = getSelObjectId();
    const visBase::DataObject* dobj = visBase::DM().getObj( selid );
    mDynamicCastGet( const visSurvey::PickSetDisplay*, pickset, dobj );

    if ( selid==-1 || pickset )
    {
	Threads::MutexLocker lock( eventmutex );
	xytmousepos = scene->getMousePos(true);
	inlcrlmousepos = scene->getMousePos(false);
	mouseposval = scene->getMousePosValue();
	sendEvent( evMouseMove );
    }
}
