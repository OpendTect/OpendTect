/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfaceedgelineimpl.cc,v 1.26 2007-05-22 03:23:23 cvsnanne Exp $";



#include "emsurfaceedgelineimpl.h"

#include "emmanager.h"
#include "emhorizon3d.h"
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


bool SurfaceConnectLine::isNodeOK(const RowCol& rc) const
{
    return EdgeLineSegment::isNodeOK(rc) &&
	   horizon_.isDefined(connectingsection,rc.getSerialized());
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


SurfaceCutLine::SurfaceCutLine( Horizon3D& surf, const SectionID& sect )
    : EdgeLineSegment( surf, sect )
    , t2d( 0 )
    , cuttinghorizon_( 0 )
    , cutonpositiveside( false )
    , meshdist( getMeshDist() )
{}


bool SurfaceCutLine::shouldHorizonTrack(int idx, const RowCol& dir) const
{
    if ( idx && idx!=size()-1 )
	return false;

    if ( size()==1 ) 
	return true;

    //Dont track if we are tracking in the direction of the existing line
    const RowCol backnode = idx ? (*this)[idx-1] : (*this)[1];
    const RowCol& curnode = (*this)[idx];

    const RowCol backdir = (backnode-curnode).getDirection();
    const float angle = idx ? backdir.counterClockwiseAngleTo(dir)
			    : backdir.clockwiseAngleTo(dir);
    static const float threehundreddegs = M_PI+2*M_PI/3;
    return angle<threehundreddegs;
}
    



float SurfaceCutLine::getMeshDist()
{
    const SurveyInfo& survinfo( SI() );
    const BinID step( survinfo.inlStep(), survinfo.crlStep() );
    return survinfo.transform(BinID(0,0)).distTo(survinfo.transform(step));
}


bool SurfaceCutLine::isNodeOK(const RowCol& testrc) const
{
    /*
    const int cacheidx = cacherc.indexOf(testrc,false);

    float disttosurface = mUdf(float);
    if ( cacheidx==-1 )
    {
	const Coord3 ntpos = horizon_.getPos( section, testrc.getSerialized() );
	if ( ntpos.isDefined() )
	{
	    disttosurface = cuttinghorizon_->geometry().normalDistance(ntpos);
	    if ( cutonpositiveside ) disttosurface = -disttosurface;
	}

	SurfaceCutLine* thisp = const_cast<SurfaceCutLine*>(this);
	thisp->distcache += disttosurface;
	thisp->cacherc+=testrc;
	thisp->poscache += ntpos;
	thisp->ischanged += false;
    }
    else
	disttosurface = distcache[cacheidx];

    if ( mIsUdf(disttosurface) )
	return false;

//  return fabs(disttosurface)<meshdist/2;
    return disttosurface<meshdist/2;
    */
    return true;
}


bool SurfaceCutLine::trackWithCache( int start, bool forward,
				     const EdgeLineSegment* prev,
				     const EdgeLineSegment* next )
{
    /*
    if ( !cuttinghorizon_ ) 
	return EdgeLineSegment::track( start, forward, prev, next );

    if ( size()>1 && isAtCuttingEdge(start) )
	return false;

    if ( (forward && start!=size()-1) || (!forward && start ) )
    {
	pErrMsg( "Tracking non-edge" );
	return false;
    }

    const RowCol& sourcerc = (*this)[start];

    RowCol backnode;
    if ( !getNeighborNode(start,!forward,backnode,prev,next) )
    {
	if ( !getHorizonStart(start,!forward,backnode) )
	    return false;
    }

    const RowCol backnodedir = (backnode-sourcerc).getDirection();
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    const int nrdirs = dirs.size();
    int curdiridx = dirs.indexOf(backnodedir) + nrdirs;

    const RowCol& step = horizon_.geometry().step();

    RowCol lastdefineddir;
    bool firstfound = false;
    bool rightsidefound = false;
    int bestidx = -1;
    bool changebestposition = false;
    Coord3 bestpos;
    float bestscore = mUdf(float);


    const int backnodecacheidx = cacherc.indexOf(backnode,false);
    float backnodedistance;
    if ( backnodecacheidx==-1 )
    {
	const Coord3 pos = horizon_.getPos( section, backnode.getSerialized() );
	poscache += pos;
	if ( !pos.isDefined() )
	    mSetUdf(backnodedistance);
	else
	{
	    backnodedistance = cuttinghorizon_->geometry().normalDistance(pos,t2d);
	    if ( cutonpositiveside ) backnodedistance = -backnodedistance;
	}

	distcache += backnodedistance;
	cacherc += backnode;
	ischanged += false;
    }
    else
	backnodedistance = distcache[backnodecacheidx];

    if ( !mIsUdf(backnodedistance) && backnodedistance>meshdist/16 )
	rightsidefound = true;

    for ( int idx=1; idx<nrdirs; idx++ )
    {
	//Ignore the first, since that is the backnode
	if ( forward ) curdiridx--;
	else curdiridx++;

	const RowCol& curdir = dirs[curdiridx%nrdirs];
	const bool isnb = curdir.isNeighborTo(lastdefineddir,RowCol(1,1),true);
	if ( firstfound && !isnb )
	    break;

	const RowCol currc = curdir*step+sourcerc;
	const int cacheidx = cacherc.indexOf(currc,false);
	float distance;
	if ( cacheidx==-1 )
	{
	    const Coord3 pos = horizon_.getPos( section, currc.getSerialized() );
	    poscache += pos;
	    if ( !pos.isDefined() )
		mSetUdf(distance);
	    else
	    {
		distance = cuttinghorizon_->geometry().normalDistance(pos,t2d);
		if ( cutonpositiveside ) distance = -distance;
	    }

	    distcache += distance;
	    cacherc += currc;
	    ischanged += false;
	}
	else
	    distance = distcache[cacheidx];

	if ( !mIsUdf(distance) )
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

	    if ( idx==nrdirs-1 && bestidx==-1 )
	    {
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos,
			    	  &sourcerc );
		if ( !mIsUdf(curscore) &&
			curscore<meshdist/2 && curscore<=bestscore )
		{
		    bestidx = curdiridx;
		    changebestposition = changecurbestpos;
		    bestpos = curbestpos;
		    bestscore = curscore;
		}

		break;
	    }

	    // If we are really close, check if we can get a match 

	    if ( zerodist )
	    {
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos,
			          &sourcerc );
		if ( !mIsUdf( curscore ) &&
			curscore<meshdist/16 && curscore<=bestscore  )
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

		const RowCol prevrc = prevdir*step+sourcerc;
		Coord3 prevbestpos;
		bool changeprevbestpos;
		const float prevscore =
		    computeScore( prevrc, changeprevbestpos, prevbestpos,
			          &sourcerc );

		if ( !mIsUdf( prevscore ) && !changeprevbestpos )
		{
		    changebestposition = false;
		    bestidx = dirs.indexOf(prevdir);
		    break;
		}
		
		Coord3 curbestpos;
		bool changecurbestpos;
		const float curscore =
		    computeScore( currc, changecurbestpos, curbestpos,
			          &sourcerc );

		if ( mIsUdf(prevscore) && mIsUdf(curscore) )
		break;

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
	    const RowCol prevrc = lastdefineddir*step+sourcerc;
	    Coord3 prevbestpos;
	    bool changeprevbestpos;
	    const float prevscore =
		computeScore( prevrc, changeprevbestpos, prevbestpos,
			      &sourcerc );

	    if ( !mIsUdf(prevscore) && prevscore<meshdist/2 )
	    {
		changebestposition = false;
		bestidx = dirs.indexOf(lastdefineddir);
	    }
	}
	
	if ( bestidx==-1 )
	    return false;
    }

    const RowCol newrc = sourcerc+dirs[bestidx%nrdirs]*step;
    if ( forward )
	(*this) += newrc;
    else insert( 0, newrc );

    // TODO: Needs better testing. Gives weired results now
    if ( changebestposition )
    {
	const int cacheindex = cacherc.indexOf(newrc);
	float distance = cuttinghorizon_->geometry().normalDistance(bestpos,t2d);
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
    */

    return true;
}


bool SurfaceCutLine::isAtCuttingEdge(int idx) const
{
    const Coord3 ntpos = horizon_.getPos(section,(*this)[idx].getSerialized());

    const Interval<float> xinterval( ntpos.x-10, ntpos.x+10 );
    const Interval<float> yinterval( ntpos.y-10, ntpos.y+10 );
    const StepInterval<float>& zrange = SI().zRange(false);
    const Interval<float> zinterval( zrange.start, zrange.stop );

    TypeSet<PosID> cuttingnodes;
    cuttinghorizon_->geometry().findPos( xinterval,yinterval,zinterval,
				      &cuttingnodes );
    if ( cuttingnodes.isEmpty() )
	return false;
    
    int closestaboveidx = -1, closestbelowidx = -1;
    float closestabove, closestbelow;
    for ( int idy=0; idy<cuttingnodes.size(); idy++ )
    {
	const Coord3 cuttingpos = cuttinghorizon_->getPos(cuttingnodes[idy]);
	const float diff =  cuttingpos.z - ntpos.z;
	if ( fabs(diff)<zrange.step/4 )
	    return cuttinghorizon_->geometry().isAtEdge(cuttingnodes[idy]);

	if ( diff>0 && ( closestaboveidx==-1  || diff<closestabove ))
	{ closestabove = diff; closestaboveidx=idy; }
	if ( diff<0 && ( closestbelowidx==-1  || diff<closestbelow ))
	{ closestbelow = diff; closestbelowidx=idy; }
    }

    if ( closestaboveidx!=-1 && closestbelowidx!=-1 )
    {
	return
	    cuttinghorizon_->geometry().isAtEdge(cuttingnodes[closestaboveidx]) &&
	    cuttinghorizon_->geometry().isAtEdge(cuttingnodes[closestbelowidx]);
    }
    else if ( closestaboveidx!=-1 )
	return cuttinghorizon_->geometry().isAtEdge(cuttingnodes[closestaboveidx]);
    else if ( closestbelowidx!=-1 )
	return cuttinghorizon_->geometry().isAtEdge(cuttingnodes[closestbelowidx]);

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
	    horizon_.setPos( section, cacherc[idx].getSerialized(),
		  	    poscache[idx], true );
	    ischanged[idx] = false;
	}
    }
}

#define mSetupCreateCut(createline) \
    const SectionRelations* relations = \
				horizon_.relations.getRelations(section); \
    if ( !relations || relidx>=relations->cuttingsurfaces.size() ) \
	return 0; \
 \
    mDynamicCastGet(Horizon3D*,cuttinghorizon_, \
	    	    EMM().getObject(relations->cuttingsurfaces[relidx])); \
    const SectionID& cuttingsection = relations->cuttingsections[relidx]; \
    const bool cutonposside = relations->cutpositiveside[relidx]; \
 \
    if ( !cuttinghorizon_ || \
	 !horizon_.geometry().hasSection(section) || \
	 !cuttinghorizon_->geometry().hasSection(cuttingsection) ) \
	return 0; \
 \
    EdgeLineSet& elset = \
	    *horizon_.edgelinesets.getEdgeLineSet(section,createline); \
    if ( !&elset ) return 0

SurfaceCutLine* SurfaceCutLine::createCutFromSeed( Horizon3D& horizon,
    const SectionID& section, int relidx, const RowCol& seed,
    bool bothdirs, const FloatMathFunction* t2d )
{
    return 0;

    /*
    mSetupCreateCut(false);

    SurfaceCutLine* scl = new SurfaceCutLine( horizon, section );
    scl->setCuttingSurface( cuttinghorizon_, cutonposside );
    scl->setTime2Depth( t2d );
    (*scl) += seed;
    if ( !scl->isNodeOK(seed) )
    {
	delete scl; return 0;
    }

    bool forward = true;

    RowCol backnode;
    if ( !scl->getNeighborNode(0,!forward,backnode,0,0) )
    {
	if ( !scl->getSurfaceStart(0,!forward,backnode) )
	    return false;
    }

    (*scl) += backnode;

    while ( scl->trackWithCache(scl->size()-1,forward,0,0) )
    {
	//Check that new node doesn't exist on segment before
	const RowCol& newtracked = scl->last();
	if ( scl->indexOf(newtracked,forward) != scl->size()-1 )
	{
	    scl->remove( scl->size()-1 );
	    break;
	}
    }

    if ( scl->size()>1 )
	scl->remove(0);

    forward = false;

    while ( scl->trackWithCache(0,forward,0,0) )
    {
	const RowCol& newtracked = scl->first();
	if ( scl->indexOf(newtracked,forward) != 0 )
	{
	    delete scl; scl = 0; break;
	}
    }

    return scl;
    */
}


SurfaceCutLine* SurfaceCutLine::createCutFromEdges( Horizon3D& horizon,
   const SectionID& section, int relidx, const FloatMathFunction* t2d )
{
    return 0;
    /*
    mSetupCreateCut(true);

    int lineidx = elset.getMainLine();
    EdgeLine& el = *elset.getLine( lineidx );
    if ( !&el ) return 0;

    const float meshdist = getMeshDist();

    TypeSet<RowCol> border;
    TypeSet<float> distances;
    computeDistancesAlongLine( el, *cuttinghorizon, t2d, border, distances,
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
	SurfaceCutLine* scl = new SurfaceCutLine( horizon, section );
	scl->setCuttingSurface( cuttinghorizon_, cutonposside );
	scl->setTime2Depth( t2d );
	(*scl) += possibletrackstarts[idx];
	while ( scl->trackWithCache(forward ? scl->size()-1 : 0,forward,0,0) )
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
    */
}



void SurfaceCutLine::computeDistancesAlongLine( const EdgeLine& line,
	const Horizon3D& cuttinghorizon, const  FloatMathFunction* t2d,
	TypeSet<RowCol>& border, TypeSet<float>& distances, bool negate,
	bool usecaching )
{
    /*
    const Horizon3D& horizon = line.getHorizon();
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
	    const Coord3 coord = horizon.getPos(section,rc.getSerialized() );
	    dist = cuttinghorizon.geometry().normalDistance( coord, t2d );
	    if ( negate ) dist = -dist;
	}

	if ( !mIsUdf(dist) )
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
    */
}


bool SurfaceCutLine::internalIdenticalSettings(
					const EdgeLineSegment& els_ ) const
{
    const SurfaceCutLine* els = reinterpret_cast<const SurfaceCutLine*>(&els_);
    return els->cuttinghorizon_==cuttinghorizon_ &&
	   els->cutonpositiveside==cutonpositiveside &&
	   EdgeLineSegment::internalIdenticalSettings(els_);
}


float SurfaceCutLine::computeScore( const RowCol& targetrc,
	bool& changescorepos, Coord3& scorepos, const RowCol* sourcerc )
{
    /*
    const int cacheidx = cacherc.indexOf(targetrc,false);
    float distance;
    Coord3 pos;
    if ( cacheidx==-1 )
    {
	poscache += pos = horizon_.getPos( section, targetrc.getSerialized() );
	if ( !pos.isDefined() )
	    mSetUdf(distance);
	else
	{
	    distance = cuttinghorizon_->geometry().normalDistance(pos,t2d);
	    if ( cutonpositiveside ) distance = -distance;
	}

	distcache += distance;
	cacherc += targetrc;
	ischanged += false;
    }
    else
    {
	distance = distcache[cacheidx];
	pos = poscache[cacheidx];
    }

    if ( !pos.isDefined() )
	return mUdf(float);

    TypeSet<PosID> cuttingnodes;
    if ( !getCuttingPositions( pos, cuttingnodes ) )
	return mUdf(float);
    
    if ( cuttingnodes.isEmpty() )
    {
	// If we don't find any cutting nodes, it might be that they are spaced
	// with double spacing here (which is bad). If there is a cutting node
	// in the continuation, we can give a score, since we are surrounded
	// by the cutting horizon
	//
	// Should perhaps be removed later
	if ( !sourcerc )
	    return mUdf(float);

	const Coord backpos = horizon_.getPos(section,sourcerc->getSerialized());
	const Coord diff( pos.x-backpos.x, pos.y-backpos.y);
	const Coord targetpos(pos.x+diff.x,pos.y+diff.y);

	if ( getCuttingPositions(targetpos,cuttingnodes) && cuttingnodes.size())
	{
	    changescorepos = false;
	    return distance;
	}

	return mUdf(float);
    }

    scorepos = pos;

    float closestabove, closestbelow;
    int closestaboveidx=-1, closestbelowidx=-1;
    if ( cuttingnodes.size() )
    {
	for ( int idy=0; idy<cuttingnodes.size(); idy++ )
	{
	    const Coord3 cuttingpos = cuttinghorizon_->getPos(cuttingnodes[idy]);
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
	    const RowCol closestaboverc( cuttingnodes[closestaboveidx].subID());
	    const RowCol closestbelowrc( cuttingnodes[closestbelowidx].subID());
	    if ( !closestaboverc.isNeighborTo(closestbelowrc,
					      cuttinghorizon_->geometry().step()) )
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
	const Coord3 cuttingpos = cuttinghorizon_->getPos(cuttingnodes[0]);
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
    */
	return 0;
}


bool SurfaceCutLine::getCuttingPositions( const Coord& pos,
					  TypeSet<EM::PosID>& cuttingnodes )
{
    const Interval<float> xinterval( pos.x-10, pos.x+10 );
    const Interval<float> yinterval( pos.y-10, pos.y+10 );
    const StepInterval<float>& zrange = SI().zRange(false);
    const Interval<float> zinterval( zrange.start, zrange.stop );

    cuttinghorizon_->geometry().findPos( xinterval,yinterval,zinterval,
				      &cuttingnodes );
    return true;
}


void SurfaceCutLine::fillPar(IOPar& par) const
{
    EdgeLineSegment::fillPar(par);
    par.set(cuttingobjectstr,cuttinghorizon_->multiID());
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

    const EM::ObjectID id = EM::EMM().getObjectID( mid );
    mDynamicCastGet(EM::Horizon3D*,newsurf,EM::EMM().getObject(id));
    if ( !newsurf || !newsurf->isLoaded() )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader(id,0);
	if ( loader ) loader->execute();

	newsurf = dynamic_cast<EM::Horizon3D*>(EM::EMM().getObject(id));
	if ( !newsurf || !newsurf->isLoaded() )
	    return false;
    }

    cuttinghorizon_ = newsurf;
    return true;
}


} //namespace
