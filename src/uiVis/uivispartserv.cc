/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.2 2002-03-28 16:02:47 nanne Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "visdataman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vismaterial.h"

#include "uimsg.h"

#include "pickset.h"
#include "survinfo.h"
#include "geompos.h"
#include "uidset.h"



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
