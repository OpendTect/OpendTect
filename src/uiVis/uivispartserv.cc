/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.12 2002-04-11 14:34:58 nanne Exp $
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

const int uiVisPartServer::evShowPosition   	= 0;
const int uiVisPartServer::evSelectionChange    = 1;
const int uiVisPartServer::evPicksChanged    	= 2;


uiVisPartServer::uiVisPartServer( uiApplService& a, const CallBack appcb_ )
	: uiApplPartServer(a)
	, appcb(appcb_)
	, planepos(0)
{
    visBase::DM().selMan().selnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
}


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifer.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );

    for ( int idx=0; idx<scenes.size(); idx++ )
	scenes[idx]->unRef();
}


bool uiVisPartServer::deleteAllObjects()
{
    return visBase::DM().reInit();
}


void uiVisPartServer::setObjectName( const char* nm, int id )
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


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    scenes += newscene;
    newscene->ref();
    selsceneid = newscene->id();
    return selsceneid;
}


visSurvey::Scene* uiVisPartServer::getSelScene()
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene;
}


int uiVisPartServer::addDataDisplay( uiVisPartServer::ElementType etp )
{
    visSurvey::SeisDisplay::Type type =
	    etp == Inline ?	visSurvey::SeisDisplay::Inline
	: ( etp == Crossline ?	visSurvey::SeisDisplay::Crossline
			     :	visSurvey::SeisDisplay::Timeslice );

    visSurvey::SeisDisplay* sd = visSurvey::SeisDisplay::create( type, appcb );
    seisdisps += sd; 
    visBase::VisColorTab* coltab = visBase::VisColorTab::create();
    coltab->colorSeq().loadFromStorage("Red-White-Black");
    sd->textureRect().setColorTab( coltab );
    sd->textureRect().manipChanges()->notify(
	    				mCB(this,uiVisPartServer,showPosCB));

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->addInlCrlTObject( sd );
    setSelObjectId( sd->id() );
    return getSelObjectId();
}


void uiVisPartServer::removeDataDisplay()
{

    visBase::DataObject* sdobj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,sdobj)
    if ( !sd ) return;

    sd->textureRect().manipChanges()->remove(
	    				mCB(this,uiVisPartServer,showPosCB));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( sd );
    scene->removeObject( objidx );
}


void uiVisPartServer::selectObjCB( CallBacker* )
{
    sendEvent( evSelectionChange );
}


void uiVisPartServer::picksChangedCB(CallBacker*)
{
    sendEvent( evPicksChanged );
}


void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );
}


int uiVisPartServer::getSelObjectId() const
{
    return visBase::DM().selMan().selected()[0];
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


void uiVisPartServer::removePickSetDisplay()
{
    visBase::DataObject* psobj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,psobj)
    if ( !ps ) return;

    ps->changed.remove( mCB( this, uiVisPartServer, picksChangedCB ));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( ps );
    scene->removeObject( objidx );
}


bool uiVisPartServer::setPicks( const PickSet& pickset )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
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


void uiVisPartServer::getPickSets( UserIDSet& pset )
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


void uiVisPartServer::setColor( const Color& col )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    vo->getMaterial()->setColor( col );
}


Color uiVisPartServer::getColor() const
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial()->getColor();
}

visBase::Material* uiVisPartServer::getMaterial()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial();
}


float uiVisPartServer::getClipRate()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().clipRate();
}


void uiVisPartServer::setClipRate( float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setClipRate( cr );
}


bool uiVisPartServer::getAutoscale()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().autoScale();
}


void uiVisPartServer::setAutoscale( bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setAutoscale( yn );
}


void uiVisPartServer::setDataRange( const Interval<float>& intv )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().getColorTab().scaleTo( intv );
}


Interval<float> uiVisPartServer::getDataRange()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().getColorTab().getInterval();
}


void uiVisPartServer::setColorSeq( const ColorTable& ctab )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().getColorTab().colorSeq().colors() = ctab;
    sd->textureRect().getColorTab().colorSeq().colorsChanged();
}

const ColorTable& uiVisPartServer::getColorSeq()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().getColorTab().colorSeq().colors();
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


uiVisPartServer::ObjectType uiVisPartServer::getObjectType( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::SeisDisplay*,sd,dobj)
    if ( sd ) return DataDisplay;
    mDynamicCastGet(const visSurvey::PickSetDisplay*, psd, dobj );
    if ( psd ) return PickSetDisplay;

    return Unknown;
}


CubeSampling& uiVisPartServer::getCubeSampling( bool manippos )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getCubeSampling( manippos );
}


void uiVisPartServer::setAttribSelSpec( AttribSelSpec& as )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->setAttribSelSpec( as );
}


AttribSelSpec& uiVisPartServer::getAttribSelSpec()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getAttribSelSpec();
}


void uiVisPartServer::putNewData( AttribSlice* slice )
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->operationSucceeded( sd->putNewData(slice) );
}


void uiVisPartServer::showPosCB( CallBacker* )
{
    sendEvent( evShowPosition );
}


float uiVisPartServer::getPlanePos()
{
    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    if ( !sd ) return 0;
    Geometry::Pos geompos = sd->textureRect().getRectangle().manipOrigo();
    planepos = sd->getType() == visSurvey::SeisDisplay::Inline ? geompos.x :
	       sd->getType() == visSurvey::SeisDisplay::Crossline ? geompos.y :
	       geompos.z;
    return planepos; 
}
