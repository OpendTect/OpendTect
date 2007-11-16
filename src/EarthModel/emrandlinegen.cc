/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2007
 RCS:           $Id: emrandlinegen.cc,v 1.2 2007-11-16 13:39:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "emrandlinegen.h"
#include "emhorizon3d.h"
#include "randomlinegeom.h"
#include "isocontourtracer.h"
#include "survinfo.h"


EM::RandomLineSetGenerator::Setup::Setup( bool rel )
    : contzrg_(SI().zRange(true))
    , isrel_(rel)
    , sectionnr_(-1)
    , selpoly_(0)
{
    if ( !isrel_ )
	assign( linezrg_, contzrg_ );
    else
    {
	linezrg_.stop = 30 * contzrg_.step;
	linezrg_.start = -linezrg_.stop;
    }
}


EM::RandomLineSetGenerator::RandomLineSetGenerator( const EM::Horizon3D& hor,
			const EM::RandomLineSetGenerator::Setup& su )
	: hor_(hor)
    	, geom_(hor.geometry())
	, setup_(su)
{
}


void EM::RandomLineSetGenerator::createLines(
				Geometry::RandomLineSet& rls ) const
{
    BinID bid, prevbid;
    for ( int isect=0; isect<geom_.nrSections(); isect++ )
    {
	if ( setup_.sectionnr_ >= 0 && setup_.sectionnr_ != isect )
	    continue;

	const EM::SectionID sid = hor_.sectionID( isect );
	const Array2D<float>* arr = geom_.sectionGeometry(sid)->getArray();
	if ( !arr )
	    continue;

	const StepInterval<int> inlrg = geom_.rowRange(sid);
	const StepInterval<int> crlrg = geom_.colRange(sid);
	IsoContourTracer ict( *arr );
	ict.setSampling( inlrg, crlrg );
	ict.selectPolyROI( setup_.selpoly_ );
	const float zeps = 0.0001 * setup_.contzrg_.step;

	for ( float z = setup_.contzrg_.start;
	      z - zeps < setup_.contzrg_.stop;
	      z += setup_.contzrg_.step )
	{
	    ObjectSet<ODPolygon<float> > polys;
	    ict.getContours( polys, z );
	    if ( polys.isEmpty() )
		continue;
	    
	    prevbid = BinID( mUdf(int), mUdf(int) );
	    for ( int ipoly=0; ipoly<polys.size(); ipoly++ )
	    {
		const ODPolygon<float>& poly = *polys[ipoly];
		Geometry::RandomLine* rl = new Geometry::RandomLine;
		BinID addbid( prevbid );

		for ( int ipt=0; ipt<poly.size(); ipt++ )
		{
		    const Geom::Point2D<float> vtx = poly.getVertex( ipt );
		    bid.inl = inlrg.snap( vtx.x );
		    bid.crl = crlrg.snap( vtx.y );
		    if ( bid != prevbid )
		    {
			rl->addNode( bid );
			prevbid = bid;
		    }
		    if ( ipt == 0 && poly.isClosed() )
			addbid = bid;
		}
		if ( !mIsUdf(addbid.inl) && addbid != prevbid
		  && rl->nrNodes() > 2 )
		    rl->addNode( addbid );

		if ( rl->nrNodes() < 2 )
		    delete rl;
		else
		{
		    rls.addLine( rl );
		    Interval<float> zrg( setup_.linezrg_ );
		    if ( setup_.isrel_ ) zrg.shift( z );
		    rl->setZRange( zrg );
		}
	    }

	    deepErase( polys );
	}
    }
}
