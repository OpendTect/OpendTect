/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.54 2002-05-23 08:25:31 kristofer Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "visdataman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vissurvwell.h"
#include "vissurvhorizon.h"
#include "visplanedatadisplay.h"
#include "visselman.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "visrectangle.h"
#include "vistexturerect.h"
#include "visobject.h"
#include "errh.h"

#include "uimsg.h"

#include "pickset.h"
#include "survinfo.h"
#include "geompos.h"
#include "uidset.h"
#include "color.h"
#include "colortab.h"
#include "settings.h"

#include "uizscaledlg.h"
#include "uimaterialdlg.h"
#include "uipickszdlg.h"
#include "uislicesel.h"

#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "thread.h"

const int uiVisPartServer::evManipulatorMove   	= 0;
const int uiVisPartServer::evSelection		= 1;
const int uiVisPartServer::evDeSelection	= 2;
const int uiVisPartServer::evPicksChanged    	= 3;
const int uiVisPartServer::evGetNewData    	= 4;
const int uiVisPartServer::evSelectableStatusCh = 5;
const int uiVisPartServer::evMouseMove		= 6;

const char* uiVisPartServer::appvelstr = "AppVel";

uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
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


bool uiVisPartServer::deleteAllObjects()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	scenes[idx]->mouseposchange.remove(
	    			mCB( this, uiVisPartServer, mouseMoveCB ));
        scenes[idx]->unRef();
    }

    scenes.erase();
    picks.erase();
    seisdisps.erase();

    return visBase::DM().reInit();
}


void uiVisPartServer::usePar( const IOPar& par )
{
    if ( !visBase::DM().usePar( par ) )
	pErrMsg( "Could not parse session");

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
	    	(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );

	newscene->mouseposchange.notify(
				mCB( this, uiVisPartServer, mouseMoveCB ));
	scenes += newscene;
	newscene->ref();
	selsceneid = newscene->id();
    }

    TypeSet<int> pickids;
    visBase::DM().getIds( typeid(visSurvey::PickSetDisplay), pickids );

    for ( int idx=0; idx<pickids.size(); idx++ )
    {
	visSurvey::PickSetDisplay* pickset =
	    (visSurvey::PickSetDisplay*) visBase::DM().getObj( pickids[idx] );	

	picks += pickset;
	pickset->changed.notify( mCB( this, uiVisPartServer, picksChangedCB ));
    }


    TypeSet<int> seisdispids;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), seisdispids );

    for ( int idx=0; idx<seisdispids.size(); idx++ )
    {
	visSurvey::PlaneDataDisplay* sd =
	   (visSurvey::PlaneDataDisplay*)visBase::DM().getObj(seisdispids[idx]);
	seisdisps += sd;

	sd->getMovementNotification()->notify(
				    mCB(this,uiVisPartServer,getDataCB) );

	sd->textureRect().manipChanges()->notify(
			    mCB(this,uiVisPartServer,manipMoveCB));

	getDataCB( sd );

    }

    float appvel;
    if ( par.get( appvelstr, appvel ) )
	visSurvey::SPM().setAppVel( appvel );

}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes.size(); idx++ )
	storids += scenes[idx]->id();

    par.set( appvelstr, visSurvey::SPM().getAppVel() );

    visBase::DM().fillPar(par, storids);
}


uiVisPartServer::ObjectType uiVisPartServer::getObjectType( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,sd,dobj)
    if ( sd ) return DataDisplay;
    mDynamicCastGet(const visSurvey::PickSetDisplay*, psd, dobj );
    if ( psd ) return PickSetDisplay;
    mDynamicCastGet(const visSurvey::WellDisplay*, well, dobj );
    if ( well ) return WellDisplay;
    mDynamicCastGet(const visSurvey::HorizonDisplay*, hor, dobj );
    if ( well ) return HorizonDisplay;


    return Unknown;
}


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return 0;
    return obj->name();
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    if ( so ) so->turnOn( yn );
}


bool uiVisPartServer::isOn( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    return so ? so->isOn() : false;
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;

    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);

    if ( sd ) sd->showDraggers(!yn);
}


void uiVisPartServer::showAnnotText( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnotText( yn );
}


bool uiVisPartServer::isAnnotTextShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotTextShown() : false;
}


void uiVisPartServer::showAnnotScale( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnotScale( yn ); 
} 


bool uiVisPartServer::isAnnotScaleShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotScaleShown() : false;
}


void uiVisPartServer::showAnnot( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnot( yn );
}


bool uiVisPartServer::isAnnotShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotShown() : false;
}


bool uiVisPartServer::isSelectable( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    return obj->selectable();
}


void uiVisPartServer::makeSelectable(int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,visobj,obj);
    if ( !visobj ) return;

    if ( getSelObjectId() == id ) setSelObjectId( -1 );
    visobj->setSelectable( yn );

    Threads::MutexLocker lock( eventmutex );
    eventobjid = id;
    sendEvent( evSelectableStatusCh );
}

    
void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );

    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
	if ( sd ) sd->showDraggers(true);
    }
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB( this, uiVisPartServer, mouseMoveCB ));
    scenes += newscene;
    newscene->ref();
    selsceneid = newscene->id();
    return selsceneid;
}


void uiVisPartServer::removeScene( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->mouseposchange.remove(
		mCB( this, uiVisPartServer, mouseMoveCB ) );
    scene->unRef();
    scenes -= scene;
}


void uiVisPartServer::getSceneIds( TypeSet<int>& sceneids )
{
    sceneids.erase();
    for ( int idx=0; idx<scenes.size(); idx++ )
	sceneids += scenes[idx]->id();
}


int uiVisPartServer::addDataDisplay( uiVisPartServer::ElementType etp )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::PlaneDataDisplay::Type type =
	    etp == Inline ?	visSurvey::PlaneDataDisplay::Inline
	: ( etp == Crossline ?	visSurvey::PlaneDataDisplay::Crossline
			     :	visSurvey::PlaneDataDisplay::Timeslice );

    visSurvey::PlaneDataDisplay* sd = visSurvey::PlaneDataDisplay::create();
    sd->setType( type );
    sd->getMovementNotification()->notify(
			    mCB(this,uiVisPartServer,getDataCB) );

    seisdisps += sd; 
    visBase::VisColorTab* coltab = visBase::VisColorTab::create();
    BufferString ctname = "Red-White-Black";
    mSettUse(get,"dTect.Color table","Name",ctname);
    coltab->colorSeq().loadFromStorage( ctname );
    sd->textureRect().setColorTab( coltab );
    sd->textureRect().manipChanges()->notify(
	    				mCB(this,uiVisPartServer,manipMoveCB));

    scene->addInlCrlTObject( sd );
    setSelObjectId( sd->id() );
    return getSelObjectId();
}


void uiVisPartServer::removeDataDisplay( int id )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,sdobj)
    if ( !sd ) return;

    sdobj->deSelect();
    sd->textureRect().manipChanges()->remove(
	    				mCB(this,uiVisPartServer,manipMoveCB));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( sd );
    scene->removeObject( objidx );
    seisdisps -= sd;
}


void uiVisPartServer::resetManipulation( int id )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,sdobj)
    if ( !sd ) return;

    sd->resetManip();
}


void uiVisPartServer::setPlanePos( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( !sd ) return;
    CubeSampling* cs = &sd->getCubeSampling( true );
    uiSliceSel dlg( appserv().parent(), cs );
    if ( dlg.go() )
    {
	Geometry::Pos width( cs->hrg.stop.inl - cs->hrg.start.inl, 
			     cs->hrg.stop.crl - cs->hrg.start.crl,
			     cs->zrg.stop - cs->zrg.start );
	sd->setWidth( width );
	Geometry::Pos origo(cs->hrg.start.inl,cs->hrg.start.crl,cs->zrg.start);
	sd->setOrigo( origo );
	sd->updateAtNewPos();
	sendEvent( evManipulatorMove );
	sendEvent( evSelection );
    }
}


float uiVisPartServer::getPlanePos( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( !sd ) return 0;
    Geometry::Pos geompos = sd->textureRect().getRectangle().manipOrigo();
    return   sd->getType()==visSurvey::PlaneDataDisplay::Inline ? geompos.x :
	     sd->getType()==visSurvey::PlaneDataDisplay::Crossline ? geompos.y :
	     geompos.z;
}


bool uiVisPartServer::isPlaneManipulated( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
    if ( !sd ) return false;

    Geometry::Pos manippos = sd->textureRect().getRectangle().manipOrigo();
    Geometry::Pos rectpos = sd->textureRect().getRectangle().origo();
    if ( manippos!=rectpos ) return true;
    return false;
}


void uiVisPartServer::setAttribSelSpec( int id, AttribSelSpec& as )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( sd ) sd->setAttribSelSpec( as );
}


CubeSampling& uiVisPartServer::getCubeSampling( int id, bool manippos )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd->getCubeSampling( manippos );
}


AttribSelSpec& uiVisPartServer::getAttribSelSpec(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd->getAttribSelSpec();
}


void uiVisPartServer::putNewData( int id, AttribSlice* slice )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( sd ) sd->operationSucceeded( sd->putNewData(slice) );
}


void uiVisPartServer::getDataDisplayIds( int sceneid, 
	uiVisPartServer::ElementType etp, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    visSurvey::PlaneDataDisplay::Type type =
	    etp == Inline ?	visSurvey::PlaneDataDisplay::Inline
	: ( etp == Crossline ?	visSurvey::PlaneDataDisplay::Crossline
			     :	visSurvey::PlaneDataDisplay::Timeslice );

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
	if ( !sd ) continue;
	if ( type == sd->getType() )
	    ids += sd->id();
    }
}


int uiVisPartServer::addPickSetDisplay()
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj);

    visSurvey::PickSetDisplay* pickset =
				visSurvey::PickSetDisplay::create();
    picks += pickset;
    scene->addXYTObject( pickset );
    setSelObjectId( pickset->id() );
    pickset->changed.notify( mCB( this, uiVisPartServer, picksChangedCB ));
    return pickset->id();
}


void uiVisPartServer::removePickSetDisplay(int id)
{
    visBase::DataObject* psobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,psobj)
    if ( !ps ) return;

    ps->changed.remove( mCB( this, uiVisPartServer, picksChangedCB ));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( ps );
    scene->removeObject( objidx );
    picks -= ps;
}


bool uiVisPartServer::setPicks(int id, const PickSet& pickset )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( !ps ) return false;

    if ( ps->nrPicks() )
	ps->removeAll();

    ps->getMaterial()->setColor( pickset.color );
    ps->setName( pickset.name() );
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	Coord crd( pickset[idx].pos );
	ps->addPick( Geometry::Pos(crd.x,crd.y,pickset[idx].z) );
    }

    return true;
}


void uiVisPartServer::getAllPickSets( UserIDSet& pset )
{
    for ( int idx=0; idx<picks.size(); idx++ )
    {
	if ( !picks[idx]->nrPicks() ) continue;
	pset.add( picks[idx]->name() );
    }
}


void uiVisPartServer::getPickSetData( const char* nm, PickSet& pickset )
{
    visSurvey::PickSetDisplay* visps = 0;
    for ( int idx=0; idx<picks.size(); idx++ )
    {
	if ( !strcmp(nm,picks[idx]->name()) )
	{
	    visps = picks[idx];
	    break;
	}
    }

    if ( !visps )
    {
	BufferString msg( "Cannot find PickSet " ); msg += nm;
	uiMSG().error( msg );
	return;
    }

    pickset.color = visps->getMaterial()->getColor();
    pickset.color.setTransparency( 0 );
    for ( int idx=0; idx<visps->nrPicks(); idx++ )
    {
	Geometry::Pos pos = visps->getPick( idx );
	pickset += PickLocation( pos.x, pos.y, pos.z );
    }
}


void uiVisPartServer::getPickSetIds( int sceneid, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,obj)
	if ( psd )
	    ids += psd->id();
    }
}


int uiVisPartServer::nrPicks( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->nrPicks() : 0;
}


bool uiVisPartServer::picksetIsPresent( const char* psname )
{
    for ( int idx=0; idx<picks.size(); idx++ )
    {
        if ( !strcmp(psname,picks[idx]->name()) )
        {
	    BufferString msg( "Pickset: "); msg += psname;
	    msg += "\nis already present.";
	    uiMSG().about( msg );
	    return true;
        }
    }

    return false;
}


void uiVisPartServer::showAllPicks( int id, bool showall )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( ps )
        ps->showAll( showall );

    if ( !showall )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	    scenes[idx]->filterPicks();
    }
}


bool uiVisPartServer::allPicksShown( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->allShown() : false;
}


int uiVisPartServer::addWellDisplay( const MultiID& emwellid )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::WellDisplay* welldisplay = visSurvey::WellDisplay::create();
    if ( !welldisplay->setWellId( emwellid ) )
    {
	welldisplay->ref(); welldisplay->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    wells += welldisplay; 

    if ( welldisplay->depthIsT() )
	scene->addXYTObject( welldisplay );
    else
	scene->addXYZObject( welldisplay );

    setSelObjectId( welldisplay->id() );
    return welldisplay->id();
}


void uiVisPartServer::removeWellDisplay( int id )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return;

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( well );
    if ( objidx>0 )
    {
	scene->removeObject( objidx );
	wells -= well;
    }
}


int uiVisPartServer::getNrWellAttribs( int id ) const 
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return -1;

    return well->nrAttribs();
}


const char* uiVisPartServer::getWellAttribName( int id, int attr ) const 
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return 0;

    return well->getAttribName( attr );
}


void uiVisPartServer::displayWellAttrib( int id, int attr )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return;

    well->displayAttrib( attr );
}


int uiVisPartServer::displayedWellAttrib( int id ) const
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return -1;

    return well->displayedAttrib( );
}


const LineStyle* uiVisPartServer::wellLineStyle( int id ) const
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return 0;

    return &well->lineStyle();
}


void uiVisPartServer::setWellLineStyle( int id, const LineStyle& lst )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,well,sdobj)
    if ( !well ) return;

    well->setLineStyle( lst );
}


int uiVisPartServer::addHorizonDisplay( const MultiID& emhorid )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::HorizonDisplay* horizon = visSurvey::HorizonDisplay::create();

    if ( !horizon->setHorizonId( emhorid ) )
    {
	horizon->ref(); horizon->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    horizons += horizon; 

    if ( horizon->depthIsT() )
	scene->addXYTObject( horizon );
    else
	scene->addXYZObject( horizon );

    setSelObjectId( horizon->id() );
    return horizon->id();
}


void uiVisPartServer::removeHorizonDisplay( int id )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::HorizonDisplay*,hor,sdobj)
    if ( !hor ) return;

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( hor );
    if ( objidx>0 )
    {
	scene->removeObject( objidx );
	horizons -= hor;
    }
}


bool uiVisPartServer::canSetColorSeq( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
    return sd;
}


void uiVisPartServer::modifyColorSeq(int id, const ColorTable& ctab )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
    if ( !sd ) return;

    sd->textureRect().getColorTab().colorSeq().colors() = ctab;
    sd->textureRect().getColorTab().colorSeq().colorsChanged();
}


const ColorTable& uiVisPartServer::getColorSeq(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
    return sd->textureRect().getColorTab().colorSeq().colors();
}


void uiVisPartServer::shareColorSeq( int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,tosd,obj);
    if ( !tosd ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,fromsd,obj);
    if ( !fromsd ) return;

    visBase::ColorSequence* colseq =
				&fromsd->textureRect().getColorTab().colorSeq();
    tosd->textureRect().getColorTab().setColorSeq( colseq );
}


bool uiVisPartServer::canSetRange(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( sd ) sd->textureRect().setClipRate( cr );
}


float uiVisPartServer::getClipRate(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd ? sd->textureRect().clipRate() : 0;
}


void uiVisPartServer::setAutoscale( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( sd ) sd->textureRect().setAutoscale( yn );
}


bool uiVisPartServer::getAutoscale(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd ? sd->textureRect().autoScale() : false;
}


void uiVisPartServer::setDataRange( int id, const Interval<float>& intv )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    if ( sd ) sd->textureRect().getColorTab().scaleTo( intv );
}


Interval<float> uiVisPartServer::getDataRange(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj)
    return sd->textureRect().getColorTab().getInterval();
}


void uiVisPartServer::shareRange( int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,tosd,obj);
    if ( !tosd ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,fromsd,obj);
    if ( !fromsd ) return;

    visBase::VisColorTab* coltab = &fromsd->textureRect().getColorTab();
    tosd->textureRect().setColorTab(coltab);
}


bool uiVisPartServer::canSetColor(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    return vo;
}


void uiVisPartServer::modifyColor( int id, const Color& col )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    if ( vo ) vo->getMaterial()->setColor(col);
}


Color uiVisPartServer::getColor(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    return vo->getMaterial()->getColor();
}


void uiVisPartServer::shareColor(int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visBase::VisualObject*,tovo,obj);
    if ( !tovo ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visBase::VisualObject*,fromvo,obj);
    if ( !fromvo ) return;

    visBase::Material* material = fromvo->getMaterial();
    tovo->setMaterial(material);
}


bool uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.go();
    return dlg.valueChanged();
}


void uiVisPartServer::setMaterial( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    if ( !vo ) return;

    uiMaterialDlg dlg( appserv().parent(), vo->getMaterial() );
    dlg.go();
}


void uiVisPartServer::setPickSize( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( !ps ) return;

    uiPickSizeDlg dlg( appserv().parent(), ps );
    dlg.go();
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( sel );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
	if ( sd ) sd->showDraggers(true);
    }

    Threads::MutexLocker lock( eventmutex );
    eventobjid = sel;
    sendEvent( evSelection );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( oldsel );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,obj);
	if ( sd ) sd->showDraggers(false);
    }

    Threads::MutexLocker lock( eventmutex );
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::picksChangedCB(CallBacker*)
{
    Threads::MutexLocker lock( eventmutex );
    eventobjid = getSelObjectId();
    sendEvent( evPicksChanged );
}


void uiVisPartServer::manipMoveCB( CallBacker* )
{
    int id = getSelObjectId();
    if ( id<0 ) return;

    Threads::MutexLocker lock( eventmutex );
    eventobjid = id;
    sendEvent( evManipulatorMove );
}


void uiVisPartServer::getDataCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,sd,cb);
    if ( !sd ) return;

    Threads::MutexLocker lock( eventmutex );
    eventobjid = sd->id();
    sendEvent( evGetNewData );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb);
    if ( !cb ) return;

    int selid = getSelObjectId();
    if ( selid==-1 || getObjectType(selid)==PickSetDisplay)
    {
	Threads::MutexLocker lock( eventmutex );
	xytmousepos = scene->getMousePos(true);
	inlcrlmousepos = scene->getMousePos(false);
	mouseposval = scene->getMousePosValue();
	sendEvent( evMouseMove );
    }
}
