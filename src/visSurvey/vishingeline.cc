/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: vishingeline.cc,v 1.3 2004-07-28 06:47:58 kristofer Exp $
________________________________________________________________________

-*/


#include "vishingeline.h"

#include "emhingeline.h"
#include "emsurface.h"
#include "emmanager.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistransform.h"
#include "trackingedgeline.h"

mCreateFactoryEntry( visSurvey::EdgeLineSetDisplay );

namespace visSurvey
{

#define mStopColor	1
#define mCutColor	2
#define mConnectColor	3

EdgeLineSetDisplay::EdgeLineSetDisplay()
    : edgelineset( 0 )
    , transformation( 0 )
{
    getMaterial()->setColor( Color(255,255,0), 0 );
    getMaterial()->setColor( Color(255,128,0), mCutColor );
    getMaterial()->setColor( Color(255,0,0), mStopColor );
    getMaterial()->setColor( Color(0,255,0), mConnectColor );
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
	RowCol prevrc; bool defprevrc = false;
	int firstindex = -1;
	for ( int segmentidx=0; segmentidx<nrsegments; segmentidx++ )
	{
	    const EM::EdgeLineSegment* edgelinesegment =
					edgeline->getSegment(segmentidx);
	    int materialindex = 0;
	    if ( dynamic_cast<const Tracking::TerminationEdgeLineSegment*>(
						    edgelinesegment) )
		materialindex = mStopColor;
	    else if ( dynamic_cast<const Tracking::SurfaceConnectLine*>
						    (edgelinesegment) )
		materialindex = mConnectColor;
	    else if ( dynamic_cast<const Tracking::SurfaceCutLine*>
						    (edgelinesegment) )
		materialindex = mCutColor;

	    const int nrnodes = edgelinesegment->size();
	    for ( int nodeidx=0; nodeidx<nrnodes; nodeidx++ )
	    {
		const RowCol& rc = (*edgelinesegment)[nodeidx];
		if ( defprevrc && !rc.isNeighborTo(prevrc, surface.step()) )
		{
		    polyline->setCoordIndex( coordindexindex++, -1 );
		    defprevrc = false;
		    continue;
		}

		if ( firstindex==-1 )
		    firstindex = coordindexindex;

		const Coord3 pos = surface.getPos(section,rc);
		if ( !pos.isDefined() )
		{
		    polyline->setCoordIndex( coordindexindex++, -1 );
		    defprevrc = false;
		    continue;
		}

		polyline->getCoordinates()->setPos(coordindex,pos);
		polyline->setCoordIndex( coordindexindex, coordindex );
		polyline->setMaterialIndex( coordindexindex, materialindex );
		coordindex++; coordindexindex++;
		prevrc = rc;
		defprevrc = true;
	    }
	}

	if ( firstindex>=0  && edgeline->isClosed() )
	{
	    polyline->setCoordIndex( coordindexindex,
		    		     polyline->getCoordIndex(firstindex) );
	    polyline->setMaterialIndex( coordindexindex,
				    polyline->getMaterialIndex(firstindex) );
	    coordindexindex++;
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
