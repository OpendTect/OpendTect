/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfaceedgeline.cc,v 1.1 2004-08-20 08:27:15 kristofer Exp $";
   

#include "emsurfaceedgeline.h"
#include "emmanager.h"
#include "emsurface.h"
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


const char* EdgeLineSegment::key = "Nodes";

const char* EdgeLineSegment::classnamestr = "Segment Type";


const char* EdgeLine::nrsegmentsstr = "Nr segments";
const char* EdgeLine::segmentprefixstr = "Segment ";

const char* EdgeLineSet::nrlinesstr = "Nr lines";
const char* EdgeLineSet::lineprefixstr = "Line ";

const char* EdgeLineManager::sectionkey = "Sectionlines ";


mEdgeLineSegmentFactoryEntry(EdgeLineSegment);


EdgeLineSegment::EdgeLineSegment( Surface& surf,
				      const SectionID& sect )
    : surface( surf )
    , section( sect )
    , notifier( 0 )
{}


EdgeLineSegment::EdgeLineSegment( const EdgeLineSegment& templ)
    : surface( templ.surface )
    , section( templ.section )
    , nodes( templ.nodes )
    , notifier( 0 )
{}


EdgeLineSegment::~EdgeLineSegment()
{
    if ( notifier )
	surface.poschnotifier.remove(mCB(this,EdgeLineSegment,posChangeCB));

    delete notifier;
}


bool EdgeLineSegment::haveIdenticalSettings( const EdgeLineSegment& seg ) const
{
    return !strcmp(typeid(*this).name(),typeid(seg).name()) &&
	   internalIdenticalSettings(seg);
}


bool EdgeLineSegment::internalIdenticalSettings(
					const EdgeLineSegment& seg) const
{
    return &surface==&seg.surface && section==seg.section;
}


int EdgeLineSegment::size() const
{ return nodes.size(); }


int EdgeLineSegment::indexOf( const RowCol& rc, bool forward ) const
{ return nodes.indexOf(rc, forward ); }


void EdgeLineSegment::remove( int p1 )
{
    nodes.remove(p1);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::remove( int p1, int p2 )
{
    nodes.remove(p1,p2);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::removeAll()
{
    nodes.erase();
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::insert( int p1, const RowCol& rc )
{
    nodes.insert(p1,rc);
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::set( int p1, const RowCol& rc )
{
    if ( p1>=0 && p1<size() )
    {
	if ( rc!=nodes[p1] )
	{
	    nodes[p1] = rc;
	    if ( notifier ) notifier->trigger();
	}
    }
    else if ( p1==size() )
	(*this) += rc;
}


const RowCol& EdgeLineSegment::operator[](int idx) const
{ return nodes[idx]; }


const EdgeLineSegment&
EdgeLineSegment::operator+=(const RowCol& rc)
{
    nodes += rc;
    if ( notifier ) notifier->trigger();
    return *this;
}


bool EdgeLineSegment::isClosed() const
{ return size()>3 && isContinuedBy(this); }


bool EdgeLineSegment::isContinuedBy(  const EdgeLineSegment* seg ) const
{
    const int sz = size();
    return sz && seg->size() &&
	   nodes[sz-1].isNeighborTo((*seg)[0], surface.geometry.step());
}


bool EdgeLineSegment::isDefined( const RowCol& rc ) const
{ return surface.geometry.isDefined( section, rc ); }


bool EdgeLineSegment::isAtEdge( const RowCol& rc ) const
{ 
    const PosID pid( surface.id(), section,
	    		 surface.geometry.rowCol2SubID(rc));
    return surface.geometry.getNeighbors(pid, 0, 1, false)!=8;
}


bool EdgeLineSegment::isByPassed(int idx, const EdgeLineSegment* prev,
					 const EdgeLineSegment* next) const
{
    if ( (!idx && (!prev || !prev->isContinuedBy(this) )) ||
	   idx==size()-1 && (!next || !isContinuedBy(next) ) )
	return false;

    const RowCol& prevrc = idx ? nodes[idx-1] : (*prev)[prev->size()-1];
    const RowCol& nextrc = idx!=size()-1 ? nodes[idx+1] : (*next)[0];
    if ( !prevrc.isNeighborTo(nextrc,surface.geometry.step(), true ) )
	return false;

    const RowCol& rc = nodes[idx];
    const RowCol prevdir = (prevrc-rc).getDirection();
    const RowCol nextdir = (nextrc-rc).getDirection();

    return nextdir.clockwiseAngleTo(prevdir)>M_PI;
}


bool EdgeLineSegment::isConnToNext(int idx) const
{
    if ( idx==size()-1 )
	return true;

    return nodes[idx].isNeighborTo(nodes[idx+1], surface.geometry.step(), true );
}


bool EdgeLineSegment::isConnToPrev(int idx) const
{
    if ( !idx )
	return true;

    return nodes[idx].isNeighborTo(nodes[idx-1], surface.geometry.step(), true );
}


void EdgeLineSegment::fillPar( IOPar& par ) const
{
    TypeSet<long long> subids;
    for ( int idx=0; idx<size(); idx++ )
    {
	subids += rc2longlong( nodes[idx] );
    }

    par.set( key, subids );
    par.set( classnamestr, className() );
}


bool EdgeLineSegment::usePar( const IOPar& par )
{
    TypeSet<long long> subids;
    if ( !par.get( key, subids ) )
	return false;

    nodes.erase();
    for ( int idx=0; idx<subids.size(); idx++ )
	nodes += longlong2rc(subids[idx]);

    if ( notifier ) notifier->trigger();
    return true;
}


NotifierAccess* EdgeLineSegment::changeNotifier()
{
    if ( !notifier )
    {
	notifier = new Notifier<EdgeLineSegment>(this);
	surface.poschnotifier.notify(mCB(this,EdgeLineSegment,posChangeCB));
    }
	
    return notifier;
}


bool EdgeLineSegment::trackWithCache( int start, bool forward,
		 const EdgeLineSegment* prev, const EdgeLineSegment* next)
{
    if ( !size() ) return false;
    const RowCol& lastnode = nodes[start];
    const RowCol& step = surface.geometry.step();
    RowCol backnode;
    if ( !getNeighborNode(start, !forward, backnode, prev, next ) )
    {
	if ( !getSurfaceStart( start, !forward, backnode ) )
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
	    if ( !dir.isNeighborTo(lastdefined) )
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

    if ( indexOf(newrc)!=-1 || (next&&next->indexOf(newrc)!=-1) )
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
    TypeSet<RowCol> nodescopy(nodes);
    for ( int idx=nodescopy.size()-1, idy=0; idx>=0; idx--, idy++ )
	nodes[idy] = nodescopy[idx];
    if ( notifier ) notifier->trigger();
}


void EdgeLineSegment::makeLine( const RowCol& start, const RowCol& stop )
{
    PtrMan<NotifyStopper> stopper = notifier ? new NotifyStopper(*notifier) : 0;
    RowCol::makeLine( start, stop, nodes, surface.geometry.step() );

    if ( notifier )
    {
	stopper->restore();
	notifier->trigger();
    }
}


RowCol EdgeLineSegment::first() const
{ return size() ? nodes[0] : RowCol(0,0); }


RowCol EdgeLineSegment::last() const
{ const int sz=size(); return sz ? nodes[sz-1] : RowCol(0,0); }


#define mReTrackReturn(retval) \
{ \
    if ( notifier ) \
    { \
	stopper->restore(); \
	if ( change ) notifier->trigger(); \
    } \
    return retval; \
}


bool EdgeLineSegment::isNodeOK( const RowCol& rc ) { return isAtEdge(rc); }

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
	    if ( isDefined(rc) && isNodeOK(rc) && !isByPassed(idy,prev,next) )
		break;

	    nodes.remove(idy--);
	    change = true;
	    backward = true;
	}

	if ( !size() )
	{
	    if ( !prev->size() ) mReTrackReturn(false);
	    nodes.insert( 0, prev->last());
	    change = true;
	    if ( !trackWithCache( 0, true, prev, next ) )
		mReTrackReturn(false);
	    nodes.remove(0);
	}

	if ( !prev->isContinuedBy(this) )
	    backward = true;

    }
    else
    {
	for ( int idy=0; idy<size(); idy++ )
	{
	    const RowCol& rc = (*this)[idy];
	    if ( isDefined(rc) && isNodeOK(rc) )
		break;

	    nodes.remove(idy--);
	    change = true;
	    backward = true;
	}

	if ( !size() )
	    mReTrackReturn(false);
    }

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
	    else if ( !isDefined(rc) )
		dotrack = true;
	    else if ( !isNodeOK(rc) )
		dotrack = true;
	    else if ( isByPassed(idx, prev, next ) )
		dotrack = true;
	    else
	    {
		RowCol nextrc;
		if ( !getNeighborNode( idx, true, nextrc, prev, next ) )
		    dotrack = true;
		else
		{
		    RowCol prevrc;
		    if ( !getNeighborNode( idx, false, prevrc, prev, next ) )
			dotrack = true;
		    else
		    {
			const TypeSet<RowCol>& dirs=RowCol::clockWiseSequence();
			const RowCol& cur = nodes[idx];
			int prevdir = dirs.indexOf((prevrc-cur).getDirection());
			int nextdir = dirs.indexOf((nextrc-cur).getDirection());

			int curdir = (prevdir+1)%dirs.size();
			while ( curdir!=nextdir )
			{
			    const RowCol nrc = cur+dirs[curdir]*surface.geometry.step();
			    if ( isDefined(nrc) )
			    { dotrack=true; break; }
			    if ( ++curdir>=dirs.size() ) curdir = 0;
			}
		    }
		}
	    }
	}

	if ( dotrack )
	{
	    if ( idx!=size()-1 )
	    {
		nodes.remove( idx+1, size()-1 );
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
		if ( prev->isContinuedBy(this) )
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
	if ( idx<size()-1 ) rc = nodes[idx+1];
	else if ( next ) rc = (*next)[0];
	else return false;
    }
    else 
    {
	if ( idx ) rc = nodes[idx-1];
	else if ( prev ) rc = (*prev)[prev->size()-1];
	else return false;
    }

    return nodes[idx].isNeighborTo(rc, surface.geometry.step(), true );
}


bool EdgeLineSegment::getSurfaceStart( int start, bool clockwise, 
					   RowCol& rc ) const
{
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();

    const RowCol& step = surface.geometry.step();
    const RowCol& centernode = nodes[start];

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
    const RowCol& step = surface.geometry.step();
    if ( forward )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    if ( rc.isNeighborTo(nodes[idx], step, true ) )
		return idx;
	}
    }
    else
    {
	for ( int idx=size()-1; idx>=0; idx-- )
	{
	    if ( rc.isNeighborTo(nodes[idx], step, true ) )
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
       						   Surface&	surf,
						   const SectionID& sect )
{
    BufferString name;
    if ( !par.get( classnamestr, name ) )
	return 0;

    for ( int idx=0; idx<factories().size(); idx++ )
    {
	if ( strcmp( factories()[idx]->name, name ) )
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
     mCBCapsuleUnpack( PosID, pid, cb );
     if ( pid.sectionID()!=section ) return;
     
     const RowCol rc = surface.geometry.subID2RowCol(pid.subID());
     const int nodeidx = indexOf(rc);
     if ( nodeidx==-1 ) return;

     if ( !isDefined(rc) )
	 nodes.remove(nodeidx);

     notifier->trigger();
}


EdgeLine::EdgeLine( EM::Surface& surf, const EM::SectionID& sect )
    : surface( surf )
    , section( sect )
    , changenotifier(this)
{}


EdgeLine* EM::EdgeLine::clone() const
{
    EdgeLine* res = new EdgeLine( surface, section );
    for ( int idx=0; idx<segments.size(); idx++ )
    {
	EdgeLineSegment* seg = segments[idx]->clone();
	seg->changeNotifier()->notify( mCB(res,EdgeLine,sectionChangeCB) );
	res->segments += seg;
    }
    
    return res;
}


void EdgeLine::setSection( const EM::SectionID& s )
{
    section = s;
    for ( int idx=0; idx<segments.size(); idx++ )
	segments[idx]->setSection(s);
}


int EdgeLine::getSegment( const EM::PosID& pos, int* seq ) const
{
    if ( pos.objectID()!=surface.id() || pos.sectionID()!=section )
	return -1;

    return getSegment( surface.geometry.subID2RowCol(pos.subID()), seq );
}


int EdgeLine::getSegment( const RowCol& rowcol, int* seq ) const 
{
    for ( int idx=0; idx<segments.size(); idx++ )
    {
	const int idy = segments[idx]->indexOf( rowcol );
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

	return first.isNeighborTo((*segments[idx])[segsize-1], surface.geometry.step(),
				  true );
    }

    return false;
}


bool EdgeLine::isInside( const EM::PosID& pid, bool undefval ) const
{
    if ( pid.objectID()!=surface.id() || pid.sectionID()!=section )
	return undefval;

    return isInside( surface.geometry.subID2RowCol(pid.subID()), undefval );
}


bool EdgeLine::isInside( const RowCol& rc, bool undefval ) const
{
    if ( !isClosed() )
	return undefval;

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
	    if ( !distsquare ) return true;
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

    const RowCol& step = surface.geometry.step();

    TypeSet<RowCol> rcs;
    RowCol prevrc;
    for ( int segment=0; segment<segments.size(); segment++ )
    {
	for ( int idx=0; idx<segments[segment]->size(); idx++ )
	{
	    const RowCol rc = (*segments[segment])[idx];
	    if ( (!idx && !segment) ||
		    rcs[rcs.size()-1].isNeighborTo(prevrc,step,true ) )
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
	    if ( (!idx && !segment) ||
		    rcs[rcs.size()-1].isNeighborTo(prevrc,step,true ) )
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




int EdgeLine::computeArea() const
{
    if ( !isClosed() )
	return -1;

    TypeSet<RowCol> nodesinside;
    for ( int idx=0; idx<segments.size(); idx++ )
	for ( int idy=0; idy<segments[idx]->size(); idy++ )
	    nodesinside += (*segments[idx])[idy];

    int layer2start = nodesinside.size();
    int layer1start = 0;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const RowCol& step = surface.geometry.step();
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
	int startidx;
	int startseg = getSegment( ns[0]->first(), &startidx );
	if ( startseg==-1 )
	{
	    deepErase( ns );
	    return;
	}

	int stopidx;
	int stopseg = getSegment( ns[ns.size()-1]->last(), &stopidx );
	if ( stopseg==-1 )
	{
	    deepErase( ns );
	    return;
	}

	if ( startseg==stopseg && startidx>stopidx )
	{

	    while ( startseg ) 
	    {
		delete segments[0];
		segments.remove(0);
		startseg--;
	    }

	    while ( segments.size()>1 )
	    {
		delete segments[1];
		segments.remove(1);
	    }

	    stopseg = 0;
	}

	int curseg = startseg;
	int curidx = startidx;

	while ( true )
	{
	    if ( curseg!=stopseg )
	    {
		const int nrseg = segments.size();
		segments[curseg]->remove( curidx, segments[curseg]->size()-1 );
		if ( nrseg!=segments.size() )
		{
		    if ( stopseg>curseg ) stopseg--;
		    if ( startseg>curseg ) startseg--;
		    curseg--;
		}
	    }
	    else
	    {
		if ( startidx>stopidx )
		{
		    segments[curseg]->remove(startidx,segments[curseg]->size());
		    segments[curseg]->remove(0,stopidx);



		}
		else if ( nrSegments()==1 )
		{
		    for ( int idy=segments[curseg]->size()-1; idy>stopidx;idy--)
		    {
			segments[curseg]->insert(0,(*segments[curseg])[idy]);
			stopidx++;
			startidx++;
			idy++;
		    }

		    segments[curseg]->remove(startidx,
			    		     segments[curseg]->size());
		}
		else
		{
		    EdgeLineSegment* nels = segments[curseg]->clone();
		    segments[curseg]->remove(startidx,segments[curseg]->size());
		    nels->remove(0,stopidx);
		    segments.insertAt(nels,curseg+1);
		}
		break;
	    }

	    if ( ++curseg>=segments.size() )
		curseg = 0;
	    curidx = 0;
	}

	for ( int idy=0; idy<nrSegments(); idy++ )
	{
	    if ( segments[idy]->size() ) continue;

	    delete segments[idy];
	    segments.remove(idy);
	    if ( idy<=startseg ) startseg--;
	    idy--;
	}

	idx = startseg+1;
    }

    for ( int idy=0; idy<ns.size(); idy++ )
    {
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


void EdgeLine::fillPar( IOPar& par ) const
{
    par.set( nrsegmentsstr, nrSegments() );
    for ( int idx=0; idx<nrSegments(); idx++ )
    {
	IOPar segmentpar;
	segments[idx]->fillPar( segmentpar );

	BufferString key = segmentprefixstr;
	key += idx;

	par.mergeComp( segmentpar, key );
    }
}


bool EdgeLine::usePar( const IOPar& par )
{
    deepErase( segments );
    int nrsegments = 0;
    par.get( nrsegmentsstr, nrsegments );
    for ( int idx=0; idx<nrsegments; idx++ )
    {
	BufferString key = segmentprefixstr;
	key += idx;

	PtrMan<IOPar> segmentpar = par.subselect( key );
	if ( !segmentpar ) 
	    continue;

	EdgeLineSegment* els = EM::EdgeLineSegment::factory( *segmentpar,
				    surface, section );
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
    if ( !segment->size() )
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

    const RowCol& step = surface.geometry.step();

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
	     curseg.last().isNeighborTo(nextseg.first(), surface.geometry.step() ) &&
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


PosID EM::EdgeLineIterator::current() const
{
    const Surface& surface = el.getSurface();
    return PosID( surface.id(), el.getSection(),
			surface.geometry.rowCol2SubID(currentRowCol()));
}


EdgeLineSet::EdgeLineSet(EM::Surface& surf, const EM::SectionID& sect)
    : surface(surf), section(sect), changenotifier(this)
{}


EdgeLineSet::~EdgeLineSet()
{
    removeAll();
}



EdgeLineSet* EM::EdgeLineSet::clone() const
{
    EdgeLineSet* res = new EdgeLineSet( surface, section );
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

    StepInterval<int> rowrange;
    StepInterval<int> colrange;
    surface.geometry.getRange( section, rowrange, true );
    if ( rowrange.width() )
	surface.geometry.getRange( section, colrange, false );

    
    for ( int row=rowrange.start; row<=rowrange.stop; row+=rowrange.step )
    {
	for ( int col=colrange.start; col<=colrange.stop; col+=colrange.step )
	{
	    const RowCol rc(row,col);
	    if ( !surface.geometry.isDefined(section,rc) )
		continue;

	    const PosID pid(surface.id(),section,surface.geometry.rowCol2SubID(rc));

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

	    surface.unSetPos(pid,true);
	}
    }

    return true;
}


bool EdgeLineSet::findLines( EdgeLineCreationFunc func )
{
    removeAll();

    StepInterval<int> rowrange;
    surface.geometry.getRange( rowrange, true );
    if ( rowrange.width()<2 )
	return false;

    StepInterval<int> colrange;
    surface.geometry.getRange( colrange, false );
    if ( colrange.width()<2 )
	return false;

    for ( int row=rowrange.start; row<=rowrange.stop; row+=rowrange.step )
    {
	for ( int col=colrange.start; col<=colrange.stop;col+=colrange.step)
	{
	    const RowCol rc( row, col );
	    const PosID pid(surface.id(),section,surface.geometry.rowCol2SubID(rc));

	    TypeSet<PosID> linkedpos;
	    surface.geometry.getLinkedPos( pid, linkedpos );
	    if ( linkedpos.size() )
	    {
		//Todo
		continue;
	    }

	    if ( surface.geometry.isAtEdge(pid) )
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

		EdgeLineSegment* els = func( surface, section );
		(*els) += rc;
		int idx=0;
		while ( els->track(idx++,true) )
		    ;

		if ( els->isClosed() )
		{
		    EdgeLine* el = new EdgeLine( surface, section );
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
    par.set( nrlinesstr, nrLines() );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BufferString key = lineprefixstr;
	key += idx;

	IOPar linepar;
	lines[idx]->fillPar( linepar );
	par.mergeComp( linepar, key );
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
    par.get( nrlinesstr, nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	BufferString key = lineprefixstr;
	key += idx;

	PtrMan<IOPar> linepar = par.subselect(key);
	if ( !linepar ) continue;
	EdgeLine* el = new EdgeLine( surface, section );
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


EdgeLineManager::EdgeLineManager( EM::Surface& surf )
    : surface(surf)
    , addremovenotify( this )
{ linesets.allowNull(); }


EdgeLineManager::~EdgeLineManager()
{ removeAll(); }


const EdgeLineSet* EM::EdgeLineManager::getEdgeLineSet(
	const SectionID& section ) const
{
    return const_cast<EdgeLineManager*>(this)->
					getEdgeLineSet( section, false );
}


EdgeLineSet* EM::EdgeLineManager::getEdgeLineSet(
	const SectionID& section, bool create )
{
    const int sectionnr = surface.geometry.sectionNr(section);
    if ( sectionnr==-1 )
	return 0;

    while ( sectionnr>=linesets.size() ) linesets += 0;

    if ( !linesets[sectionnr] && create )
    {
	EdgeLineSet* els = new EdgeLineSet( surface, section );
	if ( !els->findLines() )
	    delete els;
	else
	{
	    linesets.replace( els, sectionnr );
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

    const int dstidx = surface.geometry.sectionNr(dst);
    while ( dstidx>=linesets.size() ) linesets += 0;
    linesets.replace(els, dstidx);
    addremovenotify.trigger( dst );
}


void EdgeLineManager::removeSection( const SectionID& pid )
{
    const int nr = surface.geometry.sectionNr(pid);
    if ( nr<0 || nr>=linesets.size() ) return;
    delete linesets[nr];
    linesets.remove(nr);
    addremovenotify.trigger( pid );
}


void EdgeLineManager::removeLineSet( const SectionID& pid )
{
    const int nr = surface.geometry.sectionNr(pid);
    if ( nr<0 || nr>=linesets.size() ) return;
    delete linesets[nr];
    linesets.replace(0,nr);
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
    for ( int idx=0; idx<surface.geometry.nrSections(); idx++ )
    {
	const EdgeLineSet* els = getEdgeLineSet(surface.geometry.sectionID(idx) );
	if ( !els ) continue;

	IOPar elspar;
	els->fillPar( elspar );
	BufferString key = sectionkey;
	key += surface.geometry.sectionID(idx);
	par.mergeComp( elspar, key );
    }
}


bool EdgeLineManager::usePar( const IOPar& par )
{
    removeAll();
    for ( int idx=0; idx<surface.geometry.nrSections(); idx++ )
    {
	BufferString key = sectionkey;
	key += surface.geometry.sectionID(idx);

	PtrMan<IOPar> elspar = par.subselect(key);
	if ( !elspar ) continue;

	EdgeLineSet* els = new EdgeLineSet( surface, surface.geometry.sectionID(idx) );
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
