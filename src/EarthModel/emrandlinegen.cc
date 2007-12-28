/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2007
 RCS:           $Id: emrandlinegen.cc,v 1.6 2007-12-28 10:29:09 cvsbert Exp $
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

    if ( side_ < 1 ) crLine( rl, true, outrls );
    if ( side_ > -1 ) crLine( rl, false, outrls );
}


void EM::RandomLineByShiftGenerator::crLine( const Geometry::RandomLine& rl,
					 bool isleft,
					 Geometry::RandomLineSet& outrls ) const
{
    BufferString newnm( rl.name() );
    newnm += "/"; newnm += dist_; newnm += isleft ? "L" : "R";
    Geometry::RandomLine* outrl = new Geometry::RandomLine( newnm );
    BinID prevnode( mUdf(int), 0 );
    for ( int idx=0; idx<rl.nrNodes(); idx++ )
    {
	const bool atstart = idx == 0;
	const bool atend = idx == rl.nrNodes() - 1;
	const Coord c1( SI().transform(rl.nodePosition(idx)) );
	const Coord c0( atstart ? c1 : SI().transform(rl.nodePosition(idx-1)) );
	const Coord c2( atend ? c1 : SI().transform(rl.nodePosition(idx+1)) );
	Coord cs0, cs10, cs12, cs2;
	if ( !atstart && !getShifted(c0,c1,cs0,cs10,isleft)
	  || !atend && !getShifted(c1,c2,cs12,cs2,isleft) )
	    continue;

	Coord nodec;
	if ( atstart )
	    nodec = cs12;
	else if ( atend )
	    nodec = cs10;
	else
	    nodec = getIntersection( cs0, cs10, cs12, cs2 );
	BinID newnode = SI().transform( nodec );
	if ( newnode != prevnode )
	    outrl->addNode( newnode );
	prevnode = newnode;
    }
    if ( outrl->nrNodes() < 2 )
	delete outrl;
    else
	outrls.addLine( outrl );
}


bool EM::RandomLineByShiftGenerator::getShifted( Coord c1, Coord c2,
				    Coord& cs1, Coord& cs2, bool isleft ) const
{
    cs1 = c1; cs2 = c2;
    c2 -= c1;
    double r0 = sqrt( c2.x*c2.x + c2.y * c2.y );
    if ( mIsZero(r0,1e-3) ) return false;

    double scl = dist_ / r0;
    Coord vec( scl * c2.y, scl * c2.x );
    if ( isleft ) vec.x = -vec.x;
    else	  vec.y = -vec.y;

    cs1 += vec; cs2 += vec;
    return true;
}


Coord EM::RandomLineByShiftGenerator::getIntersection( Coord c00, Coord c01,
				    Coord c10, Coord c11 ) const
{
    //TODO calculate proper intersection. Beware of points on one line ...
    return (c01 + c10) / 2;
}
