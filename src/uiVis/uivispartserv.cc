/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.6 2002-04-09 13:26:14 nanne Exp $
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

#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"

const int uiVisPartServer::evShowPosition   = 0;
const int uiVisPartServer::evSelectPlane    = 1;
const int uiVisPartServer::evDeselectPlane  = 2;


uiVisPartServer::uiVisPartServer( uiApplService& a, const CallBack appcb_ )
	: uiApplPartServer(a)
	, appcb(appcb_)
	, planepos(0)
{
}


uiVisPartServer::~uiVisPartServer()
{
}


bool uiVisPartServer::deleteAllObjects()
{
    return visBase::DM().reInit();
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
    seisdisps += sd; sd->ref();
    visBase::VisColorTab* coltab = visBase::VisColorTab::create();
    coltab->colorSeq().loadFromStorage("Red-White-Black");
    sd->textureRect().setColorTab( coltab );
    sd->textureRect().manipChanges()->notify(mCB(this,uiVisPartServer,showPos));
    sd->selection()->notify( mCB(this,uiVisPartServer,selectObj) );

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->addInlCrlTObject( sd );
    selobjid = sd->id();
    return selobjid;
}


void uiVisPartServer::removeDataDisplay()
{
    visBase::DataObject* sdobj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,sdobj)
    if ( !sd ) return;

    sd->unRef();
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( sd );
    scene->removeObject( objidx );
}


void uiVisPartServer::selectObj( CallBacker* )
{
    visBase::DataObject* dobj = const_cast<visBase::DataObject*>
	(visBase::DM().selMan().getSelDataObject(0));
    dobj = visBase::DM().getObj( dobj->id() + 4 );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,dobj)
    if ( !sd ) return;
    setSelObjectId( sd->id() );
    sendEvent( evSelectPlane );
}


void uiVisPartServer::setSelObjectId( int id )
{
    selobjid = id;
    visBase::DM().selMan().select( id );
}


//  ============ Pick management =========================

int uiVisPartServer::addPickSetDisplay()
{
    visSurvey::PickSet* pickset = visSurvey::PickSet::create();
    picks += pickset;
    pickset->ref();
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->addInlCrlTObject( pickset );
    selobjid = pickset->id();
    return selobjid;
}


void uiVisPartServer::removePickSetDisplay()
{
    visBase::DataObject* psobj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::PickSet*,ps,psobj)
    if ( !ps ) return;

    ps->unRef();
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( ps );
    scene->removeObject( objidx );
}


bool uiVisPartServer::setPicks( const PickSet& pickset )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::PickSet*,ps,obj)
    if ( !ps ) return false;

    if ( ps->nrPicks() )
	ps->removeAll();

    ps->getMaterial()->setColor( pickset.color );
    ps->setName( pickset.name() );
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	Coord crd( pickset[idx].pos );
	BinID bid = SI().transform( crd );
	float z = (float)pickset[idx].z;
	ps->addPick( Geometry::Pos(bid.inl,bid.crl,z) );
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
    visSurvey::PickSet* visps = 0;
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
	BinID bid( mNINT(pos.x), mNINT(pos.y) );
	Coord crd = SI().transform( bid );
	pickset += PickLocation( crd, pos.z );
    }
}


int uiVisPartServer::nrPicks( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSet*,ps,obj)
    return ps ? ps->nrPicks() : 0;
}


// =============== Material management ===================


void uiVisPartServer::setColor( const Color& col )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    vo->getMaterial()->setColor( col );
}


Color uiVisPartServer::getColor() const
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial()->getColor();
}

visBase::Material* uiVisPartServer::getMaterial()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visBase::VisualObject*,vo,obj)
    return vo->getMaterial();
}


// ============= DataRange management ================================

float uiVisPartServer::getClipRate()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().clipRate();
}


void uiVisPartServer::setClipRate( float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setClipRate( cr );
}


bool uiVisPartServer::getAutoscale()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().autoScale();
}


void uiVisPartServer::setAutoscale( bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().setAutoscale( yn );
}


void uiVisPartServer::setDataRange( const Interval<float>& intv )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->textureRect().getColorTab().scaleTo( intv );
}


Interval<float> uiVisPartServer::getDataRange()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->textureRect().getColorTab().getInterval();
}


// ============= Various ================================

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


CubeSampling& uiVisPartServer::getCubeSampling( bool manippos )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getCubeSampling( manippos );
}


void uiVisPartServer::setAttribSelSpec( AttribSelSpec& as )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->setAttribSelSpec( as );
}


AttribSelSpec& uiVisPartServer::getAttribSelSpec()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    return sd->getAttribSelSpec();
}


void uiVisPartServer::putNewData( AttribSlice* slice )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    sd->operationSucceeded( sd->putNewData(slice) );
}


void uiVisPartServer::showPos( CallBacker* )
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::SeisDisplay*,sd,obj)
    if ( !sd ) return;
    Geometry::Pos geompos = sd->textureRect().getRectangle().manipOrigo();
    planepos = sd->getType() == visSurvey::SeisDisplay::Inline ? geompos.x :
	       sd->getType() == visSurvey::SeisDisplay::Crossline ? geompos.y :
	       geompos.z;
    sendEvent( evShowPosition );
}


float uiVisPartServer::getPlanePos()
{
    return planepos; 
}
