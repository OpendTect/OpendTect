/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.18 2002-04-16 06:48:19 kristofer Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "visdataman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "visseisdisplay.h"
#include "visselman.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "visrectangle.h"
#include "vistexturerect.h"
#include "visobject.h"

#include "uimsg.h"

#include "pickset.h"
#include "survinfo.h"
#include "geompos.h"
#include "uidset.h"
#include "color.h"
#include "colortab.h"

#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "thread.h"

const int uiVisPartServer::evShowPosition   	= 0;
const int uiVisPartServer::evSelection		= 1;
const int uiVisPartServer::evDeSelection	= 2;
const int uiVisPartServer::evPicksChanged    	= 3;


uiVisPartServer::uiVisPartServer( uiApplService& a )
	: uiApplPartServer(a)
        , viewmode( false )
        , eventmutex( *new Threads::Mutex )
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
    delete &eventmutex;
}


bool uiVisPartServer::deleteAllObjects()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
        scenes[idx]->unRef();

    scenes.erase();

    return visBase::DM().reInit();
}


void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );

    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
	mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj);
	if ( sd ) sd->showDraggers(true);
    }
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


uiVisPartServer::ObjectType uiVisPartServer::getObjectType( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SeisDisplay*,sd,dobj)
    if ( sd ) return DataDisplay;
    mDynamicCastGet(const visSurvey::PickSetDisplay*, psd, dobj );
    if ( psd ) return PickSetDisplay;

    return Unknown;
}


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return;
    obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return "";
    return obj->name();
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    so->turnOn( yn );
}


bool uiVisPartServer::isOn( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    return so->isOn();
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;

    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj);

    if ( sd ) sd->showDraggers(!yn);
}

    
int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    scenes += newscene;
    newscene->ref();
    selsceneid = newscene->id();
    return selsceneid;
}


visSurvey::Scene* uiVisPartServer::getScene(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene;
}


int uiVisPartServer::addDataDisplay( uiVisPartServer::ElementType etp )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::SeisDisplay::Type type =
	    etp == Inline ?	visSurvey::SeisDisplay::Inline
	: ( etp == Crossline ?	visSurvey::SeisDisplay::Crossline
			     :	visSurvey::SeisDisplay::Timeslice );

    visSurvey::SeisDisplay* sd = visSurvey::SeisDisplay::create( type, *scene,
				     mCB( this, uiVisPartServer, getDataCB ));
    seisdisps += sd; 
    visBase::VisColorTab* coltab = visBase::VisColorTab::create();
    coltab->colorSeq().loadFromStorage("Red-White-Black");
    sd->textureRect().setColorTab( coltab );
    sd->textureRect().manipChanges()->notify(
	    				mCB(this,uiVisPartServer,showPosCB));

    scene->addInlCrlTObject( sd );
    setSelObjectId( sd->id() );
    return getSelObjectId();
}


void uiVisPartServer::removeDataDisplay( int id )
{

    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,sdobj)
    if ( !sd ) return;

    sdobj->deSelect();
    sd->textureRect().manipChanges()->remove(
	    				mCB(this,uiVisPartServer,showPosCB));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( sd );
    scene->removeObject( objidx );
}


void uiVisPartServer::resetManipulation( int id )
{
    visBase::DataObject* sdobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,sdobj)
    if ( !sd ) return;

    sd->resetManip();
}


float uiVisPartServer::getPlanePos(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    if ( !sd ) return 0;
    Geometry::Pos geompos = sd->textureRect().getRectangle().manipOrigo();
    return	sd->getType()==visSurvey::SeisDisplay::Inline ? geompos.x :
		sd->getType()==visSurvey::SeisDisplay::Crossline ? geompos.y:
		geompos.z;
}


void uiVisPartServer::setAttribSelSpec( int id, AttribSelSpec& as )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->setAttribSelSpec( as );
}


CubeSampling& uiVisPartServer::getCubeSampling( int id, bool manippos )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getCubeSampling( manippos );
}


AttribSelSpec& uiVisPartServer::getAttribSelSpec(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getAttribSelSpec();
}


void uiVisPartServer::putNewData( int id, AttribSlice* slice )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->operationSucceeded( sd->putNewData(slice) );
}


int uiVisPartServer::addPickSetDisplay()
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj);

    visSurvey::PickSetDisplay* pickset =
				visSurvey::PickSetDisplay::create( *scene );
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
    for ( int idx=0; idx<visps->nrPicks(); idx++ )
    {
	Geometry::Pos pos = visps->getPick( idx );
	pickset += PickLocation( pos.x, pos.y, pos.z );
    }
}


int uiVisPartServer::nrPicks( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->nrPicks() : 0;
}

/*
 *
TODO Should change to addColorSequence
void uiVisPartServer::setColorSeq(int id, const ColorTable& ctab )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().getColorTab().colorSeq().colors() = ctab;
    sd->textureRect().getColorTab().colorSeq().colorsChanged();
}
*/


const ColorTable& uiVisPartServer::getColorSeq(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().getColorTab().colorSeq().colors();
}


Color uiVisPartServer::getColor(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial()->getColor();
}


void uiVisPartServer::setColor( int id, const Color& col )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    vo->getMaterial()->setColor( col );
}


visBase::Material* uiVisPartServer::getMaterial(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial();
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setClipRate( cr );
}


float uiVisPartServer::getClipRate(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().clipRate();
}


void uiVisPartServer::setAutoscale( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setAutoscale( yn );
}


bool uiVisPartServer::getAutoscale(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().autoScale();
}


void uiVisPartServer::setDataRange( int id, const Interval<float>& intv )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().getColorTab().scaleTo( intv );
}


Interval<float> uiVisPartServer::getDataRange(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().getColorTab().getInterval();
}





















void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( sel );
	mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj);
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
	mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj);
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


void uiVisPartServer::showPosCB( CallBacker* )
{
    Threads::MutexLocker lock( eventmutex );
    eventobjid = getSelObjectId();
    sendEvent( evShowPosition );
}


