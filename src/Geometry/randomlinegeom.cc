/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/

#include "randomlinegeom.h"

#include "interpol1d.h"
#include "ioman.h"
#include "iopar.h"
#include "randomlinetr.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "trigonometry.h"

mDefineInstanceCreatedNotifierAccess(Geometry::RandomLine);

namespace Geometry
{

RandomLine::RandomLine( const char* nm )
    : NamedMonitorable(nm)
    , nameChanged(this)
    , nodeChanged(this)
    , zrangeChanged(this)
    , lset_(0)
    , locked_(false)
    , survid_( Survey::GM().default3DSurvID() )
{
    ConstRefMan<Survey::Geometry3D> geom = Survey::GM().getGeometry3D(survid_);
    assign( zrange_, geom->zRange() );

    mDefineStaticLocalObject( Threads::Atomic<int>, oid, (0) );
    id_ = oid++;

    RLM().add( this );
    mTriggerInstanceCreatedNotifier();
}


RandomLine::~RandomLine()
{
    sendDelNotif();
    RLM().remove( this );
    id_ = -2;
}


void RandomLine::setDBKey( const DBKey& dbky )
{
    dbky_ = dbky;
    if ( !dbky_.isInvalid() )
	setName( IOM().nameOf(dbky_) );
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


void RandomLine::setName( const char* nm )
{
    NamedMonitorable::setName( nm );
    nameChanged.trigger();
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

void RandomLine::allNodePositions( TrcKeyPath& tks ) const
{
    tks.setSize( nodes_.size(), TrcKey::udf() );
    for ( int idx=0; idx<nodes_.size(); idx++ )
	tks[idx] = TrcKey( nodes_[idx] );
}

void RandomLine::limitTo( const TrcKeyZSampling& cs )
{
    if ( cs.hsamp_.survid_ != survid_ )
	{ pErrMsg( "Limiting go range in different survey"); }

    if ( nrNodes() != 2 )
	return;

    zrange_.limitTo( cs.zsamp_ );
    const TrcKeySampling& hs = cs.hsamp_;
    const bool startin = hs.includes( nodes_[0] );
    const bool stopin = hs.includes( nodes_[1] );
    if ( startin && stopin )
	return;

    ConstRefMan<Survey::Geometry3D> geom = Survey::GM().getGeometry3D(survid_);

    Coord svert[4];
    svert[0] = geom->transform( hs.start_ );
    svert[1] = geom->transform( BinID(hs.start_.inl(),hs.stop_.crl()) );
    svert[2] = geom->transform( hs.stop_ );
    svert[3] = geom->transform( BinID(hs.stop_.inl(),hs.start_.crl()) );

    Line2 line( geom->transform(nodes_[0]), geom->transform(nodes_[1]) );
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
	nodes_[startin ? 1 : 0] = geom->transform( points[0] );
    else if ( geom->transform(nodes_[0]).distTo(points[0])
	    < geom->transform(nodes_[0]).distTo(points[1]) )
    {
	nodes_[0] = geom->transform( points[0] );
	nodes_[1] = geom->transform( points[1] );
    }
    else
    {
	nodes_[0] = geom->transform( points[1] );
	nodes_[1] = geom->transform( points[0] );
    }
}


Coord RandomLine::getNormal( const TrcKeyPath& knots, const TrcKey& trcpos )
{
    const Coord poscrd = SI().transform( trcpos.binID() );
    double minsqdist = mUdf(double);
    Coord linestart, linestop;
    for ( int idx=0; idx<knots.size()-1; idx++ )
    {
	const BinID firstbid = knots[idx].binID();
	const BinID secondbid = knots[idx+1].binID();
	Interval<int> inlrg( firstbid.inl(), secondbid.inl() );
	Interval<int> crlrg( firstbid.crl(), secondbid.crl() );
	if ( !inlrg.includes(trcpos.inl(),true) ||
	     !crlrg.includes(trcpos.crl(),true) )
	    continue;

	const Coord firstcrd = SI().transform( firstbid );
	const Coord secondcrd = SI().transform( secondbid );
	Line2 linesegment( firstcrd, secondcrd );
	const Coord closestcrd = linesegment.closestPoint( poscrd );
	const double sqdist = closestcrd.sqDistTo( poscrd );
	if ( minsqdist > sqdist )
	{
	    linestart = firstcrd;
	    linestop = secondcrd;
	    minsqdist = sqdist;
	}
    }

    const Coord dir = linestart - linestop;
    return Coord( dir.y, -dir.x );
}


int RandomLine::getNearestPathPosIdx( const TrcKeyPath& knots,
				      const TrcKeyPath& path,
				      const TrcKey& trcpos )
{
    int posidx = path.indexOf( trcpos );
    if ( posidx>=0 )
	return posidx;

    const Coord poscrd = SI().transform( trcpos.binID() );
    double minsqdist = mUdf(double);
    BinID linestart, linestop;
    int linestartidx, linestopidx;
    linestartidx = linestopidx = -1;
    TrcKey intsecpos;
    Line2 closestlineseg;
    for ( int idx=0; idx<knots.size()-1; idx++ )
    {
	const BinID firstbid = knots[idx].binID();
	const BinID secondbid = knots[idx+1].binID();
	Interval<int> inlrg( firstbid.inl(), secondbid.inl() );
	Interval<int> crlrg( firstbid.crl(), secondbid.crl() );
	if ( !inlrg.includes(trcpos.inl(),true) ||
	     !crlrg.includes(trcpos.crl(),true) )
	    continue;

	const Coord firstcrd = SI().transform( firstbid );
	const Coord secondcrd = SI().transform( secondbid );
	Line2 linesegment( firstcrd, secondcrd );
	const Coord closestcrd = linesegment.closestPoint( poscrd );
	const double sqdist = closestcrd.sqDistTo( poscrd );
	if ( minsqdist > sqdist )
	{
	    linestart = firstbid;
	    linestop = secondbid;
	    linestartidx = path.indexOf( firstbid );
	    linestopidx = path.indexOf( secondbid );
	    minsqdist = sqdist;
	    intsecpos = TrcKey( SI().transform(closestcrd) );
	    closestlineseg = linesegment;
	}
    }

    if ( linestopidx<0 )
	return -1;

    BinID intsecbid = intsecpos.binID();
    posidx = path.indexOf( intsecbid );
    if ( posidx>=0 )
	return posidx;

    const float disttointsec = Math::Sqrt((float)linestart.sqDistTo(intsecbid));
    const float disttostop = Math::Sqrt( (float)linestart.sqDistTo(linestop) );
    const int nearestpathidx =
	linestartidx + mNINT32( (linestopidx-linestartidx) *
				(disttointsec/disttostop) );
    StepInterval<int> linesegrg( nearestpathidx-10, nearestpathidx+10, 1 );
    Interval<int> limitrg( 0, path.size() );
    linesegrg.limitTo( limitrg );
    minsqdist = mUdf(double);
    int nearestposidx = -1;
    for ( int ipath=linesegrg.start; ipath<=linesegrg.stop; ipath++ )
    {
	const BinID rdlpos = path[ipath].binID();
	const double sqdist = mCast(double,rdlpos.sqDistTo(intsecbid)) ;
	if ( minsqdist > sqdist )
	{
	    minsqdist = sqdist;
	    nearestposidx = ipath;
	}
    }

    return nearestposidx;
}


void RandomLine::getPathBids( const TypeSet<BinID>& knots,
			     Pos::SurvID survid,
			    TypeSet<BinID>& bids,
			    bool allowduplicate,
			    TypeSet<int>* segments )
{
    ConstRefMan<Survey::Geometry3D> geom = Survey::GM().getGeometry3D( survid );
    for ( int idx=1; idx<knots.size(); idx++ )
    {
	BinID start = knots[idx-1];
	BinID stop = knots[idx];
	if ( start == stop ) continue;
	const int nrinl = int(abs(stop.inl()-start.inl()) / geom->inlStep() +1);
	const int nrcrl = int(abs(stop.crl()-start.crl()) / geom->crlStep() +1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	const char fastdim = inlwise ? 0 : 1;
	const char slowdim = inlwise ? 1 : 0;

	bool reverse = stop[fastdim] - start[fastdim] < 0;
	int step = geom->sampling().hsamp_.step_[fastdim];
	if ( reverse ) step *= -1;

	for ( int idi=0; idi<nrlines; idi++ )
	{
	    BinID bid;
	    int bidx = start[fastdim] + idi*step;
	    float val = Interpolate::linear1D( (float)start[fastdim],
					      (float)start[slowdim],
					      (float)stop[fastdim],
					      (float)stop[slowdim],
					      (float)bidx );
	    int bidy = (int)(val + .5);
	    BinID nextbid = inlwise ? BinID(bidx,bidy) : BinID(bidy,bidx);
	    geom->snap( nextbid );
	    bool didadd;
	    if ( allowduplicate )
	    {
		didadd = true;
		bids += nextbid;
	    }
	    else
	    {
		didadd = bids.addIfNew( nextbid );
	    }

	    if ( didadd && segments ) (*segments) += (idx-1);\
	}
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

    ConstRefMan<Survey::Geometry3D> geom =
	    Survey::GM().getGeometry3D( baserandln.getSurvID() );

    const Coord startpt = geom->transform( baserandln.nodePosition(0) );
    const Coord stoppt = geom->transform( baserandln.nodePosition(1) );
    Line2 rline( startpt, stoppt );

    if ( parallel )
	createParallelLines( rline, geom->getSurvID(), dist );
    else
    {
	const Coord centrpt = ( startpt + stoppt ) / 2;
	Line2 rlineperp;
	rline.getPerpendicularLine( rlineperp, centrpt );
	createParallelLines( rlineperp, geom->getSurvID(), dist );
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


void RandomLineSet::createParallelLines( const Line2& baseline,
					Pos::SurvID survid, double dist )
{
    ConstRefMan<Survey::Geometry3D> geom =
	Survey::GM().getGeometry3D( survid );
    const TrcKeySampling hs( geom->sampling().hsamp_ );
    Coord svert[4];
    svert[0] = geom->transform( hs.start_ );
    svert[1] = geom->transform( BinID(hs.start_.inl(),hs.stop_.crl()) );
    svert[2] = geom->transform( hs.stop_ );
    svert[3] = geom->transform( BinID(hs.stop_.inl(),hs.start_.crl()) );

    Line2 sbound[4];			// Survey boundaries
    for ( int idx=0; idx<4; idx++ )
	sbound[idx] = Line2( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );

    bool posfinished = false, negfinished = false;
    for ( int idx=0; idx<100; idx++ )
    {
	if ( posfinished && negfinished )
	    break;

	Line2 posline;
	Line2 negline;
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
	    rln->addNode( geom->transform(endsposline[0]) );
	    rln->addNode( geom->transform(endsposline[1]) );
	    addLine( *rln );
	}

	if ( !idx ) continue;
	if ( endsnegline.size() < 2 )
	    negfinished = true;
	else
	{
	    RandomLine* rln = new RandomLine;
	    rln->addNode( geom->transform(endsnegline[0]) );
	    rln->addNode( geom->transform(endsnegline[1]) );
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


void RandomLineSet::getGeometry( const DBKey& rdlsid, TypeSet<BinID>& knots,
				 StepInterval<float>* zrg )
{
    Geometry::RandomLineSet rls; uiString errmsg;
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


int RandomLineManager::indexOf( const DBKey& dbky ) const
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->getDBKey() == dbky )
	    return idx;
    }

    return -1;
}


RandomLine* RandomLineManager::get( const DBKey& dbky )
{
    if ( dbky.isInvalid() )
	return 0;
    const int rlidx = indexOf( dbky );
    RandomLine* rl = lines_.validIdx(rlidx) ? lines_[rlidx] : 0;
    if ( rl )
	return rl;

    PtrMan<IOObj> ioobj = IOM().get( dbky );
    if ( !ioobj ) return 0;

    RandomLineSet rdlset;
    BufferString msg;
    const bool res = RandomLineSetTranslator::retrieve( rdlset, ioobj, msg );
    if ( !res || rdlset.isEmpty() ) return 0;

    rl = rdlset.getRandomLine( 0 );
    rl->setDBKey( dbky );
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


bool RandomLineManager::isLoaded( const DBKey& dbky ) const
{
    const int rlidx = indexOf( dbky );
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
