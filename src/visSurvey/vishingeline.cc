/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: vishingeline.cc,v 1.1 2004-04-28 09:59:54 nanne Exp $
________________________________________________________________________

-*/


#include "vishingeline.h"

#include "emhingeline.h"
#include "emsurface.h"
#include "emmanager.h"
#include "visdrawstyle.h"
#include "vispolyline.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::HingeLineDisplay );

namespace visSurvey
{

HingeLineDisplay::HingeLineDisplay()
    : hingeline( 0 )
    , transformation( 0 )
    , drawstyle( visBase::DrawStyle::create() )
{
    LineStyle ls;
    ls.width = 3;

    drawstyle->ref();
    drawstyle->setLineStyle( ls );
    addChild( drawstyle->getInventorNode() );
}


HingeLineDisplay::~HingeLineDisplay()
{
    if ( transformation ) transformation->unRef();
    for ( int idx=0; idx<polylines.size(); idx++ )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove(idx--);
    }

    if ( hingeline )
	const_cast<EM::HingeLine*>(hingeline)->changenotifier.remove(
		mCB(this,HingeLineDisplay,updateHingeLineChangeCB));

    drawstyle->unRef();
}


void HingeLineDisplay::setHingeLine( const EM::HingeLine* nhl )
{
    if ( hingeline )
	const_cast<EM::HingeLine*>(hingeline)->changenotifier.remove(
		mCB(this,HingeLineDisplay,updateHingeLineChangeCB));

    hingeline = nhl;

    if ( hingeline )
	const_cast<EM::HingeLine*>(hingeline)->changenotifier.notify(
		mCB(this,HingeLineDisplay,updateHingeLineChangeCB));

    updateHingeLineChangeCB(0);
}


bool HingeLineDisplay::setHingeLine( int emobjid )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::HingeLine*,emhl,em.getObject(emobjid))
    if ( !emhl ) return false;
    setHingeLine( emhl );
    return true;
}


void HingeLineDisplay::setTransformation( visBase::Transformation* nt)
{
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();

    for ( int idx=0; idx<polylines.size(); idx++ )
	polylines[idx]->setTransformation( nt );
}


visBase::Transformation* HingeLineDisplay::getTransformation()
{ return transformation; }



void HingeLineDisplay::updateHingeLineChangeCB(CallBacker*)
{
    if ( !hingeline ) return;

    for ( int idx=0; idx<hingeline->nrSegments(); idx++ )
    {
	if ( idx>=polylines.size() )
	{
	    visBase::PolyLine* polyline = visBase::PolyLine::create();
	    polylines += polyline;
	    polyline->ref();
	    polyline->setTransformation( transformation );
	    addChild( polyline->getInventorNode() );
	}

	visBase::PolyLine* polyline = polylines[idx];

	const int nrnodes = hingeline->nrNodes(idx);
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Coord3 pos =
		hingeline->getSurface().getPos(hingeline->getNode(idx,idy));

	    if ( idy>=polyline->size() )
		polyline->addPoint( pos );
	    else
		polyline->setPoint( idy, pos );
	}

	for ( int idy=polyline->size()-1; idy>=nrnodes; idy-- )
	{
	    polyline->removePoint(idy);
	}
    }

    for ( int idx=polylines.size()-1; idx>=hingeline->nrSegments(); idx-- )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove( idx );
    }
}

}; // namespace visSurvey
