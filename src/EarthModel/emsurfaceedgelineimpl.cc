/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfaceedgelineimpl.cc,v 1.2 2004-09-07 06:20:49 kristofer Exp $";



#include "emsurfaceedgelineimpl.h"

#include "emmanager.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfacerelations.h"
#include "executor.h"
#include "iopar.h"
#include "mathfunc.h"
#include "ranges.h"
#include "sorting.h"
#include "survinfo.h"
#include "errh.h"

#include <math.h>


namespace EM 
{

mEdgeLineSegmentFactoryEntry( TerminationEdgeLineSegment );
mEdgeLineSegmentFactoryEntry( SurfaceConnectLine );
mEdgeLineSegmentFactoryEntry( SurfaceCutLine );

const char* SurfaceConnectLine::connectingsectionstr = "Connecting Segment";
const char* SurfaceCutLine::cuttingobjectstr = "Cutting Surface";
const char* SurfaceCutLine::possidestr = "Cut Positive Side";

bool SurfaceConnectLine::internalIdenticalSettings(
					const EdgeLineSegment& els ) const
{
    return
	reinterpret_cast<const SurfaceConnectLine*>(&els)->connectingsection ==
			connectingsection &&
			EdgeLineSegment::internalIdenticalSettings(els);
}


bool SurfaceConnectLine::isNodeOK(const RowCol& rc)
{
    return EdgeLineSegment::isNodeOK(rc) &&
	   surface.geometry.isDefined(connectingsection,rc);
}


void SurfaceConnectLine::fillPar(IOPar& par) const
{
    EdgeLineSegment::fillPar(par);
    par.set(connectingsectionstr,connectingsection);
}


bool SurfaceConnectLine::usePar(const IOPar& par)
{
    int dummy;
    const bool res = EdgeLineSegment::usePar(par) &&
		     par.get(connectingsectionstr,dummy);
    if ( res ) connectingsection = dummy;
    return res;
}


SurfaceCutLine::SurfaceCutLine( Surface& surf, const SectionID& sect )
    : EdgeLineSegment( surf, sect )
    , t2d( 0 )
    , cuttingsurface( 0 )
    , cutonpositiveside( false )
    , meshdist( getMeshDist() )
{}


bool SurfaceCutLine::shouldTrack(int idx) const
{ return !idx || idx==size()-1; }



float SurfaceCutLine::getMeshDist()
{
    const SurveyInfo& survinfo( SI() );
    const BinID step( survinfo.inlStep(true), survinfo.crlStep(true));
    return survinfo.transform(BinID(0,0)).distance(survinfo.transform(step));
}


bool SurfaceCutLine::isNodeOK(const RowCol& testrc)
{
    const int cacheidx = cacherc.indexOf(testrc,false);

    float disttosurface = mUndefValue;
    if ( cacheidx==-1 )
    {
	const Coord3 ntpos = surface.geometry.getPos( section, testrc );
	if ( ntpos.isDefined() )
	{
	    disttosurface = cuttingsurface->geometry.normalDistance(ntpos,t2d);
	    if ( cutonpositiveside ) disttosurface = -disttosurface;
	}
	distcache += disttosurface;
	cacherc+=testrc;
	poscache += ntpos;
	ischanged += false;
    }
    else
	disttosurface = distcache[cacheidx];

    if ( mIsUndefined(disttosurface) )
	return false;

    return fabs(disttosurface)<meshdist/2;
}


bool SurfaceCutLine::trackWithCache( int start, bool forward, const
			    EdgeLineSegment* prev, const EdgeLineSegment* next)
{
    if ( !cuttingsurface ) 
	return EdgeLineSegment::track( start, forward, prev, next );

    if ( size()>1 && isAtCuttingEdge(start) )
	return false;

    if ( (forward && start!=size()-1) || (!forward && start ) )
    {
	pErrMsg( "Tracking non-edge" );
	return false;
    }

    const RowCol& rc = (*this)[start];

    RowCol backnode;
    if ( !getNeighborNode(start, !forward, backnode, prev, next ) )
    {
	if ( !getSurfaceStart( start, !forward, backnode ) )
	    return false;
    }

    const RowCol backnodedir = (backnode-rc).getDirection();
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();
    int curdiridx = dirs.indexOf(backnodedir) + nrdirs;

    const RowCol& step = surface.geometry.step();

    RowCol lastdefineddir;
    bool firstfound = false;
    bool rightsidefound = false;
    int bestidx = -1;
    bool changebestposition = false;
    Coord3 bestpos;
    float bestscore = mUndefValue;
    for ( int idx=1; idx<nrdirs; idx++ )
    {
	//Ignore the first, since that is the backnode
	if ( forward ) curdiridx--;
	else curdiridx++;

	const RowCol& curdir = dirs[curdiridx%nrdirs];
	if ( firstfound && !curdir.isNeighborTo(lastdefineddir,RowCol(1,1),true ) )
	    break;

	const RowCol currc = curdir*step+rc;
	const int cacheidx = cacherc.indexOf(currc,false);
	float distance;
	if ( cacheidx==-1 )
	{
	    const Coord3 pos = surface.geometry.getPos( section, currc );
	    poscache += pos;
	    if (  pos.isDefined() )
	    {
		distance = cuttingsurface->geometry.normalDistance(pos,t2d);
		if ( cutonpositiveside ) distance = -distance;
	    }
	    else
		distance = mUndefValue;

	    distcache += distance;
	    cacherc += currc;
	    ischanged += false;
	}
	else
	    distance = distcache[cacheidx];

	if ( !mIsUndefined(distance) )
	{
	    firstfound = true;
	    const RowCol prevdir = lastdefineddir;
	    lastdefineddir = curdir;

	    const bool zerodist = fabs(distance)<meshdist/16;
	    if ( !zerodist && distance>0 )
		rightsidefound = true;

	    if ( !rightsidefound )
		continue;

	    /* Normally, one node has to be on the 'wrong' part of the fault.
	     * But if we are at the last node, we might make something out of
	     * it anyway.
	    */

	    if ( idx==nrdirs-1 && bestidx==-1 )
	    {
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos );
		if ( curscore<meshdist/2 && curscore<=bestscore )
		{
		    bestidx = curdiridx;
		    changebestposition = changecurbestpos;
		    bestpos = curbestpos;
		    bestscore = curscore;
		}

		break;
	    }

	    /*If we are really close, check if we can get a match */

	    if ( zerodist )
	    {
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos );
		if ( curscore<meshdist/16 && curscore<=bestscore  )
		{
		    bestidx = curdiridx;
		    changebestposition = changecurbestpos;
		    bestpos = curbestpos;
		    bestscore = curscore;
		    break;
		}

		continue;
	    }

	    if ( distance<0 )
	    {
		if ( bestidx!=-1 )
		    break;

		const RowCol prevrc = prevdir*step+rc;
		Coord3 prevbestpos;
		bool changeprevbestpos;
		const float prevscore =
		    computeScore( prevrc, changeprevbestpos, prevbestpos );

		if ( !changeprevbestpos )
		{
		    changebestposition = false;
		    bestidx = dirs.indexOf(prevdir);
		    break;
		}
		
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos );

		const bool usecur = curscore<prevscore;
		changebestposition = usecur
		    		? changecurbestpos : changeprevbestpos;
		bestpos = usecur ? curbestpos : prevbestpos;
		bestidx = usecur ? curdiridx : dirs.indexOf(prevdir);
		bestscore = usecur ? curscore : prevscore;
		break;
	    }
	}
	else if ( rightsidefound )
	{
	    if ( !curdir.isNeighborTo(lastdefineddir,RowCol(1,1),true) )
		break;
	}
    }

    if ( bestidx==-1 )
    {
	if ( rightsidefound )
	{
	    const RowCol prevrc = lastdefineddir*step+rc;
	    Coord3 prevbestpos;
	    bool changeprevbestpos;
	    const float prevscore =
		computeScore( prevrc, changeprevbestpos, prevbestpos );

	    if ( prevscore<meshdist/2 )
	    {
		changebestposition = false;
		bestidx = dirs.indexOf(lastdefineddir);
	    }
	}

	return false;
    }

    const RowCol newrc = rc+dirs[bestidx%nrdirs]*step;
    if ( forward )
	(*this) += newrc;
    else insert( 0, newrc );

    if ( changebestposition )
    {
	const int cacheindex = cacherc.indexOf(newrc);
	float distance = cuttingsurface->geometry.normalDistance(bestpos,t2d);
	if ( cutonpositiveside ) distance = -distance;
	if ( cacheindex!=-1 )
	{
	    poscache[cacheindex] = bestpos;
	    ischanged[cacheindex] = true;
	    distcache[cacheindex] = distance;
	}
	else
	{
	    cacherc += newrc;
	    poscache += bestpos;
	    ischanged += true;
	    distcache += distance;
	}
    }

    return true;
}


bool SurfaceCutLine::isAtCuttingEdge(int idx) const
{
    const Coord3 ntpos = surface.geometry.getPos(section,(*this)[idx]);

    const Interval<float> xinterval( ntpos.x-10, ntpos.x+10 );
    const Interval<float> yinterval( ntpos.y-10, ntpos.y+10 );
    const StepInterval<float>& zrange = SI().zRange();
    const Interval<float> zinterval( zrange.start, zrange.stop );

    TypeSet<PosID> cuttingnodes;
    cuttingsurface->geometry.findPos( xinterval,yinterval,zinterval,
				      &cuttingnodes );
    if ( !cuttingnodes.size() )
	return false;
    
    int closestaboveidx = -1, closestbelowidx = -1;
    float closestabove, closestbelow;
    for ( int idy=0; idy<cuttingnodes.size(); idy++ )
    {
	const Coord3 cuttingpos = cuttingsurface->getPos(cuttingnodes[idy]);
	const float diff =  cuttingpos.z - ntpos.z;
	if ( fabs(diff)<zrange.step/4 )
	    return cuttingsurface->geometry.isAtEdge(cuttingnodes[idy]);

	if ( diff>0 && ( closestaboveidx==-1  || diff<closestabove ))
	{ closestabove = diff; closestaboveidx=idy; }
	if ( diff<0 && ( closestbelowidx==-1  || diff<closestbelow ))
	{ closestbelow = diff; closestbelowidx=idy; }
    }

    if ( closestaboveidx!=-1 && closestbelowidx!=-1 )
    {
	return
	    cuttingsurface->geometry.isAtEdge(cuttingnodes[closestaboveidx]) &&
	    cuttingsurface->geometry.isAtEdge(cuttingnodes[closestbelowidx]);
    }
    else if ( closestaboveidx!=-1 )
	return cuttingsurface->geometry.isAtEdge(cuttingnodes[closestaboveidx]);
    else if ( closestbelowidx!=-1 )
	return cuttingsurface->geometry.isAtEdge(cuttingnodes[closestbelowidx]);

    return false;
}


void SurfaceCutLine::removeCache()
{
    cacherc.erase();
    poscache.erase();
    distcache.erase();
    ischanged.erase();
}



void SurfaceCutLine::commitChanges()
{
    for ( int idx=0; idx<ischanged.size(); idx++ )
    {
	if ( ischanged[idx] )
	{
	    surface.geometry.setPos( section, cacherc[idx],
		    		     poscache[idx], false, true );
	    ischanged[idx] = false;
	}
    }
}

#define mSetupCreateCut(createline) \
    const SectionRelations* relations = \
				surface.relations.getRelations(section); \
    if ( !relations || relidx>=relations->cuttingsurfaces.size() ) \
	return 0; \
 \
    mDynamicCastGet(Surface*,cuttingsurface, \
	    	    EMM().getObject(relations->cuttingsurfaces[relidx])); \
    const SectionID& cuttingsection = relations->cuttingsections[relidx]; \
    const bool cutonposside = relations->cutpositiveside[relidx]; \
 \
    if ( !cuttingsurface || \
	 !surface.geometry.hasSection(section) || \
	 !cuttingsurface->geometry.hasSection(cuttingsection) ) \
	return 0; \
 \
    EdgeLineSet& elset = \
	    *surface.edgelinesets.getEdgeLineSet(section,createline); \
    if ( !&elset ) return 0

SurfaceCutLine* SurfaceCutLine::createCutFromSeed( Surface& surface,
    const SectionID& section, int relidx, const RowCol& seed,
    bool boothdirs, const MathFunction<float>* t2d )
{
    mSetupCreateCut(false);

    //Keep this?
    //if ( !elset.isOnLine(seed,&lineidx,&segmentidx,&segmentpos) )
	//return 0;

    SurfaceCutLine* scl =
	    new SurfaceCutLine( surface, section );
    scl->setCuttingSurface( cuttingsurface, cutonposside );
    scl->setTime2Depth( t2d );
    (*scl) += seed;
    if ( !scl->isNodeOK(seed) )
    {
	delete scl; return 0;
    }
	
    while ( scl->trackWithCache( scl->size()-1, true, 0, 0 ) )
    {
	const RowCol& newtracked = scl->last();
	if ( scl->indexOf( newtracked, true )!=scl->size() )
	{
	    delete scl; scl = 0; break;
	}
    }

    if ( !scl ) return 0;
    if ( scl->size()>1 )
	scl->remove(0);

    while ( scl->trackWithCache( 0, false, 0, 0 ) )
    {
	const RowCol& newtracked = scl->first();
	if ( scl->indexOf( newtracked, false )!=0 )
	{
	    delete scl; scl = 0; break;
	}
    }

    return scl;
}


SurfaceCutLine* SurfaceCutLine::createCutFromEdges( Surface& surface,
   const SectionID& section, int relidx, const MathFunction<float>* t2d )
{
    mSetupCreateCut(true);

    int lineidx = elset.getMainLine();
    EdgeLine& el = *elset.getLine( lineidx );
    if ( !&el ) return 0;

    const float meshdist = getMeshDist();

    TypeSet<RowCol> border;
    TypeSet<float> distances;
    computeDistancesAlongLine( el, *cuttingsurface, t2d, border, distances,
	    			cutonposside, true );
    TypeSet<RowCol> forwardtrackstarts;
    TypeSet<RowCol> backwardtrackstarts;
    int decreasing = border.size()-2;
    int furthestoncuttingside = 0;
    for ( int increasing=0; increasing<border.size(); increasing++ )
    {
	if ( distances[increasing]<distances[furthestoncuttingside] )
	    furthestoncuttingside = increasing;

	if ( !increasing )
	    continue;

	const float inccurdist = distances[increasing];
	const float incprevdist = distances[increasing-1];
	if ( inccurdist<=0 && incprevdist>0 )
	{
	    if ( incprevdist<meshdist/2 )
		forwardtrackstarts += border[increasing-1];
	    if ( inccurdist>-meshdist/2 )
		forwardtrackstarts += border[increasing];
	}

	float deccurdist = distances[decreasing];
	float decprevdist = distances[decreasing+1];
	if ( deccurdist<=0 && decprevdist>0 )
	{
	    if ( decprevdist<meshdist/2 )
		backwardtrackstarts += border[decreasing-2];
	    if ( deccurdist>-meshdist/2 )
		backwardtrackstarts += border[decreasing];
	}

	decreasing--;
    }

    TypeSet<RowCol> possibletrackstarts;
    BoolTypeSet trackforward;

    while ( forwardtrackstarts.size() || backwardtrackstarts.size() )
    {
	if ( forwardtrackstarts.size() )
	{
	    possibletrackstarts += forwardtrackstarts[0];
	    trackforward += true;
	    forwardtrackstarts.remove(0);
	}
	if ( backwardtrackstarts.size() )
	{
	    possibletrackstarts += backwardtrackstarts[0];
	    trackforward += false;
	    backwardtrackstarts.remove(0);
	}

    }

    SurfaceCutLine* cutline = 0;
    for ( int idx=0; idx<possibletrackstarts.size(); idx++ )
    {
	const bool forward = trackforward[idx];
	const int firstborderidx = border.indexOf(possibletrackstarts[idx]);
	SurfaceCutLine* scl = new SurfaceCutLine( surface, section );
	scl->setCuttingSurface( cuttingsurface, cutonposside );
	scl->setTime2Depth( t2d );
	(*scl) += possibletrackstarts[idx];
	while ( scl->trackWithCache(forward ? scl->size()-1 : 0, forward,0,0))
	{
	    const int newidx = forward ? scl->size()-1 : 0;
	    const RowCol& newtracked = (*scl)[newidx];
	    if ( scl->size()==2 )
	    {
		const int otherstart = possibletrackstarts.indexOf(newtracked);
		if ( otherstart!=-1 && trackforward[otherstart]==forward )
		{
		    delete scl; scl = 0; break;
		}
	    }

	    //Check if it's allready on border
	    const int lastborderidx = border.indexOf( newtracked );
	    if ( lastborderidx==-1 )
		continue;

	    //Check if previous was on border if it was, we are
	    //following a border and should not co additional checks
	    const RowCol& prevrc = (*scl)[forward?newidx-1:1];
	    const int prevborderidx = border.indexOf( prevrc );

	    if ( prevborderidx!=-1 &&
		 (abs(prevborderidx-lastborderidx)==1 ||
		 (!prevborderidx&&lastborderidx==border.size()-1 ) ||
		 (prevborderidx==border.size()-1&&!lastborderidx) ))
		continue;

	    if ( lastborderidx>furthestoncuttingside ==
		 firstborderidx>furthestoncuttingside )
		break;
	}

	if ( !scl ) continue;

	if ( scl->size()==1 )
	{
	    delete scl;
	    continue;
	}

	scl->remove( forward ? 0 : scl->size()-1 );
	//Track to starting node	
	if ( !scl->trackWithCache(forward ? 0 : scl->size()-1,
				  !forward,0,0) ||
	     (forward?scl->first():scl->last())!=possibletrackstarts[idx] )
	{
	    //Continue if it didn't retrack to old trackstart
	    delete scl; continue;
	}

	//Check if it connects with line, if not, try next trackstart
	const int lastborderidx =
		border.indexOf( forward?scl->last():scl->first());
	if ( lastborderidx==-1 )
	{
	    if ( !cutline || (cutline && scl->size()>cutline->size()) )
	    {
		delete cutline;
		cutline=scl;
	    }
	    else delete scl;
	}
	else
	{
	    if ( lastborderidx>furthestoncuttingside ==
		 firstborderidx>furthestoncuttingside )
	    {
		//It has bumbped in to the edge close to the start
		delete scl;
		continue;
	    }

	    delete cutline; cutline = scl;
	    break;		//Complete line found
	}
    }

    return cutline;
}



void SurfaceCutLine::computeDistancesAlongLine( const EdgeLine& line,
	const Surface& cuttingsurface, const  MathFunction<float>* t2d,
	TypeSet<RowCol>& border, TypeSet<float>& distances, bool negate,
	bool usecaching )
{
    const Surface& surface = line.getSurface();
    const SectionID section = line.getSection();
    int furthestaway = -1;

    const float meshdist = getMeshDist();
    EdgeLineIterator iterator(line, true );
    if ( !iterator.isOK() )
	return;

    int nodestonextcalc = 0;
    float dist;
    do
    {
	const RowCol& rc = iterator.currentRowCol();
	if ( !usecaching || !nodestonextcalc )
	{
	    const Coord3 coord = surface.geometry.getPos(section,rc );
	    dist = cuttingsurface.geometry.normalDistance( coord, t2d );
	    if ( negate ) dist = -dist;
	}

	if ( !mIsUndefined(dist) )
	{
	    if ( furthestaway==-1 || distances[furthestaway]<dist )
		furthestaway = border.size();

	    border += rc;
	    distances += dist;

	    if ( !nodestonextcalc )
	    {
		nodestonextcalc = mNINT(fabs(dist)/meshdist)-2;
		if ( nodestonextcalc<0 ) nodestonextcalc=0;
	    }
	    else 
		nodestonextcalc--;
	}
    } while ( iterator.next() );

    if ( furthestaway>0 )
    {
	for ( int idx=0; idx<furthestaway; idx++ )
	{ border += border[idx]; distances += distances[idx]; }
	border.remove(0,furthestaway-1);
	distances.remove(0,furthestaway-1);
    }
}


bool SurfaceCutLine::internalIdenticalSettings(
					const EdgeLineSegment& els_ ) const
{
    const SurfaceCutLine* els = reinterpret_cast<const SurfaceCutLine*>(&els_);
    return els->cuttingsurface==cuttingsurface &&
	   els->cutonpositiveside==cutonpositiveside &&
	   EdgeLineSegment::internalIdenticalSettings(els_);
}


float SurfaceCutLine::computeScore( const RowCol& rc, bool& changescorepos,
				    Coord3& scorepos )
{
    const int cacheidx = cacherc.indexOf(rc,false);
    float distance;
    Coord3 pos;
    if ( cacheidx==-1 )
    {
	poscache += pos = surface.geometry.getPos( section, rc );
	if (  pos.isDefined() )
	{
	    distance = cuttingsurface->geometry.normalDistance(pos,t2d);
	    if ( cutonpositiveside ) distance = -distance;
	}
	else
	    distance = mUndefValue;

	distcache += distance;
	cacherc += rc;
	ischanged += false;
    }
    else
    {
	distance = distcache[cacheidx];
	pos = poscache[cacheidx];
    }

    if ( !pos.isDefined() )
	return mUndefValue;

    const Interval<float> xinterval( pos.x-10, pos.x+10 );
    const Interval<float> yinterval( pos.y-10, pos.y+10 );
    const StepInterval<float>& zrange = SI().zRange();
    const Interval<float> zinterval( zrange.start, zrange.stop );

    TypeSet<PosID> cuttingnodes;
    cuttingsurface->geometry.findPos( xinterval,yinterval,zinterval,
				      &cuttingnodes );

    if ( !cuttingnodes.size() )
	return mUndefValue;

    scorepos = pos;

    float closestabove, closestbelow;
    int closestaboveidx=-1, closestbelowidx=-1;
    if ( cuttingnodes.size() )
    {
	for ( int idy=0; idy<cuttingnodes.size(); idy++ )
	{
	    const Coord3 cuttingpos = cuttingsurface->getPos(cuttingnodes[idy]);
	    if ( cuttingpos.z>=pos.z )
	    {
		if ( closestaboveidx==-1 || cuttingpos.z<closestabove )
		{
		    closestaboveidx = idy;
		    closestabove = cuttingpos.z;
		}
	    }
	    else
	    {
		if ( closestbelowidx==-1 || cuttingpos.z>closestbelow )
		{
		    closestbelowidx = idy;
		    closestbelow = cuttingpos.z;
		}
	    }
	}

	if ( closestaboveidx!=-1 && closestbelowidx!=-1 )
	{
	    const RowCol closestaboverc = SurfaceGeometry::subID2RowCol(
		    cuttingnodes[closestaboveidx].subID());
	    const RowCol closestbelowrc = SurfaceGeometry::subID2RowCol(
		    cuttingnodes[closestbelowidx].subID());
	    if ( !closestaboverc.isNeighborTo(closestbelowrc,
					      cuttingsurface->geometry.step()) )
	    {
		if ( closestabove-pos.z<pos.z-closestbelow )
		    scorepos.z = closestabove;
		else
		    scorepos.z = closestbelow;

		changescorepos = true;
	    }
	    else
	    {
		changescorepos = false;
		return 0;
	    }
	}
	else if ( closestaboveidx==-1 )
	{
	    scorepos.z = closestbelow;
	    changescorepos = true;
	}
	else if ( closestbelowidx==-1 )
	{
	    scorepos.z = closestabove;
	    changescorepos = true;
	}
    }
    else
    {
	const Coord3 cuttingpos = cuttingsurface->getPos(cuttingnodes[0]);
	scorepos.z = cuttingpos.z;
	changescorepos = true;
    }

    float score = fabs(distance);

    if ( changescorepos )
    {
	const Coord3 scorenpos( scorepos, t2d ?
		t2d->getValue(scorepos.z) : scorepos.z);

	const Coord3 npos( pos, t2d ? t2d->getValue(pos.z):pos.z);
	score +=  fabs( scorenpos.z-npos.z );
    }

    return score;
}

void SurfaceCutLine::fillPar(IOPar& par) const
{
    EdgeLineSegment::fillPar(par);
    par.set(cuttingobjectstr,cuttingsurface->multiID());
    par.setYN(possidestr,cutonpositiveside);
}


bool SurfaceCutLine::usePar(const IOPar& par)
{
    if ( !EdgeLineSegment::usePar(par) ||
	    !par.getYN(possidestr,cutonpositiveside))
	return false;

    MultiID mid;
    if ( !par.get(cuttingobjectstr,mid) )
	return false;

    const EM::ObjectID id = EM::EMM().multiID2ObjectID( mid );
    mDynamicCastGet(EM::Surface*,surface,EM::EMM().getObject(id));
    if ( !surface || !surface->isLoaded() )
    {
	PtrMan<Executor> loader = EM::EMM().load(mid,0);
	if ( loader ) loader->execute();

	surface = dynamic_cast<EM::Surface*>(EM::EMM().getObject(id));
	if ( !surface || !surface->isLoaded() )
	    return false;
    }

    cuttingsurface = surface;
    return true;
}


} //namespace
