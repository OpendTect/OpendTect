/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.3 2002-03-29 17:26:42 nanne Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "visdataman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vismaterial.h"
#include "visobject.h"

#include "uimsg.h"

#include "pickset.h"
#include "survinfo.h"
#include "geompos.h"
#include "uidset.h"
#include "color.h"



uiVisPartServer::uiVisPartServer( uiApplService& a )
	: uiApplPartServer(a)
{
}


uiVisPartServer::~uiVisPartServer()
{
}


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    scenes += newscene;
    newscene->ref();
    selsceneid = newscene->id();
    return selsceneid;
}


int uiVisPartServer::addPickSetDisplay()
{
    visSurvey::PickSet* pickset = visSurvey::PickSet::create();
    picks += pickset;
    pickset->ref();
    scenes[selsceneid]->addInlCrlTObject( pickset );
    selobjid = pickset->id();
    return selobjid;
}


void uiVisPartServer::removePickSetDisplay()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visSurvey::PickSet*,ps,obj)
    if ( !ps ) return;

    ps->unRef();
    int objidx = scenes[selsceneid]->getFirstIdx( ps );
    scenes[selsceneid]->removeObject( objidx );
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
	BinID bid( pos.x, pos.y );
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
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    so->getMaterial()->setColor( col );
}


Color uiVisPartServer::getColor()
{
    visBase::DataObject* obj = visBase::DM().getObj( selobjid );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    return so->getMaterial()->getColor();
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
