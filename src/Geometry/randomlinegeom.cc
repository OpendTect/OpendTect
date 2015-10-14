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
#include "ioman.h"
#include "iopar.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "trigonometry.h"

namespace Geometry
{

RandomLine::RandomLine( const char* nm )
    : NamedObject(nm)
    , nodeChanged(this)
    , zrangeChanged(this)
    , lset_(0)
    , mid_(MultiID::udf())
    , locked_(false)
{
    assign( zrange_, SI().zRange(true) );

    mDefineStaticLocalObject( Threads::Atomic<int>, oid, (0) );
    id_ = oid++;

    RLM().add( this );
}


RandomLine::~RandomLine()
{
    RLM().remove( this );
    id_ = -2;
}


void RandomLine::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    if ( !mid_.isUdf() )
	setName( IOM().nameOf(mid) );
}


int RandomLine::addNode( const BinID& bid )
{
    nodes_ += bid;
    const int nodeidx = nodes_.size()-1;
    ChangeData cd( ChangeData::Added, nodeidx );
    nodeChanged.trigger( cd );
    return nodeidx;
}


void RandomLine::insertNode( int idx, const BinID& bid )
{
    nodes_.insert( idx, bid );
    ChangeData cd( ChangeData::Inserted, idx );
    nodeChanged.trigger( cd );
}


void RandomLine::setNodePosition( int idx, const BinID& bid, bool moving )
{
    nodes_[idx] = bid;
    ChangeData cd( moving ? ChangeData::Moving : ChangeData::Moved, idx );
    nodeChanged.trigger( cd );
}


void RandomLine::setNodePositions( const TypeSet<BinID>& bids )
{
    while ( nrNodes() > bids.size() )
	removeNode( 0 );

    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( idx<nrNodes() )
	    setNodePosition( idx, bids[idx], true );
	else
	    addNode( bids[idx] );
    }

    ChangeData cd( ChangeData::Moved, -1 );
    nodeChanged.trigger( cd );
}


void RandomLine::removeNode( int idx )
{
    nodes_.removeSingle( idx );
    ChangeData cd( ChangeData::Removed, idx );
    nodeChanged.trigger( cd );
}


void RandomLine::removeNode( const BinID& bid )
{ removeNode( nodes_.indexOf(bid) ); }

int RandomLine::nodeIndex( const BinID& bid ) const
{ return nodes_.indexOf( bid ); }

int RandomLine::nrNodes() const
{ return nodes_.size(); }

const BinID& RandomLine::nodePosition( int idx ) const
{ return nodes_[idx]; }

void RandomLine::allNodePositions( TypeSet<BinID>& bids ) const
{ bids = nodes_; }


void RandomLine::limitTo( const TrcKeyZSampling& cs )
{
    if ( nrNodes() != 2 ) return;

    zrange_.limitTo( cs.zsamp_ );
    const TrcKeySampling& hs = cs.hsamp_;
    const bool startin = hs.includes( nodes_[0] );
    const bool stopin = hs.includes( nodes_[1] );
    if ( startin && stopin )
	return;

    Coord svert[4];
    svert[0] = SI().transform( hs.start_ );
    svert[1] = SI().transform( BinID(hs.start_.inl(),hs.stop_.crl()) );
    svert[2] = SI().transform( hs.stop_ );
    svert[3] = SI().transform( BinID(hs.stop_.inl(),hs.start_.crl()) );

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
	if ( start == stop ) continue;
	const int nrinl = int(abs(stop.inl()-start.inl()) / SI().inlStep() + 1);
	const int nrcrl = int(abs(stop.crl()-start.crl()) / SI().crlStep() + 1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	if ( inlwise )
	    { mGetBinIDs(inl(),crl()); }
	else
	    { mGetBinIDs(crl(),inl()); }
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
    deepUnRef(lines_);
    delete &pars_;
}


void RandomLineSet::setEmpty()
{
    deepUnRef( lines_ );
    pars_.setEmpty();
}


RandomLine* RandomLineSet::getRandomLine( int idx )
{ return lines_.validIdx(idx) ? lines_[idx] : 0; }


const RandomLine* RandomLineSet::getRandomLine( int idx ) const
{ return lines_.validIdx(idx) ? lines_[idx] : 0; }


void RandomLineSet::removeLine( int idx )
{ lines_.removeSingle(idx)->unRef(); }


void RandomLineSet::addLine( RandomLine& rl )
{
    rl.lset_ = this;
    lines_ += &rl;
    rl.ref();
}


void RandomLineSet::insertLine( RandomLine& rl, int idx )
{
    rl.lset_ = this;
    lines_.insertAt( &rl, idx );
    rl.ref();
}


void RandomLineSet::createParallelLines( const Line2& baseline, double dist )
{
    const TrcKeySampling hs( SI().sampling(false).hsamp_ );
    Coord svert[4];
    svert[0] = SI().transform( hs.start_ );
    svert[1] = SI().transform( BinID(hs.start_.inl(),hs.stop_.crl()) );
    svert[2] = SI().transform( hs.stop_ );
    svert[3] = SI().transform( BinID(hs.stop_.inl(),hs.start_.crl()) );

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
	    addLine( *rln );
	}

	if ( !idx ) continue;
	if ( endsnegline.size() < 2 )
	    negfinished = true;
	else
	{
	    RandomLine* rln = new RandomLine;
	    rln->addNode( SI().transform(endsnegline[0]) );
	    rln->addNode( SI().transform(endsnegline[1]) );
	    insertLine( *rln, 0 );
	}
    }
}

void RandomLineSet::limitTo( const TrcKeyZSampling& cs )
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	lines_[idx]->limitTo( cs );
	if ( !lines_[idx]->nrNodes() )
	    removeLine( idx-- );
    }
}


void RandomLineSet::getGeometry( const MultiID& rdlsid, TypeSet<BinID>& knots,
				 StepInterval<float>* zrg )
{
    Geometry::RandomLineSet rls; BufferString errmsg;
    const PtrMan<IOObj> rdmline = IOM().get( rdlsid );
    RandomLineSetTranslator::retrieve( rls, rdmline, errmsg );
    if ( !errmsg.isEmpty() || rls.isEmpty() )
	return;

    if ( zrg )
	*zrg = Interval<float>(mUdf(float),-mUdf(float));

    TypeSet<BinID> rdmlsknots;
    for ( int lidx=0; lidx<rls.size(); lidx++ )
    {
	TypeSet<BinID> rdmlknots;
	rls.lines()[lidx]->allNodePositions( rdmlknots );
	knots.append( rdmlknots );
	if ( zrg )
	    zrg->include( rls.lines()[lidx]->zRange(), false );
    }
}


// RandomLineManager
RandomLineManager& RLM()
{
    mDefineStaticLocalObject( PtrMan<RandomLineManager>, mgr,
			      (new RandomLineManager) );
    return *mgr;
}


RandomLineManager::RandomLineManager()
    : added(this)
    , removed(this)
{
}


RandomLineManager::~RandomLineManager()
{
}


int RandomLineManager::indexOf( const MultiID& mid ) const
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->getMultiID() == mid )
	    return idx;
    }

    return -1;
}


RandomLine* RandomLineManager::get( const MultiID& mid )
{
    if ( mid.isUdf() ) return 0;
    const int rlidx = indexOf( mid );
    RandomLine* rl = lines_.validIdx(rlidx) ? lines_[rlidx] : 0;
    if ( rl )
	return rl;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return 0;

    RandomLineSet rdlset;
    BufferString msg;
    const bool res = RandomLineSetTranslator::retrieve( rdlset, ioobj, msg );
    if ( !res || rdlset.isEmpty() ) return 0;

    rl = rdlset.getRandomLine( 0 );
    rl->setMultiID( mid );
    add( rl );
    return rl;
}


RandomLine* RandomLineManager::get( int id )
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->ID() == id )
	    return lines_[idx];
    }

    return 0;
}


const RandomLine* RandomLineManager::get( int id ) const
{ return const_cast<RandomLineManager*>(this)->get( id ); }


bool RandomLineManager::isLoaded( const MultiID& mid ) const
{
    const int rlidx = indexOf( mid );
    return lines_.validIdx( rlidx );
}


bool RandomLineManager::isLoaded( int id ) const
{ return (bool)get( id ); }


int RandomLineManager::add( RandomLine* rl )
{
    if ( !rl ) return -1;

    const bool res = lines_.addIfNew( rl );
    if ( res )
    {
	rl->ref();
	added.trigger( rl->ID() );
    }

    return rl->ID();
}


void RandomLineManager::remove( RandomLine* rl )
{
    if ( !rl ) return;

    const int rlidx = lines_.indexOf( rl );
    if ( rlidx<0 ) return;

    removed.trigger( rl->ID() );
    lines_.removeSingle( rlidx )->unRef();
}

} //namespace Geometry


