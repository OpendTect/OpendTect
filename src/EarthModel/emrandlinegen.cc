/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: emrandlinegen.cc,v 1.21 2012-07-10 08:05:30 cvskris Exp $";

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
    , nrlargestonly_(-1)
    , minnrvertices_(2)
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
    const int zfac = SI().zDomain().userFactor(); // for line name

    for ( int isect=0; isect<geom_.nrSections(); isect++ )
    {
	if ( setup_.sectionnr_ >= 0 && setup_.sectionnr_ != isect )
	    continue;

	const EM::SectionID sid = hor_.sectionID( isect );
	const Array2D<float>* arr = geom_.sectionGeometry(sid)->getArray();
	if ( !arr )
	    continue;

	const StepInterval<int> inlrg = geom_.rowRange(sid);
	const StepInterval<int> crlrg = geom_.colRange(sid,-1);
	IsoContourTracer ict( *arr );
	ict.setSampling( inlrg, crlrg );
	ict.selectPolyROI( setup_.selpoly_ );
	ict.setMinNrVertices( setup_.minnrvertices_ );
	ict.setNrLargestOnly( setup_.nrlargestonly_ );
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
		    nm += mNINT32( usrz );
		    if ( usrpolynr > 1 )
			{ nm += "-"; nm += usrpolynr; }
		    usrpolynr++;
		    rl->setName( nm.buf() );

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
			    bool isleft, Geometry::RandomLineSet& outrls ) const
{
    BufferString newnm( rl.name() );
    newnm += "/"; newnm += dist_; newnm += isleft ? "L" : "R";

    TypeSet<Coord> basecoords;
    for ( int idx=0; idx<rl.nrNodes(); idx++ )
	basecoords += SI().transform( rl.nodePosition(idx) );

    const bool isclosed = rl.nrNodes()>2 && 
			  rl.nodePosition(0)==rl.nodePosition(rl.nrNodes()-1);
    if ( isclosed ) 
	basecoords += basecoords[1];

    while ( true )
    {
	Geometry::RandomLine* outrl = new Geometry::RandomLine( newnm.buf() );
	outrl->setZRange( rl.zRange() );
	TypeSet<int> dirflips;
	TypeSet<Coord> fusioncrds;

	int previdx = 0; int preprevidx = 0;
	BinID prevnode( mUdf(int), 0 );
	Coord prevnodec( Coord::udf() ); 

	const int nrbasecoords = basecoords.size();
    
	for ( int idx=0; idx<nrbasecoords; idx++ )
	{
	    const bool atstart = idx == 0;
	    const bool atend = idx == nrbasecoords-1;
	    const Coord c0 = basecoords[previdx];
	    const Coord c1 = basecoords[idx];
	    const Coord c2 = atend ? c1 : basecoords[idx+1];

	    Coord cs0, cs10, cs12, cs2;
	    if ( (!atstart && !getShifted(c0,c1,cs0,cs10,isleft))
	      || (!atend && !getShifted(c1,c2,cs12,cs2,isleft)) )
		continue;

	    Coord nodec;
	    if ( atstart )
		nodec = cs12;
	    else if ( atend )
		nodec = cs10;
	    else if ( !getIntersection(cs0, cs10, cs12, cs2, nodec) )
		nodec = (cs10 + cs12) / 2;

	    const Coord dirvec = c1 - c0;
	    if ( !atstart && dirvec.dot(nodec-prevnodec)<0 )
	    {
		dirflips += idx;
		
		if ( previdx < 1 )
		    fusioncrds += c0;
		else if ( atend )
		    fusioncrds += c1;
		else
		{
		    // weighted fusion based on adjacent triangle areas
		    const Coord prevec = c0 - basecoords[preprevidx];
		    const Coord nxtvec = c1 - c2;
		    const float w0 = fabs(prevec.x*dirvec.y-prevec.y*dirvec.x);
		    const float w1 = fabs(nxtvec.x*dirvec.y-nxtvec.y*dirvec.x);
		    fusioncrds += w0+w1>0 ? (c0*w0+c1*w1)/(w0+w1) : (c0+c1)/2;
		}
	    }

	    BinID newnode = SI().transform( nodec );

	    if ( atend && isclosed )
		outrl->setNodePosition( 0, prevnode );
	    else if ( newnode != prevnode )
		outrl->addNode( newnode );
	    
	    preprevidx = previdx; previdx = idx;
	    prevnode = newnode; prevnodec = nodec;
	}

	if ( dirflips.isEmpty() ) 
	{
	    if ( outrl->nrNodes() <= (isclosed ? 2 : 1) )
		delete outrl;
	    else
		outrls.addLine( outrl );
	    
	    break;
	}

	for ( int idx=0; idx<nrbasecoords; idx++ )
	{
	    if ( dirflips[0] != idx )
	    {
		if ( dirflips[0] != idx+1 )
		    basecoords += basecoords[idx];
		continue;
	    }
	    basecoords += fusioncrds[0];
	    dirflips.remove( 0 );
	    fusioncrds.remove( 0 );
	}

	basecoords.remove( 0, nrbasecoords-1 );
	delete outrl;
    }
}


bool EM::RandomLineByShiftGenerator::getShifted( Coord c1, Coord c2,
				    Coord& cs1, Coord& cs2, bool isleft ) const
{
    cs1 = c1; cs2 = c2;
    c2 -= c1;
    double r0 = Math::Sqrt( c2.x*c2.x + c2.y * c2.y );
    if ( mIsZero(r0,1e-3) ) return false;

    double scl = dist_ / r0;
    Coord vec( scl * c2.y, scl * c2.x );
    if ( isleft ) vec.x = -vec.x;
    else	  vec.y = -vec.y;

    cs1 += vec; cs2 += vec;
    return true;
}


bool EM::RandomLineByShiftGenerator::getIntersection( Coord c00, Coord c01, 
						      Coord c10, Coord c11,
						      Coord& cinter ) const
{
    const Coord dif0 = c00 - c01; 
    const Coord dif1 = c10 - c11;
    const double det = dif0.x*dif1.y - dif0.y*dif1.x;
    if ( mIsZero(det,1e-6) )
	return false;
    
    const double det0 = c00.x*c01.y - c00.y*c01.x;
    const double det1 = c10.x*c11.y - c10.y*c11.x;
    cinter = (dif1*det0 - dif0*det1) / det;

    return true;
}
