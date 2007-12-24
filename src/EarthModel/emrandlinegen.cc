/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2007
 RCS:           $Id: emrandlinegen.cc,v 1.5 2007-12-24 16:51:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "emrandlinegen.h"
#include "emhorizon3d.h"
#include "randomlinegeom.h"
#include "isocontourtracer.h"
#include "survinfo.h"


EM::RandomLineSetByContourGenerator::Setup::Setup( bool rel )
    : contzrg_(SI().zRange(true))
    , isrel_(rel)
    , sectionnr_(-1)
    , selpoly_(0)
{
    if ( !isrel_ )
	assign( linezrg_, contzrg_ );
    else
    {
	linezrg_.stop = 50 * contzrg_.step;
	linezrg_.start = -linezrg_.stop;
    }
}


EM::RandomLineSetByContourGenerator::RandomLineSetByContourGenerator(
			const EM::Horizon3D& hor,
			const EM::RandomLineSetByContourGenerator::Setup& su )
	: hor_(hor)
    	, geom_(hor.geometry())
	, setup_(su)
{
}


void EM::RandomLineSetByContourGenerator::createLines(
				Geometry::RandomLineSet& rls ) const
{
    BinID bid, prevbid;
    const float zfac = SI().zFactor(); // for line name

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
	    
	    int usrpolynr = 1;
	    for ( int ipoly=0; ipoly<polys.size(); ipoly++ )
	    {
		const ODPolygon<float>& poly = *polys[ipoly];
		Geometry::RandomLine* rl = new Geometry::RandomLine;

		prevbid = BinID( mUdf(int), mUdf(int) );
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
		if ( !mIsUdf(addbid.inl) && addbid!=prevbid && rl->nrNodes()>2 )
		    rl->addNode( addbid );

		if ( rl->nrNodes() < 2 )
		    delete rl;
		else
		{
		    BufferString nm( "C" ); const float usrz = z * zfac;
		    nm += mNINT( usrz );
		    if ( usrpolynr > 1 )
			{ nm += "-"; nm += usrpolynr; }
		    usrpolynr++;
		    rl->setName( nm );

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


void EM::RandomLineByShiftGenerator::generate( Geometry::RandomLineSet& outrls,
						int lnr ) const
{
    if ( lnr >= rls_.size() ) return;
    const Geometry::RandomLine& rl = *rls_.lines()[lnr];
    while ( !outrls.isEmpty() ) outrls.removeLine( 0 );

    //TODO implement
    pErrMsg("TODO: implement");
}
