/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: vishingeline.cc,v 1.2 2004-07-23 13:01:40 kristofer Exp $
________________________________________________________________________

-*/


#include "vishingeline.h"

#include "emhingeline.h"
#include "emsurface.h"
#include "emmanager.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vispolyline.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::EdgeLineSetDisplay );

namespace visSurvey
{

EdgeLineSetDisplay::EdgeLineSetDisplay()
    : edgelineset( 0 )
    , transformation( 0 )
    , drawstyle( visBase::DrawStyle::create() )
{
    LineStyle ls;
    ls.width = 3;

    drawstyle->ref();
    drawstyle->setLineStyle( ls );
    addChild( drawstyle->getInventorNode() );
}


EdgeLineSetDisplay::~EdgeLineSetDisplay()
{
    if ( transformation ) transformation->unRef();
    for ( int idx=0; idx<polylines.size(); idx++ )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove(idx--);
    }

    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.remove(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));

    drawstyle->unRef();
}


void EdgeLineSetDisplay::setEdgeLineSet( const EM::EdgeLineSet* nhl )
{
    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.remove(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));

    edgelineset = nhl;

    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.notify(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));

    updateEdgeLineSetChangeCB(0);
}


bool EdgeLineSetDisplay::setEdgeLineSet( int emobjid )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::EdgeLineSet*,emhl,em.getObject(emobjid))
    if ( !emhl ) return false;
    setEdgeLineSet( emhl );
    return true;
}


void EdgeLineSetDisplay::setTransformation( visBase::Transformation* nt)
{
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();

    for ( int idx=0; idx<polylines.size(); idx++ )
	polylines[idx]->setTransformation( nt );
}


visBase::Transformation* EdgeLineSetDisplay::getTransformation()
{ return transformation; }



void EdgeLineSetDisplay::updateEdgeLineSetChangeCB(CallBacker*)
{
    if ( !edgelineset ) return;
    const EM::Surface& surface = edgelineset->getSurface();
    const EM::SectionID section = edgelineset->getSection();

    for ( int lineidx=0; lineidx<edgelineset->nrLines(); lineidx++ )
    {
	if ( lineidx>=polylines.size() )
	{
	    visBase::IndexedPolyLine3D* polyline =
				    visBase::IndexedPolyLine3D::create();
	    polylines += polyline;
	    polyline->ref();
	    polyline->setTransformation( transformation );
	    addChild( polyline->getInventorNode() );
	}

	visBase::IndexedPolyLine3D* polyline = polylines[lineidx];
	int coordindex = 0;
	int coordindexindex = 0;
	const EM::EdgeLine* edgeline = edgelineset->getLine(lineidx);

	const int nrsegments = edgeline->nrSegments();
	for ( int segmentidx=0; segmentidx<nrsegments; segmentidx++ )
	{
	    const EM::EdgeLineSegment* edgelinesegment =
					edgeline->getSegment(segmentidx);
	    const int nrnodes = edgelinesegment->size();

	    for ( int nodeidx=0; nodeidx<nrnodes; nodeidx++ )
	    {
		const RowCol& rc = (*edgelinesegment)[nodeidx];
		const Coord3 pos = surface.getPos(section,rc);
		polyline->getCoordinates()->setPos(coordindex,pos);
		polyline->setCoordIndex( coordindexindex, coordindex );
		coordindex++; coordindexindex++;
	    }

	    polyline->setCoordIndex( coordindexindex++, -1 );
	}

	polyline->removeCoordIndexAfter(coordindexindex-1);
    }

    for ( int idx=polylines.size()-1; idx>=edgelineset->nrLines(); idx-- )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove( idx );
    }
}

}; // namespace visSurvey
