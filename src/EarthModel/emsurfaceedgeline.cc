/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfaceedgeline.cc,v 1.45 2012/03/02 20:47:24 cvsnanne Exp $";
   

#include "emsurfaceedgeline.h"

#include "rcollinebuilder.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "executor.h"
#include "iopar.h"
#include "mathfunc.h"
#include "sorting.h"
#include "survinfo.h"
#include "toplist.h"
#include "errh.h"

#include <math.h>

namespace EM {


const char* EdgeLineSegment::key() { return "Nodes"; }

const char* EdgeLineSegment::classnamestr() { return "Segment Type"; }


const char* EdgeLine::nrsegmentsstr()	{ return "Nr segments"; }
const char* EdgeLine::segmentprefixstr(){ return "Segment "; }

const char* EdgeLineSet::nrlinesstr()	{ return "Nr lines"; }
const char* EdgeLineSet::lineprefixstr(){ return "Line "; }

const char* EdgeLineManager::sectionkey(){ return "Sectionlines "; }


mEdgeLineSegmentFactoryEntry(EdgeLineSegment);


EdgeLineSegment::EdgeLineSegment( Horizon3D& surf, const SectionID& sect )
    : horizon_( surf )
    , section( sect )
    , notifier( 0 )
    {}


EdgeLineSegment::EdgeLineSegment( const EdgeLineSegment& templ)
    : horizon_( templ.horizon_ )
    , section( templ.section )
    , nodes_( templ.nodes_ )
    , notifier( 0 )
{}


EdgeLineSegment::~EdgeLineSegment()
{
    if ( notifier )
	horizon_.change.remove(mCB(this,EdgeLineSegment,posChangeCB));

    delete notifier;
}


bool EdgeLineSegment::shouldHorizonTrack(int,const RowCol& trackdir) const
{ return true; }


bool EdgeLineSegment::haveIdenticalSettings( const EdgeLineSegment& seg ) const
{
    return !strcmp(typeid(*this).name(),typeid(seg).name()) &&
	   internalIdenticalSettings(seg);
}


bool EdgeLineSegment::internalIdenticalSettings(
					const EdgeLineSegment& seg) const
{
    return &horizon_==&seg.horizon_ && section==seg.section;
}


int EdgeLineSegment::size() const
{ return nodes_.size(); }


int EdgeLineSegment::indexOf( const RowCol& rc, bool forward ) const
{ return nodes_.indexOf(rc, forward ); }


void EdgeLineSegment::remove( int p1 )
{
    nodes_.remove(p1);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::remove( int p1, int p2 )
{
    nodes_.remove(p1,p2);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::removeAll()
{
    nodes_.erase();
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::insert( int p1, const RowCol& rc )
{
    nodes_.insert(p1,rc);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::insert( int p1, const TypeSet<RowCol>& rcs )
{
    if ( rcs.isEmpty() ) return;
    if ( p1>=size() )
	nodes_.append( rcs );
    else
    {
	for ( int idx=0; idx<rcs.size(); idx++ )
	    nodes_.insert( p1+idx, rcs[idx] );
    }
	
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::set( int p1, const RowCol& rc )
{
    if ( p1>=0 && p1<size() )
    {
	if ( rc!=nodes_[p1] )
	{
	    nodes_[p1] = rc;
	    if ( notifier ) notifier->trigger();
	}
    }
    else if ( p1==size() )
	(*this) += rc;
}


void EdgeLineSegment::copyNodesFrom( const TypeSet<RowCol>& templ, bool dorev )
{
    if ( !dorev )
	nodes_ = templ;
    else
    {
	nodes_.erase();
	for ( int idx=templ.size()-1; idx>=0; idx-- )
	    nodes_ += templ[idx];
    }

    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::copyNodesFrom( const EdgeLineSegment* templ, bool dorev)
{
    if ( templ ) copyNodesFrom( templ->getNodes(), dorev );
}


const RowCol& EdgeLineSegment::operator[]( int idx ) const
{ return nodes_[idx]; }


const EdgeLineSegment&
EdgeLineSegment::operator+=( const RowCol& rc )
{
    nodes_ += rc;
    if ( notifier ) notifier->trigger();
    return *this;
}


bool EdgeLineSegment::isClosed() const
{ return size()>3 && isContinuedBy(this); }


bool EdgeLineSegment::isContinuedBy( const EdgeLineSegment* seg ) const
{
    if ( !seg ) return false;
    const int sz = size();
    return sz && seg->size() &&
	   nodes_[sz-1].isNeighborTo((*seg)[0], horizon_.geometry().step());
}


bool EdgeLineSegment::isDefined( const RowCol& rc ) const
{ return horizon_.isDefined( section, rc.toInt64() ); }


bool EdgeLineSegment::isAtEdge( const RowCol& rc ) const
{ 
    const PosID pid( horizon_.id(), section, rc.toInt64() );
    return horizon_.geometry().isAtEdge(pid);
}


bool EdgeLineSegment::isByPassed( int idx, const EdgeLineSegment* prev,
				  const EdgeLineSegment* next ) const
{
    if ( (!idx && (!prev || !prev->isContinuedBy(this) )) ||
	   (idx==size()-1 && (!next || !isContinuedBy(next))) )
	return false;

    const RowCol& prevrc = idx ? nodes_[idx-1] : (*prev)[prev->size()-1];
    const RowCol& nextrc = idx!=size()-1 ? nodes_[idx+1] : (*next)[0];
    if ( !prevrc.isNeighborTo(nextrc,horizon_.geometry().step(), true ) )
	return false;

    const RowCol& rc = nodes_[idx];
    const RowCol prevdir = (prevrc-rc).getDirection();
    const RowCol nextdir = (nextrc-rc).getDirection();

    return nextdir.clockwiseAngleTo(prevdir)>M_PI;
}


bool EdgeLineSegment::isConnToNext(int idx) const
{
    if ( idx==size()-1 )
	return true;

    return nodes_[idx].isNeighborTo( nodes_[idx+1], horizon_.geometry().step(),
	    			    true );
}


bool EdgeLineSegment::isConnToPrev(int idx) const
{
    if ( !idx ) return true;

    return nodes_[idx].isNeighborTo(nodes_[idx-1],horizon_.geometry().step(),true );
}


bool EdgeLineSegment::areAllNodesOutsideBad( int idx,
					     const EdgeLineSegment* prev,
					     const EdgeLineSegment* next) const
{
    if ( (!idx&&!prev) || (idx==size()-1&&!next) )
	return true;

    RowCol nextrc;
    if ( !getNeighborNode( idx, true, nextrc, prev, next ) )
	return false;

    RowCol prevrc;
    if ( !getNeighborNode( idx, false, prevrc, prev, next ) )
	return false;

    const TypeSet<RowCol>& dirs=RowCol::clockWiseSequence();
    const RowCol& cur = nodes_[idx];
    int prevdir = dirs.indexOf((prevrc-cur).getDirection());
    int nextdir = dirs.indexOf((nextrc-cur).getDirection());

    int curdir = (prevdir+1)%dirs.size();
    while ( curdir!=nextdir )
    {
	const RowCol nrc = cur+dirs[curdir]*horizon_.geometry().step();
	if ( isNodeOK(nrc) )
	    return false;

	if ( ++curdir>=dirs.size() ) curdir = 0;
    }

    return true;
}


void EdgeLineSegment::fillPar( IOPar& par ) const
{
    TypeSet<SubID> subids;
    for ( int idx=0; idx<size(); idx++ )
	subids += nodes_[idx].toInt64();

    par.set( key(), subids );
    par.set( classnamestr(), className() );
}


bool EdgeLineSegment::usePar( const IOPar& par )
{
    TypeSet<SubID> subids;
    if ( !par.get( key(), subids ) )
	return false;

    nodes_.erase();
    for ( int idx=0; idx<subids.size(); idx++ )
	nodes_ += RowCol(subids[idx]);

    if ( notifier ) notifier->trigger();
    return true;
}


NotifierAccess* EdgeLineSegment::changeNotifier()
{
    if ( !notifier )
    {
	notifier = new Notifier<EdgeLineSegment>(this);
//	horizon_.notifier.notify(mCB(this,EdgeLineSegment,posChangeCB));
    }
	
    return notifier;
}


bool EdgeLineSegment::trackWithCache( int start, bool forward,
		 const EdgeLineSegment* prev, const EdgeLineSegment* next)
{
    if ( !size() ) return false;
    const RowCol& lastnode = nodes_[start];
    const RowCol& step = horizon_.geometry().step();
    RowCol backnode;
    if ( !getNeighborNode(start, !forward, backnode, prev, next ) )
    {
	if ( !getHorizonStart( start, !forward, backnode ) )
	    return false;
    }

    const RowCol backnodedir = (backnode-lastnode).getDirection();

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();
    int curdir = dirs.indexOf(backnodedir) + nrdirs;

    RowCol lastdefined;
    bool found = false;
    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	const RowCol& dir = dirs[curdir%dirs.size()];
	if ( !idx )
	    lastdefined = dir;
	else
	{
	    if ( !dir.isNeighborTo(lastdefined, RowCol(1,1) ) )
		break;

	    const RowCol testrc = lastnode+dir*step;

	    if ( isDefined(testrc) )
	    {
		lastdefined = dir;
		found = true;
	    }
	}

	if ( forward ) curdir--;
	else curdir++;
    }

    if ( !found )
	return false;

    const RowCol newrc = lastnode+lastdefined*step;
    if ( !isNodeOK(newrc) )
	return false;

    if ( indexOf(newrc)!=-1 || (next&&next->indexOf(newrc)!=-1) ||
	 (prev&&prev->indexOf(newrc)!=-1 ))
	return false;

    if ( forward )
    {
	if ( start==size()-1 ) (*this) += newrc;
	else set( start+1, newrc );
    }
    else 
    {
	if ( !start ) insert( 0, newrc );
	else set(start-1, newrc );
    }

    return true;
}


void EdgeLineSegment::reverse()
{
    TypeSet<RowCol> nodescopy(nodes_);
    for ( int idx=nodescopy.size()-1, idy=0; idx>=0; idx--, idy++ )
	nodes_[idy] = nodescopy[idx];
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::makeLine( const RowCol& start, const RowCol& stop )
{
    PtrMan<NotifyStopper> stopper = notifier ? new NotifyStopper(*notifier) : 0;
    ::makeLine( start, stop, horizon_.geometry().step(), nodes_ );

    if ( notifier )
    {
	stopper->restore();
	notifier->trigger();
    }
}


RowCol EdgeLineSegment::first() const
{ return size() ? nodes_[0] : RowCol(0,0); }


RowCol EdgeLineSegment::last() const
{ const int sz=size(); return sz ? nodes_[sz-1] : RowCol(0,0); }


#define mReTrackReturn(retval) \
{ \
    if ( notifier ) \
    { \
	stopper->restore(); \
	if ( change ) notifier->trigger(); \
    } \
    if ( change ) commitChanges(); \
    return retval; \
}


bool EdgeLineSegment::isNodeOK( const RowCol& rc ) const
{ return isAtEdge(rc) && isDefined(rc); }


bool EdgeLineSegment::reTrack( const EdgeLineSegment* prev,
			       const EdgeLineSegment* next )
{
    removeCache();
    PtrMan<NotifyStopper> stopper = notifier ? new NotifyStopper(*notifier) : 0;
    bool change = false;

    bool backward = false;
    if ( prev && next )
    {
	for ( int idy=0; idy<size(); idy++ )
	{
	    const RowCol& rc = (*this)[idy];
	    if ( isNodeOK(rc) && !isByPassed(idy,prev,next) )
		break;

	    nodes_.remove(idy--);
	    change = true;
	    backward = true;
	}

	if ( !size() )
	{
	    if ( !prev->size() ) mReTrackReturn(false);
	    nodes_.insert( 0, prev->last());
	    change = true;
	    if ( !trackWithCache( 0, true, prev, next ) )
		mReTrackReturn(false);
	    nodes_.remove(0);
	}

	if ( !prev->isContinuedBy(this) )
	    backward = true;

    }
    else
    {
	for ( int idy=0; idy<size(); idy++ )
	{
	    const RowCol& rc = (*this)[idy];
	    if ( isNodeOK(rc) )
		break;

	    nodes_.remove(idy--);
	    change = true;
	}

	if ( !size() )
	    mReTrackReturn(false);
    }

    if ( !prev || size()==1 )
	backward = true;

    int idx = 0;
    while ( true )
    {
	const int nextidx = idx+1;
	bool dotrack = false;
	if ( nextidx>=size() )
	    dotrack = true;
	else
	{
	    const RowCol& rc = (*this)[nextidx];
	    if ( !isConnToNext(idx) )
		dotrack = true;
	    else if ( !isNodeOK(rc) )
		dotrack = true;
	    else if ( isByPassed(idx,prev,next) )
		dotrack = true;
	    else if ( !areAllNodesOutsideBad(idx,prev,next) )
		dotrack = true;
	}

	if ( dotrack )
	{
	    if ( idx!=size()-1 )
	    {
		nodes_.remove( idx+1, size()-1 );
		change = true;
	    }
	    if ( !trackWithCache( idx, true, prev, next ) )
	    {
		if ( !next && !prev && isClosed() )
		    mReTrackReturn(true);
		if ( isContinuedBy(next) )
		    break;
		    // return true; // go backwards goto 329 break?

		break;
	    }
	    else
		change = true;
	}

	idx = nextidx;
    }

    if ( backward )
    {
	while ( true )
	{
	    if ( !trackWithCache( 0, false, prev, next ) )
	    {
		if ( !next && !prev && isClosed() )
		    mReTrackReturn(true);
		if ( prev && prev->isContinuedBy(this) )
		    mReTrackReturn(true);

		break;
	    }
	    else
		change = true;
	}
    }

    mReTrackReturn(false);
}


bool EdgeLineSegment::getNeighborNode( int idx, bool forward, RowCol& rc,
                                           const EdgeLineSegment* prev,
					   const EdgeLineSegment* next ) const
{
    if ( forward )
    {
	if ( idx<size()-1 ) rc = nodes_[idx+1];
	else if ( next ) rc = (*next)[0];
	else return false;
    }
    else 
    {
	if ( idx ) rc = nodes_[idx-1];
	else if ( prev ) rc = (*prev)[prev->size()-1];
	else return false;
    }

    return nodes_[idx].isNeighborTo(rc, horizon_.geometry().step(), true );
}


bool EdgeLineSegment::getHorizonStart( int start, bool clockwise, 
					   RowCol& rc ) const
{
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();

    const RowCol& step = horizon_.geometry().step();
    const RowCol& centernode = nodes_[start];

    //Find a place where it goes from undef to defined
    int firstdefined = -1;
    bool foundundef = false;

    int curdir = nrdirs*2;
    for ( int idx=0; idx<nrdirs+1; idx++ )
    {
	const RowCol& dir = dirs[curdir%nrdirs];
	if ( !isDefined( centernode+dir*step ) )
	{
	    foundundef = true;
	}
	else if ( foundundef ) 
	{
	    firstdefined = curdir%dirs.size();
	    break;
	}

	if ( clockwise ) curdir++;
	else curdir--;
    }

    if ( firstdefined==-1 ) 
	return false;

    rc = centernode+dirs[firstdefined]*step;
    return true;
}


int EdgeLineSegment::findNeighborTo(RowCol const& rc, bool forward) const
{
    const RowCol& step = horizon_.geometry().step();
    if ( forward )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    if ( rc.isNeighborTo(nodes_[idx], step, true ) )
		return idx;
	}
    }
    else
    {
	for ( int idx=size()-1; idx>=0; idx-- )
	{
	    if ( rc.isNeighborTo(nodes_[idx], step, true ) )
		return idx;
	}
    }

    return -1;
}


ObjectSet<EdgeLineSegmentFactory>&
EdgeLineSegment::factories()
{
    static ObjectSet<EdgeLineSegmentFactory> f;
    return f;
}


EdgeLineSegment* EM::EdgeLineSegment::factory( const IOPar& par,
       						   Horizon3D&	surf,
						   const SectionID& sect )
{
    BufferString name;
    if ( !par.get( classnamestr(), name ) )
	return 0;

    for ( int idx=0; idx<factories().size(); idx++ )
    {
	if ( strcmp( factories()[idx]->name, name.buf() ) )
	    continue;

	EdgeLineSegment* els = factories()[idx]->func( surf, sect );
	if ( !els ) return 0;
	if ( !els->usePar( par ) )
	{
	    delete els;
	    return 0;
	}

	return els;
    }

    return 0;
}


void EdgeLineSegment::posChangeCB(CallBacker* cb)
{
     mCBCapsuleUnpack(const EMObjectCallbackData&,cbdata,cb);
     if ( cbdata.event!=EMObjectCallbackData::PositionChange )
	 return;

     if ( cbdata.pid0.sectionID()!=section ) return;
     
     const RowCol rc(cbdata.pid0.subID());
     const int nodeidx = indexOf(rc);
     if ( nodeidx==-1 ) return;

     if ( !isDefined(rc) )
	 nodes_.remove(nodeidx);

     notifier->trigger();
}


EdgeLine::EdgeLine( EM::Horizon3D& surf, const EM::SectionID& sect )
    : horizon_( surf )
    , section( sect )
    , t2d( 0 )
    , changenotifier(this)
    , removezerosegs( true )
{}


EdgeLine* EM::EdgeLine::clone() const
{
    EdgeLine* res = new EdgeLine( horizon_, section );
    for ( int idx=0; idx<segments.size(); idx++ )
    {
	EdgeLineSegment* seg = segments[idx]->clone();
	seg->changeNotifier()->notify( mCB(res,EdgeLine,sectionChangeCB) );
	res->segments += seg;
    }
    
    return res;
}


void EdgeLine::setTime2Depth( const FloatMathFunction* nt2d )
{
    t2d = nt2d;
    for ( int idx=0; idx<segments.size(); idx++ )
	segments[idx]->setTime2Depth(t2d);
}


void EdgeLine::setSection( const EM::SectionID& s )
{
    section = s;
    for ( int idx=0; idx<segments.size(); idx++ )
	segments[idx]->setSection(s);
}


int EdgeLine::getSegment( const EM::PosID& pos, int* seq ) const
{
    if ( pos.objectID()!=horizon_.id() || pos.sectionID()!=section )
	return -1;

    return getSegment( RowCol(pos.subID()), seq );
}


int EdgeLine::getSegment( const RowCol& rowcol, int* seq,
			  const EdgeLineSegment* ignoreseg ) const 
{
    for ( int idx=0; idx<segments.size(); idx++ )
    {
	const int idy = segments[idx]==ignoreseg
	    ? -1 : segments[idx]->indexOf( rowcol );
	if ( idy!=-1 )
	{
	    if ( seq ) *seq=idy;
	    return idx;
	}
    }

    return -1;
}


bool EdgeLine::isClosed() const
{
    if ( !nrSegments() ) return false;

    int nrnodes = 0;
    for ( int idx=0; idx<nrSegments(); idx++ )
	nrnodes += segments[idx]->size();

    if ( nrnodes<4 ) return false;

    RowCol first;
    bool firstfound=false;
    for ( int idx=0; idx<nrSegments(); idx++ )
    {
	if ( !segments[idx]->size() )
	    continue;
	
	first = (*segments[idx])[0];
	firstfound=true;
	break;
    }

    if ( !firstfound ) return false;

    for ( int idx=nrSegments()-1; idx>=0; idx-- )
    {
	const int segsize = segments[idx]->size();
	if ( !segsize )
	    continue;

	return first.isNeighborTo((*segments[idx])[segsize-1], horizon_.geometry().step(),
				  true );
    }

    return false;
}


bool EdgeLine::isInside( const EM::PosID& pid, bool undefval ) const
{
    if ( pid.objectID()!=horizon_.id() || pid.sectionID()!=section )
	return undefval;

    return isInside( RowCol(pid.subID()), undefval );
}


bool EdgeLine::isInside( const RowCol& rc, bool undefval ) const
{
    if ( !isClosed() )
	return undefval;

    for ( int idx=0; idx<segments.size(); idx++ )
    {
	if ( segments[idx]->indexOf(rc)!=-1 )
	    return false;
    }

    EdgeLineIterator iter( *this );
    if ( !iter.isOK() ) return undefval;


    int shortestdistsquare, segment, nodeidx;
    bool first = true;
    do
    {
	const RowCol diff = rc-iter.currentRowCol();
	const int distsquare = diff.row*diff.row+diff.col*diff.col;
	if ( first || distsquare<shortestdistsquare )
	{
	    if ( !distsquare ) return undefval;
	    shortestdistsquare = distsquare;
	    first = false;
	    nodeidx = iter.currentNodeIdx();
	    segment = iter.currentSegment();
	}
    } while ( iter.next() );

    RowCol nextrc;
    if ( nodeidx==segments[segment]->size()-1 )
    {
	const EdgeLineSegment* els = segment==nrSegments()-1
	    		? segments[0]
			: segments[segment+1];
	nextrc = els->first();
    }
    else
	nextrc = (*segments[segment])[nodeidx+1];

    RowCol prevrc;
    if ( !nodeidx )
    {
	const EdgeLineSegment* els = segment ? segments[segment-1]
					         : segments[nrSegments()-1];
	prevrc = els->last();
    }
    else
	prevrc = (*segments[segment])[nodeidx-1];

    const RowCol& closest = (*segments[segment])[nodeidx];
    const RowCol prevdir = (prevrc-closest).getDirection();
    const RowCol nextdir = (nextrc-closest).getDirection();

    const float angletorc = nextdir.clockwiseAngleTo(rc-closest);
    const float angletoprev = nextdir.clockwiseAngleTo(prevdir);

    return angletorc>=0 && angletorc<=angletoprev;
}


bool EdgeLine::isHole() const
{
    if ( !isClosed() )
	return false;

    const RowCol& step = horizon_.geometry().step();

    TypeSet<RowCol> rcs;
    RowCol prevrc;
    for ( int segment=0; segment<segments.size(); segment++ )
    {
	for ( int idx=0; idx<segments[segment]->size(); idx++ )
	{
	    const RowCol rc = (*segments[segment])[idx];
	    if ( (!idx && !segment) || rc.isNeighborTo(prevrc,step,true) )
	    {
		prevrc = rc;
		rcs += rc;
	    }
	    else
		return false;
	}
    }

    int nrextra = 2;
    for ( int segment=0; nrextra && segment<segments.size(); segment++ )
    {
	for ( int idx=0; nrextra && idx<segments[segment]->size(); idx++ )
	{
	    const RowCol rc = (*segments[segment])[idx];
	    if ( (!idx && !segment) || rc.isNeighborTo(prevrc,step,true) )
	    {
		prevrc = rc;
		rcs += rc;
		nrextra--;
	    }
	    else
		return false;
	}
    }


    float anglediff = 0;
    for ( int idx=1; idx<rcs.size()-1; idx++ )
    {
	anglediff += (rcs[idx-1]-rcs[idx]).getDirection().clockwiseAngleTo(
		     (rcs[idx+1]-rcs[idx]).getDirection()) - M_PI;
    }

    return anglediff<0;
}


bool EdgeLine::setRemoveZeroSegments(bool newstatus)
{
    const bool prevstatus = removezerosegs;
    removezerosegs = newstatus;
    if ( newstatus && !prevstatus )
    {
	bool change = false;
	for ( int idx=0; idx<segments.size(); idx++ )
	{
	    if ( segments[idx]->size() )
		continue;

	    segments[idx]->changeNotifier()->remove(
		    mCB(this,EdgeLine,sectionChangeCB) );
	    delete segments[idx];
	    segments.remove(idx--);
	    change = true;
	}

	if ( change ) 
	    changenotifier.trigger();
    }

    return prevstatus;
}




int EdgeLine::computeArea() const
{
    if ( !isClosed() )
	return -1;

    TypeSet<RowCol> nodesinside;
    for ( int idx=0; idx<segments.size(); idx++ )
	nodesinside.append( segments[idx]->getNodes() );

    int layer2start = nodesinside.size();
    int layer1start = 0;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const RowCol& step = horizon_.geometry().step();
    for ( int idx=0; idx<layer2start+2; idx++ )
    {
	const RowCol backnode = nodesinside[(idx)%layer2start];
	const RowCol curnode = nodesinside[(idx+1)%layer2start];
	const RowCol nextnode = nodesinside[(idx+2)%layer2start];

	const RowCol backnodedir = (backnode-curnode).getDirection();
	const RowCol nextnodedir = (nextnode-curnode).getDirection();

	const int backnodeidx = dirs.indexOf((backnode-curnode).getDirection());
	const int nextnodeidx = dirs.indexOf((nextnode-curnode).getDirection());

	int curdir = (nextnodeidx+1)%dirs.size();
	while ( curdir!=backnodeidx )
	{
	    const RowCol seed = curnode+step*dirs[curdir];
	    if ( isInside(seed,false) && nodesinside.indexOf(seed)==-1 )
		nodesinside+= seed;

	    curdir = (curdir+1)%dirs.size();
	}
    }

    int nrremovednodes = 0;
    while ( nodesinside.size() )
    {
	const int removestart = 0;
	const int removestop = layer1start-1;
	const int nrtoremove = removestop-removestart+1;
	if ( nrtoremove>0 ) nodesinside.remove(removestart,removestop);

	layer1start = layer2start-nrtoremove;
	layer2start = nodesinside.size();
	nrremovednodes += nrtoremove;
	int currentseed = layer1start;

	const int stop = nodesinside.size();

	while ( currentseed<stop )
	{
	    const RowCol& currc = nodesinside[currentseed];
	    for ( int row=-step.row; row<=step.row; row+=step.row )
	    {
		for ( int col=-step.col; col<=step.col; col+=step.col )
		{
		    //Only add with 4-connectivity
		    if ( (col && row) || (!row && !col) ) continue;

		    const RowCol rc(currc.row+row, currc.col+col );
		    if ( nodesinside.indexOf(rc,false)!=-1 )
			continue;

		    nodesinside+= rc;
		}
	    }

	    currentseed++;
	}
    }

    return nodesinside.size() + nrremovednodes;
}


void EdgeLine::insertSegments( ObjectSet<EM::EdgeLineSegment>& ns, int idx, 
		     bool cutexisting )
{
    NotifyStopper notifystop( changenotifier );

    for ( int idy=0; idy<ns.size()-1; idy++ )
    {
	if ( !ns[idy]->isContinuedBy(ns[idy+1] ) )
	{
	    deepErase( ns );
	    return;
	}
    }

    if ( cutexisting )
    {
	idx = cutLineBy(ns[0]->first(), ns[ns.size()-1]->last() );
	if ( idx==-1 )
	{
	    deepErase(ns);
	    return;
	}
    }


    for ( int idy=0; idy<ns.size(); idy++ )
    {
	ns[idy]->commitChanges();
	ns[idy]->setTime2Depth(t2d);
	const int newidx = idx+idy;
	if ( newidx>=segments.size() )
	    segments += ns[idy];
	else
	    segments.insertAt( ns[idy], newidx );


	ns[idy]->changeNotifier()->notify(
		mCB(this,EdgeLine,sectionChangeCB) );
    }

    //Check for bypasses at start & end
    int previdx = idx-1;
    if ( previdx<0 ) previdx = segments.size()-1;
    int prevprevidx = previdx-1;
    if ( prevprevidx<0 ) prevprevidx = segments.size()-1;
    if ( segments[previdx]->isByPassed( segments[previdx]->size()-1,
					segments[prevprevidx],
					segments[idx]) )
    {
	segments[previdx]->remove(segments[previdx]->size()-1);
    }
   
    int curidx = segments.indexOf(ns[ns.size()-1]); 
    int nextidx = curidx+1;
    if ( nextidx>=segments.size() ) nextidx = 0;
    int nextnextidx = nextidx+1;
    if ( nextnextidx>=segments.size() ) nextnextidx = 0;
    if ( segments[nextidx]->isByPassed( 0,
					segments[curidx],
					segments[nextnextidx]) )
    {
	segments[nextidx]->remove(0);
    }

    reduceSegments();

    notifystop.restore();
    changenotifier.trigger();
}


void EdgeLine::insertSegment( EdgeLineSegment* els, int idx,
				  bool cutexisting )
{
    ObjectSet<EdgeLineSegment> set;
    set += els;
    insertSegments( set, idx, cutexisting );
}


bool EdgeLine::reTrackLine()
{
    bool prevremovestatus = setRemoveZeroSegments(false);

    int highestretrackindex = 0;
    for ( int idz=0; idz<nrSegments(); idz++ )
    {
	if ( getSegment(idz)->reTrackOrderIndex()>highestretrackindex )
	    highestretrackindex = getSegment(idz)->reTrackOrderIndex();
    }

    for ( int index=highestretrackindex; index>=0; index-- )
    {
	for ( int idz=0; idz<nrSegments(); idz++ )
	{
	    EM::EdgeLineSegment* segment = getSegment(idz);
	    if ( segment->reTrackOrderIndex()!= index ||
		 !segment->size() || !segment->canTrack())
		continue;

	    EM::EdgeLineSegment* prevseg = 0;
	    for ( int idu=idz-1; idu!=idz && !prevseg; idu-- )
	    {
		if ( idu<0 ) idu = nrSegments()-1;
		if ( getSegment(idu)->size() )
		    prevseg =  getSegment(idu);
	    }

	    if ( prevseg->reTrackOrderIndex()<index ) prevseg = 0;

	    EM::EdgeLineSegment* nextseg = 0;
	    for ( int idu=idz+1; idu!=idz && !nextseg; idu++ )
	    {
		if ( idu>=nrSegments()) idu = 0;
		if ( getSegment(idu)->size() )
		    nextseg =  getSegment(idu);
	    }

	    if ( nextseg && nextseg->reTrackOrderIndex()<index ) nextseg = 0;

	    segment->reTrack( prevseg, nextseg );
	    cutLineBy( segment->first(), segment->last(), segment );
	}
    }

    setRemoveZeroSegments(prevremovestatus);

    return true;
}


bool EdgeLine::repairLine()
{
    const RowCol& step = horizon_.geometry().step();

    for ( int idz=0; idz<nrSegments(); idz++ )
    {
	EM::EdgeLineSegment* segment = getSegment(idz);
	if ( !segment->size() )
	    continue;

	int previdx = -1;
	for ( int idu=idz-1; idu!=idz && previdx==-1; idu-- )
	{
	    if ( idu<0 ) idu = nrSegments()-1;
	    if ( getSegment(idu)->size() )
		previdx = idu;
	}
	EM::EdgeLineSegment* prevseg = getSegment(previdx);

	if ( prevseg->isContinuedBy(segment) &&
	     prevseg->reTrackOrderIndex()==segment->reTrackOrderIndex() )
	    continue;

	const bool forward = segment->reTrackOrderIndex()<=
			     prevseg->reTrackOrderIndex();
	const RowCol start = forward ? prevseg->last() : segment->first();
	RowCol incomingdir, outgoingdir;

	if ( forward )
	{
	    if ( prevseg->size()>1 ) incomingdir =
		(*prevseg)[prevseg->size()-2]-start;
	    else
	    {
		EM::EdgeLineSegment* nextseg = 0;
		for ( int idu=previdx-1; idu!=idz && !nextseg; idu-- )
		{
		    if ( idu<0 ) idu = nrSegments()-1;
		    if ( getSegment(idu)->size() )
			nextseg = getSegment(idu);
		}

		if ( !nextseg ) return false;
		incomingdir = nextseg->last()-start;
	    }
	}
	else
	{
	    if ( segment->size()>1 ) outgoingdir = (*segment)[1]-start;
	    else
	    {
		EM::EdgeLineSegment* nextseg = 0;
		for ( int idu=idz+1; idu!=idz && !nextseg; idu++ )
		{
		    if ( idu>=nrSegments()) idu = 0;
		    if ( getSegment(idu)->size() )
			nextseg = getSegment(idu);
		}

		if ( !nextseg ) return false;
		outgoingdir = nextseg->first()-start;
	    }
	}

	const int startidx = forward ? 0 : prevseg->size()-1;
	EdgeLineIterator iterator( *this, forward,
				   forward ? idz : previdx, startidx );
	RowCol stop = iterator.currentRowCol();

	TypeSet<int> segmentstoremove;
	int cursegidx = iterator.currentSegment();
	int curnodeidx = iterator.currentNodeIdx();
	bool stopischanged = false;

	do
	{
	    const EM::EdgeLineSegment* curseg =
			    getSegment(iterator.currentSegment());
	    if ( curseg->reTrackOrderIndex()>prevseg->reTrackOrderIndex() )
		break;

	    const RowCol rc = iterator.currentRowCol();
	    if ( forward )
		outgoingdir = rc-start;
	    else
		incomingdir = rc-start;

	    if ( stop!=rc )
	    {
		stop = rc;
		stopischanged = true;

		cursegidx = iterator.currentSegment();
		curnodeidx = iterator.currentNodeIdx();

		if ( segmentstoremove.indexOf(cursegidx)==-1 )
		     segmentstoremove += cursegidx;
	    }

	    if ( outgoingdir.clockwiseAngleTo(incomingdir)<=3*M_PI/2+0.01 )
		break;

	} while ( iterator.next() );

	if ( !stopischanged ) 
	    continue;

	segmentstoremove -= cursegidx;
	const bool prevremovestatus = setRemoveZeroSegments(false);
	TypeSet<RowCol> empty;
	for ( int idu=0; idu<segmentstoremove.size(); idu++ )
	    getSegment(segmentstoremove[idu])->copyNodesFrom(empty,false);

	if ( forward )
	{
	    segment = getSegment(cursegidx);
	    if ( curnodeidx>0 )
		segment->remove(0,curnodeidx-1);
	}
	else
	{
	    prevseg = getSegment(cursegidx);
	    if ( curnodeidx<prevseg->size()-1 )
		prevseg->remove(curnodeidx+1, prevseg->size()-1 );

	}

	setRemoveZeroSegments(prevremovestatus);

	TypeSet<RowCol> rcs;
	::makeLine( forward ? start : stop, forward ? stop : start, step, rcs );

	rcs.remove(0);
	rcs.remove(rcs.size()-1);

	if ( rcs.size() )
	{
	    if ( !prevseg->reTrackOrderIndex() )
		prevseg->insert(prevseg->size(), rcs );
	    else if ( !segment->reTrackOrderIndex() )
		segment->insert(0, rcs);
	    else
	    {
		EM::EdgeLineSegment* helpsegment = new
		    EM::EdgeLineSegment( horizon_, section );
		helpsegment->insert(0,rcs);
		insertSegment( helpsegment, -1, true );
	    }
	}

	idz = -1;
    }

    return true;
}


int EdgeLine::cutLineBy( const RowCol& start, const RowCol& stop,
			 const EdgeLineSegment* donttouch )
{
    int startidx;
    int startseg = getSegment( start, &startidx, donttouch );
    if ( startseg==-1 )
    {
        startseg = getSegment( start, &startidx );
	if ( startseg==-1 )
	    return -1;
    }

    int stopidx;
    int stopseg = getSegment( stop, &stopidx, donttouch );
    if ( stopseg==-1 )
    {
	stopseg = getSegment( stop, &stopidx );
	if ( stopseg==-1 )
	    return -1;
    }

    const bool didremovesegments = setRemoveZeroSegments(false);
    int curseg = startseg;
    int curidx = startidx;

    while ( true )
    {
	if ( curseg!=stopseg || curidx>stopidx )
	{
	    if ( segments[curseg]!=donttouch )
		segments[curseg]->remove( curidx, segments[curseg]->size()-1 );
	}
	else
	{
	    if ( segments[curseg]==donttouch )
	    {}
	    else if ( nrSegments()==1 )
	    {
		for ( int idy=segments[curseg]->size()-1; idy>stopidx;idy--)
		{
		    segments[curseg]->insert(0,(*segments[curseg])[idy]);
		    stopidx++;
		    curidx++;
		    idy++;
		}

		segments[curseg]->remove(curidx, segments[curseg]->size());
	    }
	    else
	    {
		if ( !curidx || stopidx==segments[curseg]->size())
		    segments[curseg]->remove(curidx,stopidx);
		else
		{
		    EdgeLineSegment* nels = segments[curseg]->clone();
		    nels->remove(0,stopidx);
		    if ( segments.size()<=curseg+1 )
			segments += nels;
		    else
			segments.insertAt(nels,curseg+1);

		    segments[curseg]->remove(startidx,segments[curseg]->size());
		}
	    }
	    break;
	}

	if ( ++curseg>=segments.size() )
	    curseg = 0;
	curidx = 0;
    }

    setRemoveZeroSegments(didremovesegments);

    return segments.size() ? (startseg+1)%segments.size() : -1;
}


void EdgeLine::fillPar( IOPar& par ) const
{
    par.set( nrsegmentsstr(), nrSegments() );
    for ( int idx=0; idx<nrSegments(); idx++ )
    {
	IOPar segmentpar;
	segments[idx]->fillPar( segmentpar );

	BufferString key = segmentprefixstr();
	key += idx;

	par.mergeComp( segmentpar, key.buf() );
    }
}


bool EdgeLine::usePar( const IOPar& par )
{
    deepErase( segments );
    int nrsegments = 0;
    par.get( nrsegmentsstr(), nrsegments );
    for ( int idx=0; idx<nrsegments; idx++ )
    {
	BufferString key = segmentprefixstr();
	key += idx;

	PtrMan<IOPar> segmentpar = par.subselect( key.buf() );
	if ( !segmentpar ) 
	    continue;

	EdgeLineSegment* els = EM::EdgeLineSegment::factory( *segmentpar,
				    horizon_, section );
	if ( !els )
	    continue;

	segments += els;
	els->changeNotifier()->notify( mCB(this,EdgeLine,sectionChangeCB) );
    }

    return true;
}


void EdgeLine::sectionChangeCB(CallBacker* cb)
{
    mDynamicCastGet(EdgeLineSegment*, segment, cb );
    if ( removezerosegs && !segment->size() )
    {
	//stop traversal
	segment->changeNotifier()->disable();
	segment->changeNotifier()->remove( mCB(this,EdgeLine,sectionChangeCB) );
	segments -= segment;
	delete segment;
    }

    changenotifier.trigger();
}


int EdgeLine::findNeighborTo( const RowCol& rc, int startseg, int startpos,
				  bool forward, int* segpos ) const
{
    EdgeLineIterator iter( *this, forward, startseg, startpos );
    if ( !iter.isOK() ) return -1;

    const RowCol& step = horizon_.geometry().step();

    do
    {
	if ( rc.isNeighborTo(iter.currentRowCol(),step, true ) )
	{
	    if ( segpos ) *segpos = iter.currentNodeIdx();
	    return iter.currentSegment();
	}
    } while ( iter.next() );

    return -1;
}


void EdgeLine::reduceSegments()
{
    NotifyStopper notifystop( changenotifier );
    for ( int idx=0; idx<segments.size(); idx++ )
    {
	if ( !segments[idx]->size() )
	{
	    delete segments[idx];
	    segments.remove(idx--);
	}
    }

    for ( int idx=0; idx<segments.size(); idx++ )
    {
	EdgeLineSegment& curseg = *segments[idx];
	EdgeLineSegment& nextseg = *segments[(idx+1)%segments.size()];
	if ( &curseg!=&nextseg &&
	     curseg.last().isNeighborTo(nextseg.first(), horizon_.geometry().step() ) &&
	     curseg.haveIdenticalSettings(nextseg) )
	{
	    for ( int idy=0; idy<nextseg.size(); idy++ )
		curseg += nextseg[idy];

	    delete &nextseg;
	    segments -= &nextseg;
	}
    }

    notifystop.restore();
    changenotifier.trigger();
}


#define mSetRC( rc, oper, rowcol ) \
    if ( currc.rowcol oper rc.rowcol ) rc.rowcol = currc.rowcol;

void EdgeLine::getBoundingBox( RowCol& start, RowCol& stop ) const
{
    EdgeLineIterator iterator( *this );
    start = stop = iterator.currentRowCol();
    while ( iterator.next() )
    {
	const RowCol& currc = iterator.currentRowCol();
	if ( currc.row < start.row ) start.row = currc.row;
	if ( currc.col < start.col ) start.col = currc.col;
	if ( currc.row > stop.row ) stop.row = currc.row;
	if ( currc.col > stop.col ) stop.col = currc.col;
    }
}


// EdgeLineIterator

bool EdgeLineIterator::isOK() const
{
    if ( segment<0 || segment>=el.nrSegments() )
	return false;

    const EdgeLineSegment* els = el.getSegment(segment);
    return nodeidx>=0 && nodeidx<els->size();
}


bool EdgeLineIterator::next()
{
    if ( forward ) nodeidx++;
    else nodeidx--;

    if ( nodeidx<0 )
    {
	if ( --segment<0 )
	    segment = el.nrSegments()-1;

	nodeidx = el.getSegment(segment)->size()-1;
    }
    else if ( nodeidx>=el.getSegment(segment)->size() )
    {
	if ( ++segment>=el.nrSegments() )
	    segment = 0;

	nodeidx = 0;
    }

    if ( segment==startseg && nodeidx==startpos )
    {
	nrturns++;
	return false;
    }

    return true;
}


PosID EdgeLineIterator::current() const
{
    const Horizon3D& horizon_ = el.getHorizon();
    return PosID(horizon_.id(),el.getSection(),currentRowCol().toInt64() );
}


EdgeLineSet::EdgeLineSet(EM::Horizon3D& surf, const EM::SectionID& sect)
    : horizon_(surf), section(sect), changenotifier(this)
{}


EdgeLineSet::~EdgeLineSet()
{
    removeAll();
}



EdgeLineSet* EM::EdgeLineSet::clone() const
{
    EdgeLineSet* res = new EdgeLineSet( horizon_, section );
    for ( int idx=0; idx<lines.size(); idx++ )
    {
	EdgeLine* line = lines[idx]->clone();
	line->changenotifier.notify( mCB(res, EdgeLineSet, changedLineCB ));
	res->lines += line;
    }
    
    return res;
}


int EdgeLineSet::addLine( EdgeLine* line )
{
    line->changenotifier.notify( mCB(this,EdgeLineSet,changedLineCB) );
    lines+=line;
    return nrLines()-1;
}


void EdgeLineSet::setSection(const EM::SectionID& s)
{
    section = s;
    for ( int idx=0; idx<lines.size(); idx++ )
	lines[idx]->setSection(s);

    changenotifier.trigger();
}


bool EdgeLineSet::isOnLine( const RowCol& rc, int* lineidx,
			    int* segmentidx, int* segmentpos ) const
{
    int segment;
    for ( int idx=0; idx<lines.size(); idx++ )
    {
	segment = lines[idx]->getSegment( rc, segmentpos );
	if ( segment!=-1 )
	{
	    if ( lineidx ) *lineidx=idx;
	    if ( segmentidx ) *segmentidx=segment;
	    return true;
	}
    }

    return false;
}


int EdgeLineSet::getMainLine() const
{
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	const EdgeLine* tel = lines[idx];
	if ( tel->isClosed() && !tel->isHole() )
	    return idx;
    }

    return -1;
}


EdgeLine* EM::EdgeLineSet::getLine(int idx)
{
    if ( idx<0 || idx>=lines.size() ) return 0;
    return lines[idx];
}


const EdgeLine* EM::EdgeLineSet::getLine(int idx) const
{ return const_cast<EdgeLineSet*>(this)->getLine(idx); }


bool EdgeLineSet::removeAllNodesOutsideLines()
{
    ObjectSet<EdgeLine> holes;
    ObjectSet<EdgeLine> nonholes;
    for ( int idx=0; idx<lines.size(); idx++ )
    {
	if ( !lines[idx]->isClosed() )
	    return false;

	if ( lines[idx]->isHole() )
	    holes += lines[idx];
	else
	    nonholes += lines[idx];
    }

    const StepInterval<int> rowrange = horizon_.geometry().rowRange(section);
    const StepInterval<int> colrange = horizon_.geometry().colRange(section,-1);
    for ( int row=rowrange.start; row<=rowrange.stop; row+=rowrange.step )
    {
	for ( int col=colrange.start; col<=colrange.stop; col+=colrange.step )
	{
	    const RowCol rc(row,col);
	    if ( !horizon_.isDefined(section,rc.toInt64()) )
		continue;

	    const PosID pid(horizon_.id(),section,rc.toInt64());

	    bool foundinnonhole = false;
	    for ( int idx=0; idx<nonholes.size(); idx++ )
	    {
		if ( nonholes[idx]->isInside(pid,false) )
		{
		    foundinnonhole = true;
		    break;
		}
	    }

	    if ( foundinnonhole )
	    {
		bool notinside = false;
		for ( int idx=0; idx<holes.size(); idx++ )
		{
		    if ( !holes[idx]->isInside(pid,false) )
		    {
			notinside = true;
			break;
		    }
		}

		if ( !notinside )
		    continue;
	    }

	    horizon_.unSetPos(pid,true);
	}
    }

    return true;
}


bool EdgeLineSet::findLines( EdgeLineCreationFunc func )
{
    removeAll();

    const StepInterval<int> rowrange = horizon_.geometry().rowRange();
    if ( rowrange.width(false)<2 )
	return false;

    const StepInterval<int> colrange = horizon_.geometry().colRange();
    if ( colrange.width(false)<2 )
	return false;

    for ( int row=rowrange.start; row<=rowrange.stop; row+=rowrange.step )
    {
	for ( int col=colrange.start; col<=colrange.stop;col+=colrange.step)
	{
	    const RowCol rc( row, col );
	    const PosID pid(horizon_.id(),rc.toInt64() );

	    TypeSet<PosID> linkedpos;
	    horizon_.geometry().getLinkedPos( pid, linkedpos );
	    if ( linkedpos.size() )
	    {
		//Todo
		continue;
	    }

	    if ( horizon_.geometry().isAtEdge(pid) )
	    {
		bool found = false;
		for ( int idx=0; idx<lines.size(); idx++ )
		{
		    if ( lines[idx]->getSegment(pid) != -1 )
		    {
			found = true;
			break;
		    }
		}

		if ( found ) 
		    continue;

		EdgeLineSegment* els = func( horizon_, section );
		(*els) += rc;
		int idx=0;
		while ( els->track(idx++,true) )
		    ;

		if ( els->isClosed() )
		{
		    EdgeLine* el = new EdgeLine( horizon_, section );
		    el->changenotifier.notify(
			    mCB(this,EdgeLineSet, changedLineCB ));
		    lines += el;
		    el->insertSegment( els, 0, false );
		}
		else
		    delete els;
	    }
	}
    }

    return lines.size();
}


void EdgeLineSet::changedLineCB(CallBacker*)
{ changenotifier.trigger(); }


void EdgeLineSet::fillPar( IOPar& par ) const
{
    par.set( nrlinesstr(), nrLines() );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BufferString key = lineprefixstr();
	key += idx;

	IOPar linepar;
	lines[idx]->fillPar( linepar );
	par.mergeComp( linepar, key.buf() );
    }
}

void EdgeLineSet::removeAll()
{
    for ( int idx=0; idx<lines.size(); idx++ )
	if ( lines[idx] ) lines[idx]->changenotifier.remove( 
					mCB(this,EdgeLineSet,changedLineCB) );

    deepErase( lines );
    changenotifier.trigger();
}


bool EdgeLineSet::usePar( const IOPar& par )
{
    removeAll();

    int nrlines = 0;
    par.get( nrlinesstr(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	BufferString key = lineprefixstr();
	key += idx;

	PtrMan<IOPar> linepar = par.subselect(key.buf());
	if ( !linepar ) continue;
	EdgeLine* el = new EdgeLine( horizon_, section );
	if ( !el->usePar( *linepar ) )
	{
	    delete el;
	    return false;
	}

	lines += el;
	el->changenotifier.notify( mCB(this,EdgeLineSet, changedLineCB ));
    }

    return true;
}


EdgeLineManager::EdgeLineManager( EM::Horizon3D& surf )
    : horizon_(surf)
    , addremovenotify( this )
{
    linesets.allowNull();
}


EdgeLineManager::~EdgeLineManager()
{
    removeAll();
}


void EdgeLineManager::updateEL( CallBacker* )
{
    for ( int sidx=0; sidx<horizon_.geometry().nrSections(); sidx++ )
    {
	EdgeLineSet* elset = getEdgeLineSet( horizon_.geometry().sectionID(sidx),
					     false );
	if ( !elset ) continue;
	for ( int lidx=0; lidx<elset->nrLines(); lidx++ )
	    elset->getLine( lidx )->reTrackLine();
    }
}


const EdgeLineSet* EM::EdgeLineManager::getEdgeLineSet(
	const SectionID& section ) const
{
    return const_cast<EdgeLineManager*>(this)->
					getEdgeLineSet( section, false );
}


EdgeLineSet* EM::EdgeLineManager::getEdgeLineSet(
	const SectionID& section, bool create )
{
    const int sectionnr = horizon_.geometry().sectionIndex(section);
    if ( sectionnr==-1 )
	return 0;

    while ( sectionnr>=linesets.size() ) linesets += 0;

    if ( !linesets[sectionnr] && create )
    {
	EdgeLineSet* els = new EdgeLineSet( horizon_, section );
	if ( !els->findLines() )
	    delete els;
	else
	{
	    linesets.replace( sectionnr, els );
	    addremovenotify.trigger( section );
	}
    }

    return linesets[sectionnr];
}


void  EdgeLineManager::cloneEdgeLineSet( const EM::SectionID& src,
					     const SectionID& dst )
{
    EdgeLineSet* srcset = getEdgeLineSet(src,false);
    if ( !srcset ) return;

    EdgeLineSet* els = srcset->clone();
    els->setSection( dst );

    const int dstidx = horizon_.geometry().sectionIndex(dst);
    while ( dstidx>=linesets.size() ) linesets += 0;
    linesets.replace(dstidx, els);
    addremovenotify.trigger( dst );
}


void EdgeLineManager::removeSection( const SectionID& pid )
{
    const int nr = horizon_.geometry().sectionIndex(pid);
    if ( nr<0 || nr>=linesets.size() ) return;
    delete linesets[nr];
    linesets.remove(nr);
    addremovenotify.trigger( pid );
}


void EdgeLineManager::removeLineSet( const SectionID& pid )
{
    const int nr = horizon_.geometry().sectionIndex(pid);
    if ( nr<0 || nr>=linesets.size() ) return;
    delete linesets[nr];
    linesets.replace(nr,0);
    addremovenotify.trigger( pid );
}


void EdgeLineManager::removeAll()
{
    TypeSet<SectionID> pids;
    for ( int idx=0; idx<linesets.size(); idx++ )
	if ( linesets[idx] )
	    pids += linesets[idx]->getSection();

    deepErase( linesets );

    for ( int idx=0; idx<pids.size(); idx++ )
	addremovenotify.trigger( pids[idx] );
}


void EdgeLineManager::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<horizon_.geometry().nrSections(); idx++ )
    {
	const EdgeLineSet* els = getEdgeLineSet(horizon_.geometry().sectionID(idx) );
	if ( !els ) continue;

	IOPar elspar;
	els->fillPar( elspar );
	BufferString key = sectionkey();
	key += horizon_.geometry().sectionID(idx);
	par.mergeComp( elspar, key.buf() );
    }
}


bool EdgeLineManager::usePar( const IOPar& par )
{
    removeAll();
    for ( int idx=0; idx<horizon_.geometry().nrSections(); idx++ )
    {
	BufferString key = sectionkey();
	key += horizon_.geometry().sectionID(idx);

	PtrMan<IOPar> elspar = par.subselect(key.buf());
	if ( !elspar ) continue;

	EdgeLineSet* els = new EdgeLineSet( horizon_, horizon_.geometry().sectionID(idx) );
	if ( !els->usePar( *elspar ) )
	{
	    delete els;
	    return false;
	}

	linesets += els;
	addremovenotify.trigger( els->getSection() );
    }

    return true;
}

}; //namespace
