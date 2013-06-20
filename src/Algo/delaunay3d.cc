/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : June 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "delaunay3d.h"

#include "arraynd.h"
#include "survinfo.h"
#include "trigonometry.h"


ParallelDTetrahedralator::ParallelDTetrahedralator( DAGTetrahedraTree& dagt )
    : tree_( dagt )
    , israndom_( false )
{}


od_int64 ParallelDTetrahedralator::nrIterations() const
{ return tree_.coordList().size(); }


bool ParallelDTetrahedralator::doPrepare( int nrthreads )
{
    const od_int64 nrcoords = nrIterations();
    if ( israndom_ )
	permutation_.erase();
    else
    {
	mAllocVarLenArr( int, arr, nrcoords);
	for ( int idx=0; idx<nrcoords; idx++ )
	    arr[idx] = idx;
	
	std::random_shuffle( mVarLenArr(arr), arr+nrcoords );
	for ( int idx=0; idx<nrcoords; idx++ )
	    permutation_ += arr[idx];
    }
    
    return true;
}


bool ParallelDTetrahedralator::doWork( od_int64 start, od_int64 stop,
				       int threadid )
{
    TypeSet<int> delayedpts;
    for ( int idx=(int) start; idx<=stop && shouldContinue(); idx++ )
    {
	int dupid;
	const int insertptid = permutation_.size() ? permutation_[idx] : idx;
	if ( !tree_.insertPoint(insertptid,dupid) )
	    delayedpts += insertptid; //If failed, we delay the point.
	else
	    addToNrDone(1);
    }

    for ( int idx=0; idx<delayedpts.size() && shouldContinue(); idx++ )
    {
	int dupid;
	if ( !tree_.insertPoint( delayedpts[idx], dupid ) )
	    pErrMsg("Hmm");

	addToNrDone(1);
    }
    
    return true;
}


DAGTetrahedraTree::DAGTetrahedraTree()
    : coordlist_( 0 )
    , center_( Coord3(0,0,0) )  
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
    , initsizefactor_( 1 )			    
{}


DAGTetrahedraTree::DAGTetrahedraTree( const DAGTetrahedraTree& b )
    : coordlist_( 0 )
    , center_( Coord3(0,0,0) )  
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
    , initsizefactor_( 1 )			    
{
    *this = b;
}


DAGTetrahedraTree& DAGTetrahedraTree::operator=( const DAGTetrahedraTree& b )
{
    epsilon_ = b.epsilon_;
    if ( ownscoordlist_ )
	delete coordlist_;

    if ( b.ownscoordlist_ )
	coordlist_ = b.coordlist_ ? new TypeSet<Coord3>( *b.coordlist_ ) : 0;
    else
	coordlist_ = b.coordlist_;

    ownscoordlist_ = b.ownscoordlist_;
    tetrahedras_ = b.tetrahedras_;
    initsizefactor_ = b.initsizefactor_;

    initialcoords_[0] = b.initialcoords_[0];
    initialcoords_[1] = b.initialcoords_[1];
    initialcoords_[2] = b.initialcoords_[2];
    initialcoords_[3] = b.initialcoords_[3];

    return *this;
}


DAGTetrahedraTree::~DAGTetrahedraTree()
{
    if ( ownscoordlist_ && coordlist_ )
	coordlist_->erase();
}


bool DAGTetrahedraTree::computeCoordRanges( const TypeSet<Coord3>& coordlist,
	Interval<double>& xrg, Interval<double>& yrg, Interval<double>& zrg )
{
    if ( !coordlist.size() )
	return false;

    xrg.start = xrg.stop = coordlist[0].x;
    yrg.start = yrg.stop = coordlist[0].y;
    zrg.start = zrg.stop = coordlist[0].z;

    for ( int idx=1; idx<coordlist.size(); idx++ )
    {
	xrg.include( coordlist[idx].x );
	yrg.include( coordlist[idx].y );
	zrg.include( coordlist[idx].z );
    }

    return true;
}


bool DAGTetrahedraTree::setCoordList( const TypeSet<Coord3>& coordlist, 
				      bool copy )
{
    if ( coordlist_ && ownscoordlist_ )
	delete coordlist_;

    coordlist_ = 0;

    if ( coordlist.size()<1 )
	return false;

    Interval<double> xrg, yrg, zrg;
    computeCoordRanges( coordlist, xrg, yrg, zrg );

    if ( copy )
    {
	coordlist_ = new TypeSet<Coord3>( coordlist );
	ownscoordlist_ = true;
    }
    else
    {
	coordlist_ = const_cast<TypeSet<Coord3>* >( &coordlist );
	ownscoordlist_ = false;
    }

    setBBox( xrg, yrg, zrg );
    return true;
}

const double sq6 = Math::Sqrt( 6. );

bool DAGTetrahedraTree::setBBox( const Interval<double>& xrg, 
	const Interval<double>& yrg, const Interval<double>& zrg )
{
    tetrahedras_.erase();

    double xlength = xrg.width();
    double ylength = yrg.width();
    double zlength = zrg.width();
    center_ = Coord3( xrg.center(), yrg.center(), zrg.center() );

    bool narrow[3] = { mIsZero(xlength, 1e-3), mIsZero(ylength, 1e-3), 
		       mIsZero(zlength, 1e-3) };
    if ( narrow[0] || narrow[1] || narrow[2] )
    {
	const Coord mincoord = SI().minCoord( true );
	const Coord maxcoord = SI().maxCoord( true );
	if ( narrow[0] )
	{
	    center_.x = (mincoord.x+maxcoord.x)/2;
	    xlength = maxcoord.x-mincoord.x;
	}
	
	if ( narrow[1] )
	{
	    center_.y = (mincoord.y+maxcoord.y)/2;
	    ylength = maxcoord.y-mincoord.y;
	}

	if ( narrow[2] )
	{
	    center_.z = SI().zRange(true).center()*SI().zDomain().userFactor();
	    zlength = SI().zRange(true).width()*SI().zDomain().userFactor();
	}
    }

    const double k = initsizefactor_ * 2 * 
	Math::Sqrt( xlength*xlength+ylength*ylength+zlength*zlength );
    epsilon_ = k*(1e-5);

    const double sq2 = M_SQRT2;
    initialcoords_[0] = center_ + Coord3( 0, 0, 6*k );
    initialcoords_[1] = center_ + Coord3( 0, 6*sq2*k, -6*k );
    initialcoords_[2] = center_ + Coord3( -3*sq6*k, -3*sq2*k, -6*k );
    initialcoords_[3] = center_ + Coord3( 3*sq6*k, -3*sq2*k, -6*k );

    DAGTetrahedra initnode;
    initnode.coordindices_[0] = cInitVertex0();
    initnode.coordindices_[1] = cInitVertex1();
    initnode.coordindices_[2] = cInitVertex2();
    initnode.coordindices_[3] = cInitVertex3();
    tetrahedras_ +=initnode;

    return true;
}


void DAGTetrahedraTree::setInitSizeFactor( float newfactor )
{
    if ( newfactor<0.2 || mIsZero(initsizefactor_-newfactor,1e-5) )
	return; 

    if ( initialcoords_[0].isDefined() )
    {
	const double sq2 = M_SQRT2;
	double k = (initialcoords_[0].z-center_.z)/6;
	k = k * newfactor/initsizefactor_;
	initialcoords_[0] = center_ + Coord3( 0, 0, 6*k );
	initialcoords_[1] = center_ + Coord3( 0, 6*sq2*k, -6*k );
	initialcoords_[2] = center_ + Coord3(-3*sq6*k, -3*sq2*k, -6*k );
	initialcoords_[3] = center_ + Coord3( 3*sq6*k, -3*sq2*k, -6*k );
    }
    
    initsizefactor_ = newfactor;
}


bool DAGTetrahedraTree::init()
{
    if ( !tetrahedras_.size() ) return false;

    if ( !coordlist_ )
    {
	coordlist_ = new TypeSet<Coord3>;
	ownscoordlist_ = true;
    }

    return true;
}


int DAGTetrahedraTree::insertPoint( const Coord3& coord, int& dupid )
				    
{
    if ( !ownscoordlist_ )
	return cNoVertex();

    const int ci = coordlist_->size();
    (*coordlist_) += coord;

    if ( !insertPoint( ci, dupid ) )
    {
	if ( coordlist_->size()==ci+1 )
	    coordlist_->removeSingle( ci );

	return cNoVertex();
    }

    return ci;
}


bool DAGTetrahedraTree::insertPoint( int ci, int& dupid )
{
    dupid = cNoVertex();
    TypeSet<int> tis;
    char firstsharedface = cNoFace();
    int sharedv0 = cNoVertex(), sharedv1 = cNoVertex();
    const char res = searchTetrahedra( ci, 0, tis, firstsharedface, 
	    			       sharedv0, sharedv1, dupid );
    if ( res==cIsInside() ) 
	splitTetrahedraInside( ci, tis[0] );
    else if ( res==cIsOnFace() )
	splitTetrahedraOnFace( ci, tis[0], tis[1], firstsharedface );
    else if ( res==cIsOnEdge() )
	splitTetrahedraOnEdge( ci, tis, sharedv0, sharedv1 );
    else if ( res==cIsDuplicate() )
	return true;
    else  
	return false;

    return true;
}


#define mCrd( idx ) \
	(idx>=0 ? (*coordlist_)[idx]-center_ : initialcoords_[-idx-2]-center_)


char DAGTetrahedraTree::searchTetrahedra( const Coord3& pt )
{
    if ( !pt.isDefined() )
	return -1;

    int ci = coordlist_->size();
    (*coordlist_) += pt;
    char firstface;
    int v0, v1, dupid;
    double dist;

    char mode = location( ci, 0, firstface, dupid, v0, v1, dist );
    if ( mode!=cIsInside() )
    {
	(*coordlist_) -= pt;
	return -1; //Not inside the initial tetrahedra, so outside the body.
    }

    TypeSet<int> tis;
    mode = searchTetrahedra( ci, 0, tis, firstface, v0, v1, dupid );
    const int* crds = tetrahedras_[tis[0]].coordindices_;
    
    char res = cError();
    if ( mode==cError() )
    {
	pErrMsg("where");
	res = -1;
    }
    else if ( mode==cIsDuplicate() )
    {
	res = 0;
    }
    else if ( mode==cIsOnEdge() )
    {
	if ( v0<0 || v1<0 )
	    res = -1;
	else
	    res = 0;	
    }
    else if ( mode==cIsOnFace() )
    {
	int nrinits = 0;
	for ( int idx=0; idx<4; idx++ )
	{
	    if ( idx!=firstface )
		nrinits += crds[idx]<0;
	}

	if ( nrinits )
	    res = -1;
	else
	    res = 0;
    }
    else if ( crds[0]<0 || crds[1]<0 || crds[2]<0 || crds[3]<0 )
	res = -1;
    
    (*coordlist_) -= pt;
    return res;
}


char DAGTetrahedraTree::searchTetrahedra( int ci, int start, TypeSet<int>& tis,
	char& firstface, int& v0, int& v1, int& dupid ) const
{    
    tis.erase();
    tis += start;
    char res = cIsInside();

    const int* children = tetrahedras_[start].childindices_;
    double dist[4]; //The min face projection dist of each child tetrahedra;
    char face[4]; //The face with min projection dist in tetrahedra;
    for ( int childidx = 0; childidx<4; childidx++ )
    {
	face[childidx] = -1;
	dist[childidx] = mUdf(float);
	const int curchild = children[childidx];
	if ( curchild==cNoTetrahedra() ) continue;
	
	const char mode = location( ci, curchild, firstface, dupid, v0, v1, 
				    dist[childidx] );
	if ( mode==cIsOutside() ) 
	{
	    face[childidx] = firstface;
	    continue;
	}

	if ( mode==cError() || mode==cIsDuplicate() ) 
	    return mode;

	tis[0] = curchild;
	children =  tetrahedras_[curchild].childindices_;
	childidx = -1;
	res = mode;
    }

    //Selected tetrahedra should not have leaves!! Check.
    while ( tetrahedras_[tis[0]].childindices_[0]!=cNoTetrahedra() ||
	    tetrahedras_[tis[0]].childindices_[1]!=cNoTetrahedra() ||
   	    tetrahedras_[tis[0]].childindices_[2]!=cNoTetrahedra() ||
   	    tetrahedras_[tis[0]].childindices_[3]!=cNoTetrahedra() )
    {
	double mindist = 0;
	int child = -1;
	firstface = cNoFace();
	for ( int idx=0; idx<4; idx++ )
	{
	    const int childid = tetrahedras_[tis[0]].childindices_[idx];
	    if ( childid==cNoTetrahedra() || mIsUdf(dist[idx]) )
		continue;

	    if ( firstface==cNoFace() || mindist>dist[idx] )
	    {
		 mindist = dist[idx];
		 child = childid;
    		 firstface = face[idx];
	    }
	}

	if ( child<0 || child>=tetrahedras_.size() )
	    return cError();

	tis[0] = child;
	children = tetrahedras_[tis[0]].childindices_;
	res = cIsOnFace();

	for ( int childidx = 0; childidx<4; childidx++ )
	{
	    face[childidx] = -1;
	    dist[childidx] = mUdf(float);
	    const int curchild = children[childidx];
	    if ( curchild==cNoTetrahedra() ) continue;
	    
	    const char mode = location( ci, curchild, firstface, dupid, v0, 
		    			v1, dist[childidx] );
	    if ( mode==cIsOutside() ) 
	    {
		face[childidx] = firstface;
		continue;
	    }

	    if ( mode==cError() || mode==cIsDuplicate() )
		return mode;
	    
	    tis[0] = curchild;
	    children =  tetrahedras_[curchild].childindices_;
	    childidx = -1;
	    res = mode;
	}
    }

    if ( res==cIsOnFace() )
    {
	const int* crds = tetrahedras_[tis[0]].coordindices_;
	if ( firstface==0 )
    	    tis += searchFaceOnNeighbor( crds[1], crds[2], crds[3], tis[0] );
	else if ( firstface==1 )
    	    tis += searchFaceOnNeighbor( crds[0], crds[2], crds[3], tis[0] );
	else if ( firstface==2 )
    	    tis += searchFaceOnNeighbor( crds[0], crds[1], crds[3], tis[0] );
	else if ( firstface==3 )
    	    tis += searchFaceOnNeighbor( crds[0], crds[1], crds[2], tis[0] );
    }
    else if ( res==cIsOnEdge() )
    {
	for ( int idx=0; idx<tetrahedras_.size(); idx++ )
	{
	    const int* child = tetrahedras_[idx].childindices_;
	    if ( child[0]!=cNoTetrahedra() || child[1]!=cNoTetrahedra() ||
		 child[2]!=cNoTetrahedra() || child[3]!=cNoTetrahedra() )
		continue;

	    if ( idx==tis[0] )
		continue;

	    const int* crds = tetrahedras_[idx].coordindices_;
	    if ( (v0==crds[0] || v0==crds[1] || v0==crds[2] || v0==crds[3]) && 
		 (v1==crds[0] || v1==crds[1] || v1==crds[2] || v1==crds[3]) )
	    {
		tis += idx;
	    }
	}
    }
   
    return res;
}


char DAGTetrahedraTree::locationToTriangle( const Coord3& pt, const Coord3& a, 
	const Coord3& b, const Coord3& c, double& signeddist, 
	double& closestedgedist, char& dupidx, char& edgeidx ) const
{
    const Coord3 normal = (b-a).cross(c-b).normalize();
    signeddist = (a-pt).dot(normal);
    
    if ( !mIsZero(fabs(signeddist),epsilon_) )
	return cNotOnPlane();
    
    char bestedge = -1;
    double nearestedgedist = 0;
    double edgesqdist[3];
    closestedgedist = mUdf(float);
    
    for ( char idx=0; idx<3; idx++ )
    {
	const Coord3& v0 = idx==0 ? a : (idx==1 ? b : c);
	const Coord3& v1 = idx==0 ? b : (idx==1 ? c : a);
	bool duponfirst;
	const char res = isOnEdge( pt+signeddist*normal, v0, v1, normal, 
				   duponfirst, edgesqdist[idx] );
	const double dist = Math::Sqrt(fabs(edgesqdist[idx]));
	
	if ( res==cIsDuplicate() )
    	{
	    if ( !idx )
    		dupidx = duponfirst ? 0 : 1;
	    else if ( idx==1 )
		dupidx = duponfirst ? 1 : 2;
	    else
		dupidx = duponfirst ? 2 : 0;

    	    return cIsDuplicate();
    	}
    	else if ( res==cIsOnEdge() && (bestedge==-1 || dist<nearestedgedist) )
    	{
	    bestedge = idx;
	    nearestedgedist = dist;
    	}
	else if ( mIsUdf(closestedgedist) || closestedgedist>dist )
	    closestedgedist = dist;
    }

    if ( bestedge!=-1 )
    {
	edgeidx = bestedge;
        return cIsOnEdge();
    }

    if ( (edgesqdist[0]>0 && edgesqdist[1]>0 && edgesqdist[2]>0) ||
	 (edgesqdist[0]<0 && edgesqdist[1]<0 && edgesqdist[2]<0) )
        return cIsInside();

    return cIsOutside();
}


char DAGTetrahedraTree::isOnEdge( const Coord3& p, const Coord3& a, 
				  const Coord3& b, const Coord3 planenormal,
       				  bool& duponfirst, double& signedsqdist ) const
{
    const Coord3 dir = (b-a).normalize();
    const double t = (p-a).dot( dir );
    
    const double sqdist = (p-(a+t*dir)).sqAbs();
    const bool sign = planenormal.dot( dir.cross(p-b) ) > 0;
    signedsqdist = sign ? sqdist : -sqdist;
    if ( t<0 || t>1 || sqdist>epsilon_*epsilon_ )
	return cNotOnEdge();
    
    if ( mIsZero(t,1e-3) )
    {
	duponfirst = true;
	return cIsDuplicate();
    }
    else if ( mIsEqual(t,1,1e-3) )
    {
	duponfirst = false;
	return cIsDuplicate();
    }
    
    return cIsOnEdge();
}


char DAGTetrahedraTree::locationToTetrahedra( const Coord3& checkpt, 
	const Coord3* v, char& face, int& dupid, int& edgeend0, 
	int& edgeend1, double& mindist ) const
{
    if ( !checkpt.isDefined() )
	return cIsOutside();

    char edgeidx, dupidx;
    double signeddist[4], closestedgedist[4];
    char res[4];

    res[0] = locationToTriangle( checkpt, v[1], v[3], v[2], signeddist[0],
	    			 closestedgedist[0], dupidx, edgeidx );
    if ( res[0]==cIsDuplicate() )
    {
	dupid = dupidx==0 ? 1 : (dupidx==1 ? 3 : 2);
	return res[0];
    }
    else if ( res[0]==cIsOnEdge() )
    {
	edgeend0 = edgeidx==0 ? 1 : (edgeidx==1 ? 3 : 2 );
	edgeend1 = edgeidx==0 ? 3 : (edgeidx==1 ? 2 : 1 );
	face = 0;
	return cIsOnEdge();
    }

    res[1] = locationToTriangle( checkpt, v[0], v[2], v[3], signeddist[1],
	    			 closestedgedist[1], dupidx, edgeidx );
    if ( res[1]==cIsDuplicate() )
    {
	dupid = dupidx==0 ? 0 : (dupidx==1 ? 2 : 3);
	return res[1];
    }
    else if ( res[1]==cIsOnEdge() )
    {
	edgeend0 = edgeidx==0 ? 0 : (edgeidx==1 ? 2 : 3 );
	edgeend1 = edgeidx==0 ? 2 : (edgeidx==1 ? 3 : 0 );
	face = 1;
	return cIsOnEdge();
    }

    res[2] = locationToTriangle( checkpt, v[1], v[0], v[3], signeddist[2],
	    			 closestedgedist[2], dupidx, edgeidx );
    if ( res[2]==cIsDuplicate() )
    {
	dupid = dupidx==0 ? 1 : (dupidx==1 ? 0 : 3);
	return res[2];
    }
    else if ( res[2]==cIsOnEdge() )
    {
	edgeend0 = edgeidx==0 ? 1 : (edgeidx==1 ? 0 : 3 );
	edgeend1 = edgeidx==0 ? 0 : (edgeidx==1 ? 3 : 1 );
	face = 2;
	return cIsOnEdge();
    }

    res[3] = locationToTriangle( checkpt, v[0], v[1], v[2], signeddist[3],
	    			 closestedgedist[3], dupidx, edgeidx );
    if ( res[3]==cIsDuplicate() )
    {
	dupid = dupidx;
	return res[3];
    }
    else if ( res[3]==cIsOnEdge() )
    {
	edgeend0 = edgeidx;
	edgeend1 = edgeidx==0 ? 1 : (edgeidx==1 ? 2 : 0 );
	face = 3;
	return cIsOnEdge();
    }

    double minedgedist = 0;
    face = cNoFace();
    for ( char idx=0; idx<4; idx++ )
    {
	if ( res[idx]!=cIsInside() )
	    continue;

	if ( face==cNoFace() || closestedgedist[idx]<minedgedist  )
    	{
    	    face = idx;
	    minedgedist = closestedgedist[idx];
    	}
    }

    if ( face!=cNoFace() )
	return cIsOnFace();

    if ( (signeddist[0]>0 && signeddist[1]>0 && signeddist[2]>0 && 
	  signeddist[3]>0) || (signeddist[0]<0 && signeddist[1]<0 && 
	  signeddist[2]<0 && signeddist[3]<0) )
	return cIsInside();

    for ( char idx=0; idx<4; idx++ )
    {
	const double dist = fabs(signeddist[idx]);
	if ( face==cNoFace() || (!mIsZero(dist,epsilon_) && mindist>dist) )
	{
	    face = idx;
	    mindist = dist;
	}
    }

    return cIsOutside();
}


char DAGTetrahedraTree::location( int ci, int ti, char& face, int& dupid,
	int& edgeend0, int&edgeend1, double& mindist ) const 
{
    face = cNoVertex();	    
    dupid = cNoVertex();
    edgeend0 = cNoVertex(); 
    edgeend1 = cNoVertex();
  
    if ( ti==cNoTetrahedra() ) 
	return cIsOutside();

    const int* crds = tetrahedras_[ti].coordindices_;
    const Coord3 v[4] = { mCrd(crds[0]), mCrd(crds[1]), mCrd(crds[2]),
			  mCrd(crds[3]) };
    const char res = locationToTetrahedra( mCrd(ci), v, face, dupid, 
	    				   edgeend0, edgeend1, mindist );
    if ( res==cIsDuplicate() )
	dupid = crds[dupid];
    else if ( res==cIsOnEdge() )
    {
	edgeend0 = crds[edgeend0];
	edgeend1 = crds[edgeend1];
    }

    return res;
}


void DAGTetrahedraTree::splitTetrahedraInside( int ci, int ti )
{
    if ( ti<0 || ti>=tetrahedras_.size() )
	return;

    const int crds[] = { tetrahedras_[ti].coordindices_[0],
      			 tetrahedras_[ti].coordindices_[1],
   			 tetrahedras_[ti].coordindices_[2],
   			 tetrahedras_[ti].coordindices_[3] };
    
    const int ti0 = tetrahedras_.size();
    const int ti1 = ti0+1;
    const int ti2 = ti0+2;
    const int ti3 = ti0+3;

    DAGTetrahedra d0;
    d0.coordindices_[0] = crds[0];
    d0.coordindices_[1] = crds[1];
    d0.coordindices_[2] = crds[2];
    d0.coordindices_[3] = ci;
    d0.neighbors_[0] = ti3;
    d0.neighbors_[1] = ti1;
    d0.neighbors_[2] = ti2;
    d0.neighbors_[3] = searchFaceOnNeighbor( crds[0], crds[1], crds[2], ti );

    DAGTetrahedra d1;
    d1.coordindices_[0] = crds[0]; 
    d1.coordindices_[1] = crds[2];
    d1.coordindices_[2] = crds[3];
    d1.coordindices_[3] = ci;
    d1.neighbors_[0] = ti3;
    d1.neighbors_[1] = ti2;
    d1.neighbors_[2] = ti0;
    d1.neighbors_[3] = searchFaceOnNeighbor( crds[0], crds[2], crds[3], ti );
    
    DAGTetrahedra d2;
    d2.coordindices_[0] = crds[0];
    d2.coordindices_[1] = crds[3];
    d2.coordindices_[2] = crds[1];
    d2.coordindices_[3] = ci;
    d2.neighbors_[0] = ti3;
    d2.neighbors_[1] = ti0;
    d2.neighbors_[2] = ti1;
    d2.neighbors_[3] = searchFaceOnNeighbor( crds[0], crds[3], crds[1], ti );
    
    DAGTetrahedra d3;
    d3.coordindices_[0] = crds[1];
    d3.coordindices_[1] = crds[3];
    d3.coordindices_[2] = crds[2];
    d3.coordindices_[3] = ci;
    d3.neighbors_[0] = ti1;
    d3.neighbors_[1] = ti0;
    d3.neighbors_[2] = ti2;
    d3.neighbors_[3] = searchFaceOnNeighbor( crds[1], crds[2], crds[3], ti );
    
    tetrahedras_ += d0;
    tetrahedras_ += d1;
    tetrahedras_ += d2;
    tetrahedras_ += d3;

    tetrahedras_[ti].childindices_[0] = ti0;
    tetrahedras_[ti].childindices_[1] = ti1;
    tetrahedras_[ti].childindices_[2] = ti2;
    tetrahedras_[ti].childindices_[3] = ti3;
    
    TypeSet<int> v0s, v1s, v2s, tis; 
    v0s += crds[0]; v1s += crds[1]; v2s += crds[2]; tis += ti0;
    v0s += crds[0]; v1s += crds[2]; v2s += crds[3]; tis += ti1;
    v0s += crds[0]; v1s += crds[1]; v2s += crds[3]; tis += ti2;
    v0s += crds[1]; v1s += crds[2]; v2s += crds[3]; tis += ti3;

    legalizeTetrahedras( v0s, v1s, v2s, tis );
}


void DAGTetrahedraTree::splitTetrahedraOnFace( int ci, int ti0, int ti1, 
                                               char face )
{
    if ( ti0<0 || ti0>=tetrahedras_.size() || 
	 ti1<0 || ti1>=tetrahedras_.size() || face<0 )
    {
	pErrMsg("Could not split on face");
	return;
    }
    
    const int crds0[] = { tetrahedras_[ti0].coordindices_[0],
      			  tetrahedras_[ti0].coordindices_[1],
   			  tetrahedras_[ti0].coordindices_[2],
   			  tetrahedras_[ti0].coordindices_[3] };
    const int v0 = crds0[face];
    int s0=0, s1=0, s2=0; 
    if ( face==0 ) { s0 = crds0[1]; s1 = crds0[2]; s2 = crds0[3]; }
    else if ( face==1 ) { s0 = crds0[0]; s1 = crds0[3]; s2 = crds0[2]; }
    else if ( face==2 ) { s0 = crds0[0]; s1 = crds0[1]; s2 = crds0[3]; }
    else if ( face==3 ) { s0 = crds0[0]; s1 = crds0[2]; s2 = crds0[1]; }
    
    const int crds1[] = { tetrahedras_[ti1].coordindices_[0],
      			  tetrahedras_[ti1].coordindices_[1],
   			  tetrahedras_[ti1].coordindices_[2],
   			  tetrahedras_[ti1].coordindices_[3] };
    int v1 = 0; 
    for ( int idx=0; idx<4; idx++ )
    {
	if ( crds1[idx]!= s0 && crds1[idx]!= s1 && crds1[idx]!= s2 )
	{
	    v1 = crds1[idx];
	    break;
	}
    }

    const int nti0 = tetrahedras_.size();
    const int nti1 = nti0+1;
    const int nti2 = nti0+2;
    const int nti3 = nti0+3;
    const int nti4 = nti0+4;
    const int nti5 = nti0+5;

    DAGTetrahedra d0;
    d0.coordindices_[0] = v0;
    d0.coordindices_[1] = s0;
    d0.coordindices_[2] = s1;
    d0.coordindices_[3] = ci;
    d0.neighbors_[0] = nti3;
    d0.neighbors_[1] = nti2;
    d0.neighbors_[2] = nti1;
    d0.neighbors_[3] = searchFaceOnNeighbor( v0, s0, s1, ti0 );

    DAGTetrahedra d1;
    d1.coordindices_[0] = v0; 
    d1.coordindices_[1] = s2;
    d1.coordindices_[2] = s0;
    d1.coordindices_[3] = ci;
    d1.neighbors_[0] = nti4;
    d1.neighbors_[1] = nti0;
    d1.neighbors_[2] = nti2;
    d1.neighbors_[3] = searchFaceOnNeighbor( v0, s2, s0, ti0 );
    
    DAGTetrahedra d2;
    d2.coordindices_[0] = v0;
    d2.coordindices_[1] = s1;
    d2.coordindices_[2] = s2;
    d2.coordindices_[3] = ci;
    d2.neighbors_[0] = nti5;
    d2.neighbors_[1] = nti1;
    d2.neighbors_[2] = nti0;
    d2.neighbors_[3] = searchFaceOnNeighbor( v0, s1, s2, ti0 );
    
    DAGTetrahedra d3;
    d3.coordindices_[0] = v1;
    d3.coordindices_[1] = s1;
    d3.coordindices_[2] = s0;
    d3.coordindices_[3] = ci;
    d3.neighbors_[0] = nti0;
    d3.neighbors_[1] = nti4;
    d3.neighbors_[2] = nti5;
    d3.neighbors_[3] = searchFaceOnNeighbor( v1, s0, s1, ti1 );

    DAGTetrahedra d4;
    d4.coordindices_[0] = v1; 
    d4.coordindices_[1] = s0;
    d4.coordindices_[2] = s2;
    d4.coordindices_[3] = ci;
    d4.neighbors_[0] = nti1;
    d4.neighbors_[1] = nti5;
    d4.neighbors_[2] = nti3;
    d4.neighbors_[3] = searchFaceOnNeighbor( v1, s0, s2, ti1 );
    
    DAGTetrahedra d5;
    d5.coordindices_[0] = v1;
    d5.coordindices_[1] = s2;
    d5.coordindices_[2] = s1;
    d5.coordindices_[3] = ci;
    d5.neighbors_[0] = nti2;
    d5.neighbors_[1] = nti3;
    d5.neighbors_[2] = nti4;
    d5.neighbors_[3] = searchFaceOnNeighbor( v1, s1, s2, ti1 );

    tetrahedras_ += d0;
    tetrahedras_ += d1;
    tetrahedras_ += d2;
    tetrahedras_ += d3;
    tetrahedras_ += d4;
    tetrahedras_ += d5;
    tetrahedras_[ti0].childindices_[0] = nti0;
    tetrahedras_[ti0].childindices_[1] = nti1;
    tetrahedras_[ti0].childindices_[2] = nti2;
    tetrahedras_[ti1].childindices_[0] = nti3;
    tetrahedras_[ti1].childindices_[1] = nti4;
    tetrahedras_[ti1].childindices_[2] = nti5;

    TypeSet<int> v0s, v1s, v2s, tis; 
    v0s += v0; v1s += s0; v2s += s1; tis += nti0;
    v0s += v0; v1s += s2; v2s += s0; tis += nti1;
    v0s += v0; v1s += s1; v2s += s2; tis += nti2;
    v0s += v1; v1s += s1; v2s += s0; tis += nti3;
    v0s += v1; v1s += s0; v2s += s2; tis += nti4;
    v0s += v1; v1s += s2; v2s += s1; tis += nti5;

    legalizeTetrahedras( v0s, v1s, v2s, tis );
}


void DAGTetrahedraTree::splitTetrahedraOnEdge( int ci, const TypeSet<int>& tis,
     					       int& sharedv0, int& sharedv1 ) 
{
    TypeSet<int> v0s, v1s, v2s, ntis;
    
    for ( int idx=0; idx<tis.size(); idx++ )
    {
	const int crds[] = { tetrahedras_[tis[idx]].coordindices_[0],
			     tetrahedras_[tis[idx]].coordindices_[1],
			     tetrahedras_[tis[idx]].coordindices_[2],
       			     tetrahedras_[tis[idx]].coordindices_[3] };
	unsigned char s0=0, s1=0, s2=0, s3=0;
	for ( char idy=0; idy<4; idy++ )
	{
	    if ( crds[idy]==sharedv0 )
		s0 = idy;
	    else if ( crds[idy]==sharedv1 )
		s1 = idy;
	}

	if ( s0==0 && s1==1 )	   { s2 = 2; s3 = 3; }
	else if ( s0==1 && s1==0 ) { s2 = 3; s3 = 2; }
	else if ( s0==0 && s1==2 ) { s2 = 3; s3 = 1; }
	else if ( s0==2 && s1==0 ) { s2 = 1; s3 = 3; }
	else if ( s0==0 && s1==3 ) { s2 = 1; s3 = 2; }
	else if ( s0==3 && s1==0 ) { s2 = 2; s3 = 1; }
	else if ( s0==1 && s1==2 ) { s2 = 0; s3 = 3; }
	else if ( s0==2 && s1==1 ) { s2 = 3; s3 = 0; }
	else if ( s0==1 && s1==3 ) { s2 = 2; s3 = 0; }
	else if ( s0==3 && s1==1 ) { s2 = 0; s3 = 2; }
	else if ( s0==2 && s1==3 ) { s2 = 0; s3 = 1; }
	else if ( s0==3 && s1==2 ) { s2 = 1; s3 = 0; }

	const int nti0 = tetrahedras_.size();
	const int nti1 = tetrahedras_.size()+1;

	DAGTetrahedra d0;
    	d0.coordindices_[0] = crds[s0];
    	d0.coordindices_[1] = crds[s2];
    	d0.coordindices_[2] = crds[s3];
	d0.coordindices_[3] = ci;
	d0.neighbors_[0] = nti1 ;
	d0.neighbors_[3] = searchFaceOnNeighbor( crds[s0], crds[s2], crds[s3],
						 tis[idx] );
	DAGTetrahedra d1;
    	d1.coordindices_[0] = crds[s1];
    	d1.coordindices_[1] = crds[s3];
    	d1.coordindices_[2] = crds[s2];
	d1.coordindices_[3] = ci;
	d1.neighbors_[0] = nti0 ;
	d1.neighbors_[3] = searchFaceOnNeighbor( crds[s1], crds[s3], crds[s2],
						 tis[idx] );
    	tetrahedras_ += d0;
	tetrahedras_ += d1;
	tetrahedras_[tis[idx]].childindices_[0] = nti0;
	tetrahedras_[tis[idx]].childindices_[1] = nti1;
	
	v0s += crds[s0]; v1s += crds[s2]; v2s += crds[s3]; ntis += nti0;
	v0s += crds[s1]; v1s += crds[s3]; v2s += crds[s2]; ntis += nti1;
    }

    for ( int idx=0; idx<ntis.size(); idx++ )
    {
	tetrahedras_[ntis[idx]].neighbors_[1] =
	    searchFaceOnList( ci, v0s[idx], v2s[idx], idx, ntis );
	tetrahedras_[ntis[idx]].neighbors_[2] =
	    searchFaceOnList( ci, v0s[idx], v1s[idx], idx, ntis );
    }

    legalizeTetrahedras( v0s, v1s, v2s, ntis );
}


int DAGTetrahedraTree::searchFaceOnList( int ci, int v0, int v1, int repeat, 
					 const TypeSet<int>& tis ) const
{
    for ( int idx=0; idx<tis.size(); idx++ )
    {
	const int* crds = tetrahedras_[tis[idx]].coordindices_;
	if ( (v0==crds[0] || v0==crds[1] || v0==crds[2] || v0==crds[3]) &&
	     (v1==crds[0] || v1==crds[1] || v1==crds[2] || v1==crds[3]) &&
	     (ci==crds[0] || ci==crds[1] || ci==crds[2] || ci==crds[3]) &&
	     idx!=repeat )
	    return tis[idx];	
    }

    return cNoTetrahedra();
}


void DAGTetrahedraTree::legalizeTetrahedras( TypeSet<int>& v0s, 
	TypeSet<int>& v1s, TypeSet<int>& v2s, TypeSet<int>& tis )
{
    int start = 0;
    while ( v0s.size()>start )
    {
	int v0 = v0s[start];
	int v1 = v1s[start];
	int v2 = v2s[start];
	const int ti = tis[start];
	if ( start>10000 )
	{
	    v0s.removeRange( 0, start );
	    v1s.removeRange( 0, start );
	    tis.removeRange( 0, start );
	    start = 0;
	}
	else 
    	    start++;

	if ( tetrahedras_[ti].childindices_[0]!=cNoTetrahedra() ||
	     tetrahedras_[ti].childindices_[1]!=cNoTetrahedra() ||
	     tetrahedras_[ti].childindices_[2]!=cNoTetrahedra() ||
	     tetrahedras_[ti].childindices_[3]!=cNoTetrahedra() )
	    continue;

	const int crds[] = { tetrahedras_[ti].coordindices_[0],
			     tetrahedras_[ti].coordindices_[1],
			     tetrahedras_[ti].coordindices_[2],
			     tetrahedras_[ti].coordindices_[3] };

	const int nbs[] = { tetrahedras_[ti].neighbors_[0],
			    tetrahedras_[ti].neighbors_[1],
			    tetrahedras_[ti].neighbors_[2],
			    tetrahedras_[ti].neighbors_[3] };

	int checkti = cNoTetrahedra();
	int newpt = cNoVertex();
	for ( int idx=0; idx<4; idx++ )
	{
	    if ( crds[idx]!=v0 && crds[idx]!=v1 && crds[idx]!=v2 )
	    {
		checkti = nbs[idx];
		newpt = crds[idx];
		if ( idx==0 ) 
		{ 
		    v0 = crds[1]; v1 = crds[2]; v2 = crds[3]; 
		}
		else if ( idx==1 ) 
		{ 
		    v0 = crds[0]; v1 = crds[3]; v2 = crds[2];
	       	}
		else if ( idx==2 ) 
		{ 
		    v0 = crds[0]; v1 = crds[1]; v2 = crds[3]; 
		}
		else if ( idx==3 ) 
		{ 
		    v0 = crds[0]; v1 = crds[2]; v2 = crds[1]; 
		}

		break;
	    }
	}

	if ( checkti==cNoTetrahedra() || newpt==cNoVertex() ) continue;
	if ( checkti==ti )
	{
	    pErrMsg("Checkti duplicate");
	    continue;
	}

	const int checkcrds[] = { tetrahedras_[checkti].coordindices_[0],
				  tetrahedras_[checkti].coordindices_[1],
     				  tetrahedras_[checkti].coordindices_[2],
     				  tetrahedras_[checkti].coordindices_[3] };
	int checkpt =cNoVertex();
	for ( int idx=0; idx<4; idx++ )
	{
	    if (checkcrds[idx]!=v0 && checkcrds[idx]!=v1 && checkcrds[idx]!=v2)
	    {
		checkpt = checkcrds[idx];
		break;
	    }
	}

	if ( tetrahedras_[checkti].childindices_[0]!=cNoTetrahedra() ||
	     tetrahedras_[checkti].childindices_[1]!=cNoTetrahedra() ||
	     tetrahedras_[checkti].childindices_[2]!=cNoTetrahedra() ||
	     tetrahedras_[checkti].childindices_[3]!=cNoTetrahedra() )
	    continue;

	if ( checkpt==newpt || checkpt==cNoVertex() )
	    continue;

	const Coord3 p = mCrd(checkpt);
	const Coord3 q = mCrd(newpt);
	const Coord3 a = mCrd(v0);
	const Coord3 b = mCrd(v1);
	const Coord3 c = mCrd(v2);
	if ( !isInsideCircumSphere( p, q, a, b, c ) )
	    continue;

	char onedge; //ret 0 if on v0-v1, 1 if on v1-v2, 2 if on v0-v2.
	const char pq_intersect_abc = isIntersect( p, q, a, b, c, onedge );
	if ( pq_intersect_abc==cIsDuplicate() )
	    continue; 

	if ( pq_intersect_abc==cIsInside() )
	{
	    const int nti0 = tetrahedras_.size();
	    const int nti1 = nti0+1;
	    const int nti2 = nti0+2;
	    DAGTetrahedra d0;
	    d0.coordindices_[0] = newpt;
	    d0.coordindices_[1] = v0;
	    d0.coordindices_[2] = v1;
	    d0.coordindices_[3] = checkpt;
	    d0.neighbors_[0] = searchFaceOnNeighbor( v0, v1, checkpt, checkti );
	    d0.neighbors_[1] = nti2;
	    d0.neighbors_[2] = nti1;
	    d0.neighbors_[3] = searchFaceOnNeighbor( v0, v1, newpt, ti );

	    DAGTetrahedra d1;
	    d1.coordindices_[0] = newpt;
	    d1.coordindices_[1] = v2;
	    d1.coordindices_[2] = v0;
	    d1.coordindices_[3] = checkpt;
	    d1.neighbors_[0] = searchFaceOnNeighbor( v0, v2, checkpt, checkti );
	    d1.neighbors_[1] = nti0;
	    d1.neighbors_[2] = nti2;
	    d1.neighbors_[3] = searchFaceOnNeighbor( v0, v2, newpt, ti );

	    DAGTetrahedra d2;
	    d2.coordindices_[0] = newpt;
	    d2.coordindices_[1] = v1;
	    d2.coordindices_[2] = v2;
	    d2.coordindices_[3] = checkpt;
	    d2.neighbors_[0] = searchFaceOnNeighbor( v1, v2, checkpt, checkti );
	    d2.neighbors_[1] = nti1;
	    d2.neighbors_[2] = nti0;
	    d2.neighbors_[3] = searchFaceOnNeighbor( v1, v2, newpt, ti );

	    tetrahedras_[ti].childindices_[0] = nti0;
	    tetrahedras_[ti].childindices_[1] = nti1;
	    tetrahedras_[ti].childindices_[2] = nti2;
	    tetrahedras_[checkti].childindices_[0] = nti0;
	    tetrahedras_[checkti].childindices_[1] = nti1;
	    tetrahedras_[checkti].childindices_[2] = nti2;
	    
	    tetrahedras_ += d0;
	    tetrahedras_ += d1;
	    tetrahedras_ += d2;
	    v0s += checkpt; v1s += v0; v2s += v1;  tis += nti0;
	    v0s += checkpt; v1s += v0; v2s += v2;  tis += nti1;
	    v0s += checkpt; v1s += v1; v2s += v2;  tis += nti2;
	}
	else if ( pq_intersect_abc==cIsOutside() )
	{
	    const int checknb0 = searchFaceOnNeighbor(v0,v1,checkpt,checkti);
	    const int checknb1 = searchFaceOnNeighbor(v0,v2,checkpt,checkti);
	    const int checknb2 = searchFaceOnNeighbor(v1,v2,checkpt,checkti);
	    
	    int s0 = cNoVertex(), s1 = cNoVertex(), s2 = cNoVertex();
	    int nbti = searchFaceOnNeighbor( v0, v1, newpt, ti );
	    if ( nbti==checknb0 )
	    {
		s0 = v0; s1 = v1; s2 = v2;
	    }
	    else
	    {
		nbti = searchFaceOnNeighbor( v0, v2, newpt, ti );	
		if ( nbti==checknb1 )
		{
		    s0 = v2; s1 = v0; s2 = v1;
		}
		else
   		{
    		    nbti = searchFaceOnNeighbor( v1, v2, newpt, ti );
		    if ( nbti==checknb2 )
			s0 = v1; s1 = v2; s2 = v0;
   		}
	    }

	    if ( nbti==cNoTetrahedra() || s0==cNoVertex() )
		continue;

	    const int nti0 = tetrahedras_.size(); 
	    const int nti1 = nti0+1; 		
	    DAGTetrahedra d0;	 	
	    d0.coordindices_[0] = newpt; 
	    d0.coordindices_[1] = s2; 
	    d0.coordindices_[2] = s0; 	
	    d0.coordindices_[3] = checkpt;
	    d0.neighbors_[0] = searchFaceOnNeighbor(s0,s2,checkpt,checkti);	
	    d0.neighbors_[1] = searchFaceOnNeighbor(s0,checkpt,newpt,nbti);
	    d0.neighbors_[2] = nti1; 				
	    d0.neighbors_[3] = searchFaceOnNeighbor(s0,s2,newpt,ti); 	
	    
	    DAGTetrahedra d1;	 					
	    d1.coordindices_[0] = newpt; 			
	    d1.coordindices_[1] = s1; 		
	    d1.coordindices_[2] = s2; 	
	    d1.coordindices_[3] = checkpt; 		
	    d1.neighbors_[0] = searchFaceOnNeighbor(s1,s2,checkpt,checkti);
	    d1.neighbors_[1] = nti0;				
	    d1.neighbors_[2] = searchFaceOnNeighbor(s1,checkpt,newpt,nbti);
	    d1.neighbors_[3] = searchFaceOnNeighbor(s1,s2,newpt,ti);	
	    
	    tetrahedras_[ti].childindices_[0] = nti0; 			
	    tetrahedras_[ti].childindices_[1] = nti1; 		
	    tetrahedras_[checkti].childindices_[0] = nti0; 
	    tetrahedras_[checkti].childindices_[1] = nti1;
	    tetrahedras_[nbti].childindices_[0] = nti0; 
	    tetrahedras_[nbti].childindices_[1] = nti1;
	    tetrahedras_ += d0; 		
	    tetrahedras_ += d1; 	
	    
	    v0s += checkpt; v1s += v0; v2s += s2;  tis += nti0;		
	    v0s += checkpt; v1s += s1; v2s += s2;  tis += nti1;	
	}
	else if ( pq_intersect_abc==cIsOnEdge() )
	{ 
	    int s0 = cNoVertex(), s1 = cNoVertex(), s2 = cNoVertex();
	    int nbti = cNoTetrahedra();
	    if ( onedge==cEdge01() )
	    {
		s0 = v0; s1 = v1; s2 = v2; 
		nbti = searchFaceOnNeighbor( v0, v1, newpt, ti );
	    }
	    else if ( onedge==cEdge12() )
	    {
		s0 = v1; s1 = v2; s2 = v0; 
		nbti = searchFaceOnNeighbor( v1, v2, newpt, ti );
	    }
	    else if ( onedge==cEdge20() )
	    {
		s0 = v2; s1 = v0; s2 = v1; 
		nbti = searchFaceOnNeighbor( v2, v0, newpt, ti );
	    }
	    
	    const int checknbti = searchFaceOnNeighbor(s0,s1,checkpt,checkti);
	    if ( nbti==checknbti )
		continue;

	    if ( nbti==cNoTetrahedra() && checknbti==cNoTetrahedra() ) 	
	    {								
		const int nti0 = tetrahedras_.size();		
		const int nti1 = nti0+1;		
		DAGTetrahedra d0;		
		d0.coordindices_[0] = newpt;
		d0.coordindices_[1] = s2;
		d0.coordindices_[2] = s0;
		d0.coordindices_[3] = checkpt;
		d0.neighbors_[0] = searchFaceOnNeighbor(s0,s2,checkpt,checkti);	
		d0.neighbors_[2] = nti1;				
		d0.neighbors_[3] = searchFaceOnNeighbor(s0,s2,newpt,ti);
	
		DAGTetrahedra d1;				
		d1.coordindices_[0] = newpt;		
		d1.coordindices_[1] = s1;
		d1.coordindices_[2] = s2;
		d1.coordindices_[3] = checkpt;	
		d1.neighbors_[0] = searchFaceOnNeighbor(s1,s2,checkpt,checkti);	
		d1.neighbors_[1] = nti0;				
		d1.neighbors_[3] = searchFaceOnNeighbor(s1,s2,newpt,ti);
	
		tetrahedras_[ti].childindices_[0] = nti0;	
		tetrahedras_[ti].childindices_[1] = nti1;
		tetrahedras_[checkti].childindices_[0] = nti0;
		tetrahedras_[checkti].childindices_[1] = nti1;
		tetrahedras_ += d0;			
		tetrahedras_ += d1;		
		v0s += checkpt; v1s += s0; v2s += s2;  tis += nti0;		
		v0s += checkpt; v1s += s1; v2s += s2;  tis += nti1;	
	    }							
	    else if ( nbti!=cNoTetrahedra() && checknbti!=cNoTetrahedra() )
	    {								
		int nbpt = 0;					
		const int nbcrds[] = { tetrahedras_[nbti].coordindices_[0],
				       tetrahedras_[nbti].coordindices_[1],
				       tetrahedras_[nbti].coordindices_[2],
				       tetrahedras_[nbti].coordindices_[3] }; 
		for ( int idx=0; idx<4; idx++ )		
		{				
		    if (nbcrds[idx]!=newpt && nbcrds[idx]!=s0 && 
			    nbcrds[idx]!=s1) 
		    {			
			nbpt = nbcrds[idx];
			break;		
		    }		
		}

		const int crds0[] = { tetrahedras_[checknbti].coordindices_[0],	
				      tetrahedras_[checknbti].coordindices_[1], 
				      tetrahedras_[checknbti].coordindices_[2], 
				      tetrahedras_[checknbti].coordindices_[3]};
		if ( nbpt!= crds0[0] && nbpt!= crds0[1] &&		
		     nbpt!= crds0[2] && nbpt!= crds0[3] )	
		    continue;	
	    
		const int nti0 = tetrahedras_.size();			
		const int nti1 = nti0+1;				
		const int nti2 = nti0+2;			
		const int nti3 = nti0+3;		
		
		DAGTetrahedra d0;		
		d0.coordindices_[0] = newpt;
		d0.coordindices_[1] = s2;	
		d0.coordindices_[2] = s0;
		d0.coordindices_[3] = checkpt;
		d0.neighbors_[0] = searchFaceOnNeighbor(s0,s2,checkpt,checkti);
		d0.neighbors_[1] = nti2;				
		d0.neighbors_[2] = nti1;			
		d0.neighbors_[3] = searchFaceOnNeighbor(s0,s2,newpt,ti);	
		
		DAGTetrahedra d1;			
		d1.coordindices_[0] = newpt;
		d1.coordindices_[1] = s1;
		d1.coordindices_[2] = s2;	
		d1.coordindices_[3] = checkpt;
		d1.neighbors_[0] = searchFaceOnNeighbor(s1,s2,checkpt,checkti);
		d1.neighbors_[1] = nti0;			
		d1.neighbors_[2] = nti3;				
		d1.neighbors_[3] = searchFaceOnNeighbor(s1,s2,newpt,ti);	
		
		DAGTetrahedra d2;			
		d2.coordindices_[0] = newpt;
		d2.coordindices_[1] = s0;
		d2.coordindices_[2] = nbpt;	
		d2.coordindices_[3] = checkpt;
		d2.neighbors_[0] =
		    searchFaceOnNeighbor(checkpt,nbpt,s0,checknbti);
		d2.neighbors_[1] = nti3;
		d2.neighbors_[2] = nti0;
		d2.neighbors_[3] = searchFaceOnNeighbor(newpt,nbpt,s0,nbti); 
		
		DAGTetrahedra d3;	
		d3.coordindices_[0] = newpt;				
		d3.coordindices_[1] = nbpt;			
		d3.coordindices_[2] = s1;		
		d3.coordindices_[3] = checkpt;			
		d3.neighbors_[0] =
		    searchFaceOnNeighbor(checkpt,nbpt,s1,checknbti);
		d3.neighbors_[1] = nti1;	
		d3.neighbors_[2] = nti2;
		d3.neighbors_[3] = searchFaceOnNeighbor(newpt,nbpt,s1,nbti); 

		tetrahedras_[ti].childindices_[0] = nti0;			
		tetrahedras_[ti].childindices_[1] = nti1;		
		tetrahedras_[checkti].childindices_[0] = nti0;
		tetrahedras_[checkti].childindices_[1] = nti1;
		tetrahedras_[nbti].childindices_[0] = nti2;
		tetrahedras_[nbti].childindices_[1] = nti3;
		tetrahedras_[checknbti].childindices_[0] = nti2;
		tetrahedras_[checknbti].childindices_[1] = nti3;

		tetrahedras_ += d0;				
		tetrahedras_ += d1;			
		tetrahedras_ += d2;		
		tetrahedras_ += d3;	
		v0s += checkpt; v1s += s2; v2s += s0;  tis += nti0;		
		v0s += checkpt; v1s += s2; v2s += s1;  tis += nti1;	
		v0s += checkpt; v1s += nbpt; v2s += s0; tis += nti2; 
		v0s += checkpt; v1s += nbpt; v2s += s1; tis += nti3; 
    	    }
	}
    }
}


char DAGTetrahedraTree::isIntersect( const Coord3& p, const Coord3& q, 
	const Coord3& a, const Coord3& b, const Coord3& c, char& onedge ) const
{
    onedge = cNotOnEdge();
    const Coord3 normal = (a-b).cross(b-c).normalize();
    const double t = -normal.dot(p-a)/normal.dot(q-p);
    const Coord3 intersectpt = p+t*(q-p);
    if ( mIsZero( (intersectpt-a).sqAbs(), 1e-3 ) ||
	 mIsZero( (intersectpt-b).sqAbs(), 1e-3 ) || 
	 mIsZero( (intersectpt-c).sqAbs(), 1e-3 ) )
	return cIsDuplicate();

    if ( pointInTriangle3D( intersectpt, a, b, c, epsilon_ ) )
    {
	if ( pointOnEdge3D( intersectpt, a, b, epsilon_ ) )
	    onedge = cEdge01();
	else if ( pointOnEdge3D( intersectpt, b, c, epsilon_ ) )
	    onedge = cEdge12();
	else if ( pointOnEdge3D( intersectpt, a, c, epsilon_) )
	    onedge = cEdge20();
	
	return onedge==cNotOnEdge() ? cIsInside() : cIsOnEdge();
    }
    else
	return cIsOutside();
}


void DAGTetrahedraTree::addTriangle( int v0, int v1, int v2, 
				     TypeSet<int>& res ) const
{
    for ( int t=0; t<res.size()/3; t++ )
    {
	const int tidx = 3 * t;
	if ( (res[tidx]==v0 || res[tidx+1]==v0 || res[tidx+2]==v0) &&
	     (res[tidx]==v1 || res[tidx+1]==v1 || res[tidx+2]==v1) &&
	     (res[tidx]==v2 || res[tidx+1]==v2 || res[tidx+2]==v2) )
	    return;
    }
    
    res += v0;
    res += v1;
    res += v2;
}


#define mValidTetrahedra() \
    const int* child = tetrahedras_[idx].childindices_; \
    if ( child[0]!=cNoTetrahedra() || child[1]!=cNoTetrahedra() || \
	 child[2]!=cNoTetrahedra() || child[3]!=cNoTetrahedra() ) \
	continue;  \
    const int* c = tetrahedras_[idx].coordindices_; 


bool DAGTetrahedraTree::getTetrahedras( TypeSet<int>& result ) const
{
    for ( int idx=tetrahedras_.size()-1; idx>=0; idx-- )
    {
	mValidTetrahedra()
	if ( c[0]<0 || c[1]<0 || c[2]<0 || c[3]<0 ) continue;

	result += c[0];
	result += c[1];
	result += c[2];
	result += c[3];
    }

    return result.size();
}


bool DAGTetrahedraTree::getSurfaceTriangles( TypeSet<int>& result) const
{
    for ( int idx=0; idx<tetrahedras_.size(); idx++ )
    {
	mValidTetrahedra()
	if ( (c[0]<0) + (c[1]<0) + (c[2]<0) + (c[3]<0)!=1 )
	    continue;
   
	if ( c[0]<0 )
	    addTriangle( c[1], c[3], c[2], result );
	else if ( c[1]<0 )
	    addTriangle( c[0], c[2], c[3], result );
	else if ( c[2]<0 )
	    addTriangle( c[0], c[3], c[1], result );
	else
	    addTriangle( c[0], c[1], c[2], result );
    }

    return result.size();
}


bool DAGTetrahedraTree::getConnections( int vertex, TypeSet<int>& result ) const
{
    result.erase();
    for ( int idx=tetrahedras_.size()-1; idx>=0; idx-- )
    {
	mValidTetrahedra()
	if ( c[0]!=vertex && c[1]!=vertex && c[2]!=vertex && c[3]!=vertex )
	    continue;

	for ( int idy=0; idy<4; idy++ )
	{
	    if ( c[idy]<0 || c[idy]==vertex || result.isPresent(c[idy]) )
		continue;

	    result += c[idy];
	}
    }

    return result.size();
}


int DAGTetrahedraTree::searchFaceOnNeighbor( int a, int b, int c, int ti) const 
{
    if ( ti==cNoTetrahedra() ) return cNoTetrahedra();

    const int crds[4] = { tetrahedras_[ti].coordindices_[0], 
			  tetrahedras_[ti].coordindices_[1], 
			  tetrahedras_[ti].coordindices_[2], 
			  tetrahedras_[ti].coordindices_[3] };
    for ( int idx=0; idx<4; idx++ )
    {
	if ( crds[idx]!=a && crds[idx]!=b && crds[idx]!=c )
	    return searchFaceOnChild(a,b,c,tetrahedras_[ti].neighbors_[idx]);
    }

    return cNoTetrahedra();
}


int DAGTetrahedraTree::searchFaceOnChild( int v0, int v1, int v2, int ti) const 
{
    if ( ti==cNoTetrahedra() ) return ti;
    const int child[4] = { tetrahedras_[ti].childindices_[0],
    			   tetrahedras_[ti].childindices_[1],
    			   tetrahedras_[ti].childindices_[2],
    			   tetrahedras_[ti].childindices_[3] };
    if ( child[0]==cNoTetrahedra() && child[1]==cNoTetrahedra() &&
	 child[2]==cNoTetrahedra() && child[3]==cNoTetrahedra() )
	return ti;

    for ( int idy=0; idy<4; idy++ )
    {
	if ( child[idy]==cNoTetrahedra() )
	    continue;

	const int gc[4] = { tetrahedras_[child[idy]].coordindices_[0],
			    tetrahedras_[child[idy]].coordindices_[1],
			    tetrahedras_[child[idy]].coordindices_[2],
			    tetrahedras_[child[idy]].coordindices_[3] };
	if ( (v0==gc[0] || v0==gc[1] || v0==gc[2] || v0==gc[3]) && 
	     (v1==gc[0] || v1==gc[1] || v1==gc[2] || v1==gc[3]) && 
	     (v2==gc[0] || v2==gc[1] || v2==gc[2] || v2==gc[3]) )
	    return searchFaceOnChild( v0, v1, v2, child[idy] );
    }

    return cNoTetrahedra();//This means the face does not exist anymore.
}
 

DAGTetrahedraTree::DAGTetrahedra::DAGTetrahedra()
{
    coordindices_[0] = DAGTetrahedraTree::cNoVertex();
    coordindices_[1] = DAGTetrahedraTree::cNoVertex();
    coordindices_[2] = DAGTetrahedraTree::cNoVertex();
    coordindices_[3] = DAGTetrahedraTree::cNoVertex();

    childindices_[0] = DAGTetrahedraTree::cNoTetrahedra();
    childindices_[1] = DAGTetrahedraTree::cNoTetrahedra();
    childindices_[2] = DAGTetrahedraTree::cNoTetrahedra();
    childindices_[3] = DAGTetrahedraTree::cNoTetrahedra();

    neighbors_[0] = DAGTetrahedraTree::cNoTetrahedra();
    neighbors_[1] = DAGTetrahedraTree::cNoTetrahedra();
    neighbors_[2] = DAGTetrahedraTree::cNoTetrahedra();
    neighbors_[3] = DAGTetrahedraTree::cNoTetrahedra();
}


bool DAGTetrahedraTree::DAGTetrahedra::operator==(const DAGTetrahedra& d) const
{
    TypeSet<int> vertices;
    vertices += coordindices_[0];
    vertices += coordindices_[1];
    vertices += coordindices_[2];
    vertices += coordindices_[3];

    return vertices.isPresent( d.coordindices_[0] ) && 
	   vertices.isPresent( d.coordindices_[1] ) &&
	   vertices.isPresent( d.coordindices_[2] ) && 
	   vertices.isPresent( d.coordindices_[3] );
}	


DAGTetrahedraTree::DAGTetrahedra&  DAGTetrahedraTree::DAGTetrahedra::operator=(
	const DAGTetrahedraTree::DAGTetrahedra& b )
{
    for ( int idx=0; idx<4; idx++ )
    {
	coordindices_[idx] = b.coordindices_[idx];
	childindices_[idx] = b.childindices_[idx];
	neighbors_[idx] = b.neighbors_[idx];
    }

    return *this;
}

