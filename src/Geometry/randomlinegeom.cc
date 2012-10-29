/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "randomlinegeom.h"

#include "interpol1d.h"
#include "cubesampling.h"
#include "iopar.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace Geometry
{

RandomLine::RandomLine( const char* nm )
    : NamedObject(nm)
    , nodeAdded(this)
    , nodeInserted(this)
    , nodeRemoved(this)
    , zrangeChanged(this)
    , lset_(0)
{
    assign( zrange_, SI().zRange(true) );
}


int RandomLine::addNode( const BinID& bid )
{
    nodes_ += bid;
    nodeAdded.trigger();
    return nodes_.size()-1;
}


void RandomLine::insertNode( int idx, const BinID& bid )
{ nodes_.insert( idx, bid ); nodeInserted.trigger(); }

void RandomLine::setNodePosition( int idx, const BinID& bid )
{ nodes_[idx] = bid; }

void RandomLine::removeNode( int idx )
{ nodes_.removeSingle( idx ); nodeRemoved.trigger(); }

void RandomLine::removeNode( const BinID& bid )
{ nodes_ -= bid; nodeRemoved.trigger(); }

int RandomLine::nodeIndex( const BinID& bid ) const
{ return nodes_.indexOf( bid ); }

int RandomLine::nrNodes() const
{ return nodes_.size(); }

const BinID& RandomLine::nodePosition( int idx ) const
{ return nodes_[idx]; }

void RandomLine::allNodePositions( TypeSet<BinID>& bids ) const
{ bids = nodes_; }


void RandomLine::limitTo( const CubeSampling& cs )
{
    if ( nrNodes() != 2 ) return;

    zrange_.limitTo( cs.zrg );
    const HorSampling& hs = cs.hrg;
    const bool startin = hs.includes( nodes_[0] );
    const bool stopin = hs.includes( nodes_[1] );
    if ( startin && stopin )
	return;

    Coord svert[4];
    svert[0] = SI().transform( hs.start );
    svert[1] = SI().transform( BinID(hs.start.inl,hs.stop.crl) );
    svert[2] = SI().transform( hs.stop );
    svert[3] = SI().transform( BinID(hs.stop.inl,hs.start.crl) );

    Line2 line( SI().transform(nodes_[0]), SI().transform(nodes_[1]) );
    TypeSet<Coord> points;
    for ( int idx=0; idx<4; idx++ )
    {
	Line2 bound( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );
	Coord pt = line.intersection( bound );
	if ( !mIsUdf(pt.x) && !mIsUdf(pt.y) )
	    points += pt;
    }

    if ( !points.size() )
	nodes_.erase();
    else if ( points.size() == 1 )
	nodes_[startin ? 1 : 0] = SI().transform( points[0] );
    else if ( SI().transform(nodes_[0]).distTo(points[0])
	    < SI().transform(nodes_[0]).distTo(points[1]) )
    {
	nodes_[0] = SI().transform( points[0] );
	nodes_[1] = SI().transform( points[1] );
    }
    else
    {
	nodes_[0] = SI().transform( points[1] );
	nodes_[1] = SI().transform( points[0] );
    }
}


#define mGetBinIDs( x, y ) \
    bool reverse = stop.x - start.x < 0; \
    int step = inlwise ? SI().inlStep() : SI().crlStep(); \
    if ( reverse ) step *= -1; \
    for ( int idi=0; idi<nrlines; idi++ ) \
    { \
	BinID bid; \
	int bidx = start.x + idi*step; \
	float val = Interpolate::linear1D( (float)start.x, (float)start.y, \
					   (float)stop.x, (float)stop.y, \
					   (float)bidx ); \
	int bidy = (int)(val + .5); \
	BinID nextbid = inlwise ? BinID(bidx,bidy) : BinID(bidy,bidx); \
	SI().snap( nextbid ); \
	if ( allowduplicate ) \
	    bids += nextbid ; \
	else \
	    bids.addIfNew( nextbid ); \
	if ( segments ) (*segments) += (idx-1);\
    }

void RandomLine::getPathBids( const TypeSet<BinID>& knots, 
			    TypeSet<BinID>& bids, 
			    bool allowduplicate,
			    TypeSet<int>* segments ) 
{
    for ( int idx=1; idx<knots.size(); idx++ )
    {
	BinID start = knots[idx-1];
	BinID stop = knots[idx];
	const int nrinl = int(abs(stop.inl-start.inl) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl-start.crl) / SI().crlStep() + 1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	if ( inlwise )
	    { mGetBinIDs(inl,crl); }
	else
	    { mGetBinIDs(crl,inl); }
    }
}





RandomLineSet::RandomLineSet()
    : pars_(*new IOPar)
{
}


RandomLineSet::RandomLineSet( const RandomLine& baserandln, double dist,
			      bool parallel )
    : pars_(*new IOPar)
{
    if ( baserandln.nrNodes() != 2 )
	return;

    const Coord startpt = SI().transform( baserandln.nodePosition(0) );
    const Coord stoppt = SI().transform( baserandln.nodePosition(1) );
    Line2 rline( startpt, stoppt );
    rline.start_ = Coord::udf();
    rline.stop_ = Coord::udf();			// removing limits.
    if ( parallel )
	createParallelLines( rline, dist );
    else
    {
	const Coord centrpt = ( startpt + stoppt ) / 2;
	Line2 rlineperp( 0, 0 );
	rline.getPerpendicularLine( rlineperp, centrpt );
	createParallelLines( rlineperp, dist );
    }
}


RandomLineSet::~RandomLineSet()
{
    deepErase(lines_);
    delete &pars_;
}


void RandomLineSet::createParallelLines( const Line2& baseline, double dist )
{
    const HorSampling hs( SI().sampling(false).hrg );
    Coord svert[4];
    svert[0] = SI().transform( hs.start );
    svert[1] = SI().transform( BinID(hs.start.inl,hs.stop.crl) );
    svert[2] = SI().transform( hs.stop );
    svert[3] = SI().transform( BinID(hs.stop.inl,hs.start.crl) );

    Line2 sbound[4];			// Survey boundaries
    for ( int idx=0; idx<4; idx++ )
	sbound[idx] = Line2( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );

    bool posfinished = false, negfinished = false;
    for ( int idx=0; idx<100; idx++ )
    {
	if ( posfinished && negfinished )
	    break;

	Line2 posline( 0, 0 );
	Line2 negline( 0, 0 );
	if ( !idx )
	    posline = baseline;
	else
	{
	    if ( !posfinished )
		baseline.getParallelLine( posline, dist*idx );
	    if ( !negfinished )
		baseline.getParallelLine( negline, -dist*idx );
	}

	TypeSet<Coord> endsposline;
	TypeSet<Coord> endsnegline;
	for ( int bdx=0; bdx<4; bdx++ )
	{
	    if ( !posfinished )
	    {
		const Coord pos = posline.intersection( sbound[bdx] );
		if ( !mIsUdf(pos.x) && !mIsUdf(pos.y) )
		    endsposline += pos;
	    }

	    if ( idx && !negfinished )
	    {
		const Coord pos = negline.intersection( sbound[bdx] );
		if ( !mIsUdf(pos.x) && !mIsUdf(pos.y) )
		    endsnegline += pos;
	    }
	}

	if ( endsposline.size() < 2 )
	    posfinished = true;
	else
	{
	    RandomLine* rln = new RandomLine;
	    rln->addNode( SI().transform(endsposline[0]) );
	    rln->addNode( SI().transform(endsposline[1]) );
	    addLine( rln );
	}

	if ( !idx ) continue;
	if ( endsnegline.size() < 2 )
	    negfinished = true;
	else
	{
	    RandomLine* rln = new RandomLine;
	    rln->addNode( SI().transform(endsnegline[0]) );
	    rln->addNode( SI().transform(endsnegline[1]) );
	    rln->lset_ = this;
	    lines_.insertAt( rln, 0 );
	}
    }
}

void RandomLineSet::limitTo( const CubeSampling& cs )
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	lines_[idx]->limitTo( cs );
	if ( !lines_[idx]->nrNodes() )
	    removeLine( idx-- );
    }
}

} //namespace


