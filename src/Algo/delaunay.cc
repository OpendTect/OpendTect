/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: delaunay.cc,v 1.11 2008-06-19 14:25:03 cvskris Exp $";

#include "delaunay.h"

#include "positionlist.h"
#include "sorting.h"
#include "trigonometry.h"

#include "iostream"


DelaunayTriangulation::DelaunayTriangulation( const Coord2List& coords )
    : coordlist_( coords )
    , totalsz_( 0 )  
{
    int sz = 0;
    int previd = -1;
    while ( coordlist_.nextID(previd)!= -1 )
    {
	sz++;
	previd = coordlist_.nextID(previd);
    }
    
    totalsz_ = sz;
}


DelaunayTriangulation::~DelaunayTriangulation()
{}


int DelaunayTriangulation::triangulate()
{
    stack_.erase();
    result_.erase();
    neighbours_.setSize( 6*totalsz_-6, 0 );

    createPermutation();

    if ( totalsz_ < 3 )
	return 0;

    if ( !ensureDistinctness() )
	return -2;

    int lr;
    const int firsttriidx = findHealthyTriangle( 0, 1, 2, lr );
    if ( lr == 0 )
	return -1;

    if ( !createInitialTriangles( firsttriidx, lr ) )
	return -2;

    int ledg = 2;
    int ltri = lr == -1 ? result_.size()/3 - 1 : 0;
    for ( int idx=firsttriidx+1; idx<totalsz_; idx++ )
    {
	if ( !insertTriangle( idx, ledg, ltri ) )
	    return -2;
    }
  
    return 1;
}


bool DelaunayTriangulation::insertTriangle( int i0, int& ledg, int& ltri )
{
    const int pi0 = permutation_[i0];
    const int pi1 = result_[3*ltri+ledg-1];
    const int pi2 = ledg > 2 ? result_[3*ltri] : result_[3*ltri+ledg];
    const int lr = getSideOfLine( pi1, pi2, pi0 );
    int rtri;
    int redg;
    if ( lr > 0 )
    {
	rtri = ltri;
	redg = ledg;
	ltri = -1;
    }
    else
    {
	const int idx = -neighbours_[3*ltri+ledg-1];
	rtri = idx/3-1;
	redg = (idx%3)+1;
    }

    visibleEdge( pi0, ltri, ledg, rtri, redg );
    const int firstnewtriangle = result_.size()/3;
    int link = -neighbours_[3*ltri+ledg-1];
    for ( ; ; )
    {
	const int tri = link/3-1;
	const int edg = (link % 3) + 1;
	link = -neighbours_[3*tri+edg-1];
	
	const int c2 = result_[3*tri+edg-1];
	const int c1 = edg <= 2 ? result_[3*tri+edg] : result_[3*tri];
	const int trinr = result_.size()/3;

	neighbours_[3*tri+edg-1] = trinr+1;
	result_ += c1;
	result_ += c2;
	result_ += pi0;

	neighbours_[3*trinr] = tri+1;
	neighbours_[3*trinr+1] = trinr;
	neighbours_[3*trinr+2] = trinr+2;
	
	stack_ += result_.size()/3;

	if ( permutation_.size() < stack_.size() )
	    return false;

    	if ( tri == rtri && edg == redg )
	    break;
    }

    neighbours_[3*ltri+ledg-1] = -3*firstnewtriangle-4;
    neighbours_[3*firstnewtriangle+1] = -result_.size()-2;
    neighbours_[result_.size()-1] = -link;
    ltri = firstnewtriangle;
    ledg = 2;
  
    const int error= swapEdge( pi0, ltri, ledg );
    if ( error != 0 )
	return false;

    return true;
}


#define mGetVertice0( triidx ) ((triidx)*3)
#define mGetVertice1( triidx ) ((triidx)*3+1)
#define mGetVertice2( triidx ) ((triidx)*3+2)

int DelaunayTriangulation::swapEdge( int pid, int& ltri, int& ledg )
{
    for ( ; ; )
    {
	if ( !stack_.size() )
	    break;

	const int lastidx = stack_.size()-1;
	const int tri = stack_[lastidx]-1;
	stack_.remove( lastidx );

	int edg;
	int c2;
	if ( result_[mGetVertice0(tri)] == pid )
	{
	    edg = 2;
	    c2 = result_[mGetVertice2(tri)];
	}
	else if ( result_[mGetVertice1(tri)] == pid )
	{
	    edg = 3;
	    c2 = result_[mGetVertice0(tri)];
	}
	else
	{
	    edg = 1;
	    c2 = result_[mGetVertice1(tri)];
	}

	const int c0 = result_[3*tri+edg-1];
	const int nbid = neighbours_[3*tri+edg-1];
	int idx;
	int c1;
	if ( neighbours_[3*(nbid-1)] == tri+1 )
	{
	    idx = 1;
	    c1 = result_[3*(nbid-1)+2];
	}
	else if ( neighbours_[3*(nbid-1)+1] == tri+1 )
	{
	    idx = 2;
	    c1 = result_[3*(nbid-1)];
	}
	else
	{
	    idx = 3;
	    c1 = result_[3*(nbid-1)+1];
	}

	const int swap = selectDiagonal( pid, c0, c1, c2 );

	if ( swap == 1 )
	{
	   const int wedg0 = wrap ( edg-1, 1, 3 ); 
	   const int wedg1 = wrap ( edg+1, 1, 3 ); 
	   const int widx0 = wrap ( idx-1, 1, 3 ); 
	   const int widx1 = wrap ( idx+1, 1, 3 ); 

	   result_[3*tri+wedg1-1] = c1;
	   result_[3*(nbid-1)+widx1-1] = pid;
	   const int r = neighbours_[3*tri+wedg1-1];
	   const int s = neighbours_[3*(nbid-1)+widx1-1];
	   neighbours_[3*tri+wedg1-1] = nbid;
	   neighbours_[3*(nbid-1)+widx1-1] = tri+1;
	   neighbours_[3*tri+edg-1] = s;
	   neighbours_[3*(nbid-1)+idx-1] = r;

	   if ( 0 < neighbours_[3*(nbid-1)+widx0-1] )
	       stack_ +=nbid; 

	   if ( 0 < s )
	   {
	       if ( neighbours_[3*(s-1)] == nbid )
		   neighbours_[3*(s-1)] = tri+1;
	       else if ( neighbours_[3*(s-1)+1] == nbid )
		   neighbours_[3*(s-1)+1] = tri+1;
	       else
		   neighbours_[3*(s-1)+2] = tri+1;
	       
	       stack_ += tri+1;
	       if ( permutation_.size() < stack_.size() )
		   return 8;
	   }
	   else
	   {
	       if ( nbid == ltri+1 && widx1 == ledg )
	       {
		   ltri = tri;
		   ledg = edg;
	       }
	       
	       int l = - ( 3*(tri+1)+edg-1 );
	       int tt = tri;
	       int ee = wedg0;
	       
	       while ( 0 < neighbours_[3*tt+ee-1] )
	       {
		   tt = neighbours_[3*tt+ee-1]-1;
		   if ( result_[3*tt] == c0 )
		       ee = 3;
		   else if ( result_[3*tt+1] == c0 )
		       ee = 1;
		   else
		       ee = 2;
	       }
	       
	       neighbours_[3*tt+ee-1] = l;
	   }
	  
	   if ( 0 < r )
	   {
  	       if ( neighbours_[3*(r-1)] == tri+1 )
    		   neighbours_[3*(r-1)] = nbid;
  	       else if ( neighbours_[3*(r-1)+1] == tri+1 )
    		   neighbours_[3*(r-1)+1] = nbid;
  	       else
    		   neighbours_[3*(r-1)+2] = nbid;
	   }
	   else
	   {
  	       if ( tri == ltri && wedg1 == ledg )
  	       {
    		   ltri = nbid-1;
    		   ledg = idx;
  	       }
	       
  	       int l = - ( 3*nbid+idx-1 );
  	       int tt = nbid-1;
  	       int ee = widx0;
	       
  	       while ( 0 < neighbours_[3*tt+ee-1] )
  	       {
    		   tt = neighbours_[3*tt+ee-1]-1;
		   
    		   if ( result_[3*tt] == c2 )
      		       ee = 3;
    		   else if ( result_[3*tt+1] == c2 )
      		       ee = 1;
    		   else
      		       ee = 2;
 	       }
  	       
  	       neighbours_[3*tt+ee-1] = l;
	   }
	}
    }

    return 0;
}


void DelaunayTriangulation::visibleEdge( int pi0, int& ltri, int& ledg, 
					 int& rtri, int& redg )
{
    bool done = true;
    if ( ltri < 0 )
    {
	done = false;
	ltri = rtri;
	ledg = redg;
    }

    findRightMostEdge( rtri, redg, pi0 );
    if ( done ) return;
    
    findLeftMostEdge( ltri, ledg, pi0 );
}


void DelaunayTriangulation::findRightMostEdge( int& rtri, int& redg, int pid )
{
    for ( ; ; )
    {
    	const int link = -neighbours_[3*rtri+redg-1];
    	const int tri = link/3-1;
    	const int edg = 1+link%3;
    	const int c0 = result_[3*tri+edg-1];
    	const int c1 = edg <= 2 ? result_[3*tri+edg] : result_[3*tri];
    	
	if ( getSideOfLine(c0, c1, pid) <= 0 )
	    break;   

	rtri = tri;
    	redg = edg;
    }
}


void DelaunayTriangulation::findLeftMostEdge( int& ltri, int& ledg, int pid )
{
    int tri = ltri;
    int edg = ledg;
    for ( ; ; )
    {
	const int c1 = result_[3*tri+edg-1];
	edg = wrap( edg-1, 1, 3 );

	while ( 0 < neighbours_[3*tri+edg-1] )
	{
	    tri = neighbours_[3*tri+edg-1]-1;
	    if ( result_[3*tri] == c1 )
		edg = 3;
	    else if ( result_[3*tri+1] == c1 )
		edg = 1;
	    else
		edg = 2;
	}
   
	const int c0 = result_[3*tri+edg-1];
	if ( getSideOfLine(c0, c1, pid) <= 0 )
	    break;
    }

    ltri = tri;
    ledg = edg;
}


bool DelaunayTriangulation::createInitialTriangles( int firstidx, int lr )
{
    if ( firstidx < 2 )
	return false;

    if ( lr == -1 )
    {
	result_ += permutation_[0];
	result_ += permutation_[1];
	result_ += permutation_[firstidx];
	neighbours_[2] = -3;

	int c0 = 0;
	int c1 = 1;
	for ( int idx=1; idx<firstidx-1; idx++ )
	{
	    c0 = c1;
	    c1 = idx+1;
	    result_ += permutation_[c0];
	    result_ += permutation_[c1];
	    result_ += permutation_[firstidx];
	    neighbours_[3*idx] = -3*(idx+1);
	    neighbours_[3*idx+1] = idx+1;
	    neighbours_[3*idx+2] = idx;
	}

	const int nr = result_.size()/3;
	neighbours_[3*(nr-1)] = -3*nr-1;
	neighbours_[3*(nr-1)+1] = -5;
    }
    else 
    {
	result_ += permutation_[1];
	result_ += permutation_[0];
	result_ += permutation_[firstidx];
	neighbours_[0] = -4;

	int c0 = 0;
	int c1 = 1;
	for ( int idx=1; idx<firstidx-1; idx++ )
	{
	    c0 = c1;
	    c1 = idx+1;
	    result_ += permutation_[c1];
	    result_ += permutation_[c0];
	    result_ += permutation_[firstidx];
	    neighbours_[3*(idx-1)+2] = idx+1;
	    neighbours_[3*idx] = -3*(idx+1)-3;
	    neighbours_[3*idx+1] = idx;
	}

	const int nr = result_.size()/3;
	neighbours_[3*(nr-1)+2] = -3*nr;
	neighbours_[1] = -3*nr-2;
    }
    
    return true;
}


void DelaunayTriangulation::createPermutation()
{
    permutation_.erase();
    for ( int idx=0; idx<totalsz_; idx++ )
	permutation_ += idx;

    TypeSet<Coord> coordcopy;
    int previd = -1;
    while ( coordlist_.nextID(previd)!= -1 )
    {
	int nextid = coordlist_.nextID(previd);
	if ( nextid != -1 )
    	    coordcopy += coordlist_.get( nextid );

	previd = nextid;
    }

    sort_coupled( coordcopy.arr(), permutation_.arr(), permutation_.size() );
}


bool DelaunayTriangulation::ensureDistinctness() 
{
    int p0 = permutation_[0];
    for ( int idx = 1; idx < totalsz_; idx++ )
    {
	int p1 = p0;
	p0 = permutation_[idx];
	
	int id = -1;
	const Coord c1 = coordlist_.get( p1 );
	const Coord c0 = coordlist_.get( p0 );
	
	for ( int idy = 0; idy <= 1; idy++ )
	{
	    double max = fabs(c0.x) > fabs(c1.x) ? fabs(c0.x) : fabs(c1.x);
	    double diff = c0.x - c1.x;
	    if ( idy == 1)
	    {
		max = fabs(c0.y) > fabs(c1.y) ? fabs(c0.y) : fabs(c1.y);
		diff = c0.y - c1.y;
	    }

	    if ( 100.0 * mEpsilon() * ( max+1.0 ) < fabs( diff ) )
	    {
		id = idy;
		break;
	    }
	}

	if ( id == -1 )
      	    return false;
    }
    
    return true;
}


int DelaunayTriangulation::findHealthyTriangle(int i0, int i1, int id, int& lr) 
{
    const int nrcoords = permutation_.size();
    for ( int idx=id; idx<nrcoords; idx++ )
    {
	lr = getSideOfLine(permutation_[i0],permutation_[i1],permutation_[idx]);
	if ( lr )
	    return idx;
    }

    return -1;
}


int DelaunayTriangulation::getSideOfLine( int pi0, int pi1, int pid )
{
    const Coord v01 = coordlist_.get( pi1 ) - coordlist_.get( pi0 );
    const Coord vt = coordlist_.get( pid ) - coordlist_.get( pi0 );

    double tolabs = fabs(v01.x) > fabs(v01.y) ? fabs(v01.x) : fabs(v01.y);
    tolabs = tolabs > fabs(vt.x) ? tolabs : fabs(vt.x);
    tolabs = 0.000001 * ( tolabs > fabs(vt.y) ? tolabs : fabs(vt.y) );
    
    double prod = v01.y * vt.x - v01.x * vt.y;
    int res;

    if ( tolabs < prod )
    	res = 1;
    else if ( -tolabs <= prod )
    	res = 0;
    else if ( prod < -tolabs )
    	res = -1;

    return res;
}


int DelaunayTriangulation::selectDiagonal(int c0, int c1, int c2, int c3) const
{
  Coord d10 = coordlist_.get( c1 ) - coordlist_.get( c0 );
  Coord d12 = coordlist_.get( c1 ) - coordlist_.get( c2 );
  Coord d30 = coordlist_.get( c3 ) - coordlist_.get( c0 );
  Coord d32 = coordlist_.get( c3 ) - coordlist_.get( c2 );
  
  double maxxy301 = fabs(d30.x) > fabs(d30.y) ? fabs(d30.x) : fabs(d30.y);
  maxxy301 = maxxy301 > fabs(d10.y) ? maxxy301 : fabs(d10.y);
  maxxy301 = 100.0*mEpsilon()*(maxxy301>fabs(d10.x) ? maxxy301 : fabs(d10.x));
  
  double maxxy123 = fabs(d32.x) > fabs(d32.y) ? fabs(d32.x) : fabs(d32.y);
  maxxy123 = maxxy123 > fabs(d12.y) ? maxxy123 : fabs(d12.y);
  maxxy123 = 100.0*mEpsilon()*(maxxy123>fabs(d12.x) ? maxxy123 : fabs(d12.x));

  const double inpro301 = d10.x * d30.x + d10.y * d30.y;
  const double inpro123 = d12.x * d32.x + d12.y * d32.y;

  if ( maxxy301 < inpro301 && maxxy123 < inpro123 )
      return -1;  
  else if ( inpro301 < -maxxy301 && inpro123 < -maxxy123 )
      return 1;
  else
  {
    double max = maxxy301 > maxxy123 ? maxxy301 : maxxy123;
    double determ = ( d10.x * d30.y - d30.x * d10.y ) * inpro123 
		  + ( d32.x * d12.y - d12.x * d32.y ) * inpro301;

    if ( max < determ )
      return -1;
    else if ( determ < -max )
      return 1;
    else
	return 0;
    }
}


int DelaunayTriangulation::wrap( int val, int vallo, int valhi )
{
    const int low = vallo < valhi ? vallo : valhi;
    const int hig = vallo > valhi ? vallo : valhi;
    const int width = hig - low + 1;
    if ( width == 1 )
	return low;

    int posmod = (val-low) % width;
    if ( posmod < 0 )
	posmod += abs(width);

    return low+posmod;
}


double DelaunayTriangulation::mEpsilon() const
{
    double r = 1.0;
    while ( 1.0 < 1.0 + r )
	r = r / 2.0;
    
    return 2.0 * r;
}


ParallelDTriangulator::ParallelDTriangulator( DAGTriangleTree& dagt )
    : tree_( dagt )  
    , israndom_( true )
{}


bool ParallelDTriangulator::doPrepare( int nrthreads )
{
    if ( nrthreads!=1 )
    {
	pErrMsg( "Not debugged yet." );
	return false;
    }

    const int nrcoords = totalNr();
    if ( israndom_ )
	permutation_.erase();
    else
    {
	int arr[nrcoords];
	for ( int idx=0; idx<nrcoords; idx++ )
	    arr[idx] = idx;

	std::random_shuffle( arr, arr+nrcoords );
	for ( int idx=0; idx<nrcoords; idx++ )
	    permutation_ += arr[idx];
    }
    
    return true;
}


bool ParallelDTriangulator::doWork( int start, int stop, int threadid )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++, reportNrDone(1) )
    {
	const int insertptid = permutation_.size() ? permutation_[idx] : idx;
	int dupid;
       	if ( !tree_.insertPoint( insertptid, dupid, threadid ) )
	    return false;
    }

    return true;
}


DAGTriangleTree::DAGTriangleTree()
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
{}


DAGTriangleTree::DAGTriangleTree( const DAGTriangleTree& b )
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
{
    *this = b;
}


DAGTriangleTree& DAGTriangleTree::operator=( const DAGTriangleTree& b )
{
    epsilon_ = b.epsilon_;
    if ( ownscoordlist_ )
	delete coordlist_;

    if ( b.ownscoordlist_ )
	coordlist_ = b.coordlist_ ? new TypeSet<Coord>( *b.coordlist_ ) : 0;
    else
	coordlist_ = b.coordlist_;

    ownscoordlist_ = b.ownscoordlist_;

    triangles_ = b.triangles_;

    initialcoords_[0] = b.initialcoords_[0];
    initialcoords_[1] = b.initialcoords_[1];
    initialcoords_[2] = b.initialcoords_[2];
    return *this;
}


DAGTriangleTree::~DAGTriangleTree()
{
    if ( ownscoordlist_ && coordlist_ )
	coordlist_->erase();
}


bool DAGTriangleTree::computeCoordRanges( const TypeSet<Coord>& coordlist, 
					  Interval<double>& xrg,
					  Interval<double>& yrg )
{
    if ( !coordlist.size() )
	return false;

    xrg.start = xrg.stop = coordlist[0].x;
    yrg.start = yrg.stop = coordlist[0].y;

    for ( int idx=1; idx<coordlist.size(); idx++ )
    {
	xrg.include( coordlist[idx].x );
	yrg.include( coordlist[idx].y );
    }

    return true;
}


bool DAGTriangleTree::setCoordList( const TypeSet<Coord>& coordlist, bool copy )
{
    coordlock_.writeLock();
    if ( coordlist_ && ownscoordlist_ )
	delete coordlist_;

    coordlist_ = 0;
    coordlock_.writeUnLock();

    if ( coordlist.size()<3 )
	return false;

    Interval<double> xrg, yrg;
    computeCoordRanges( coordlist, xrg, yrg );

    coordlock_.writeLock();

    if ( copy )
    {
	coordlist_ = new TypeSet<Coord>( coordlist );
	ownscoordlist_ = true;
    }
    else
    {
	coordlist_ = const_cast<TypeSet<Coord>* >( &coordlist );
	ownscoordlist_ = false;
    }

    coordlock_.writeUnLock();
    return setBBox( xrg, yrg );
}


bool DAGTriangleTree::setBBox(const Interval<double>& xrg,
			  const Interval<double>& yrg)
{
    triangles_.erase();
    const double xlength = xrg.width();
    const double ylength = yrg.width();
    if ( mIsZero(xlength,1e-4) || mIsZero(ylength,1e-4) )
	return false;

    const Coord center( xrg.center(), yrg.center() );
    const double radius = 2*sqrt( xlength*xlength+ylength*ylength );
    initialcoords_[0] = Coord( center.x-radius*sqrt(3), center.y-radius );
    initialcoords_[1] = Coord( center.x+radius*sqrt(3), center.y-radius );
    initialcoords_[2] = Coord( center.x, center.y+2*radius );

    DAGTriangle initnode;
    initnode.coordindices_[0] = cInitVertex0();
    initnode.coordindices_[1] = cInitVertex1();
    initnode.coordindices_[2] = cInitVertex2();
    triangles_ +=initnode;

    return true;
}


bool DAGTriangleTree::init()
{
    if ( !triangles_.size() ) return false;

    coordlock_.writeLock();
    if ( !coordlist_ )
    {
	coordlist_ = new TypeSet<Coord>;
	ownscoordlist_ = true;
    }
    coordlock_.writeUnLock();

    return true;
}


int DAGTriangleTree::insertPoint( const Coord& coord, int& dupid, 
				   unsigned char threadid )
{
    if ( !ownscoordlist_ )
	return cNoVertex();

    coordlock_.writeLock();
    const int ci = coordlist_->size();
    (*coordlist_) += coord;
    coordlock_.writeUnLock();

    if ( !insertPoint( ci, dupid, threadid ) )
    {
	coordlock_.writeLock();
	if ( coordlist_->size()==ci+1 )
	    coordlist_->remove( ci );
	coordlock_.writeUnLock();

	return cNoVertex();
    }

    return ci;
}


bool DAGTriangleTree::getTriangle( const Coord& coord, int& dupid,
			TypeSet<int>& vertices, unsigned char lockerid )
{
    dupid = cNoVertex();
    vertices.erase();
    if ( !ownscoordlist_ )
	return false;

    coordlock_.writeLock();
    const int ci = coordlist_->size();
    (*coordlist_) += coord;
    coordlock_.writeUnLock();

    int ti0, ti1;
    const char res = searchTriangle( ci, 0, ti0, ti1, dupid, lockerid );
    if ( dupid!=cNoVertex() )
	return true;

    if ( ti0==cNoVertex() && ti1==cNoVertex() )
	return false;

    const int* crds0 = triangles_[ti0].coordindices_;
    const int* crds1 = ti1!=cNoVertex() ? triangles_[ti1].coordindices_ : 0;
    if ( crds0[0]>=0 && crds0[1]>=0 && crds0[2]>=0 )
    {
    	vertices += crds0[0];
    	vertices += crds0[1];
    	vertices += crds0[2];
	return true;
    }
    
    if ( crds1 && crds1[0]>=0 && crds1[1]>=0 && crds1[2]>=0 )
    {
    	vertices += crds1[0];
    	vertices += crds1[1];
    	vertices += crds1[2];
	return true;
    }

   return false;
}


bool DAGTriangleTree::insertPoint( int ci, int& dupid, unsigned char lockerid )
{
    dupid = cNoVertex();
    coordlock_.readLock();
    if ( mIsUdf((*coordlist_)[ci].x) || mIsUdf((*coordlist_)[ci].y) ) 
    {
	coordlock_.readUnLock();
	BufferString msg = "The point ";
	msg += ci;
	msg +=" is not defined!";
	pErrMsg( msg );
	return true; //For undefined point, skip.
    }

    coordlock_.readUnLock();

    int ti0, ti1;
    const char res = searchTriangle( ci, 0, ti0, ti1, dupid, lockerid );
    
    if ( res==cIsInside() || res==cIsOnEdge() )
    {
	int nti0, nti1;
	bool writelocked0 = triangles_[ti0].writeLock( condvar_, lockerid );	
	bool writelocked1 = ti1==-1 ? false :
	    triangles_[ti1].writeLock( condvar_, lockerid );	

	const char nres = searchFurther( ci, ti0, ti1, nti0, nti1, dupid,
					 lockerid );

	if ( nres==cIsInside() || nres==cIsOnEdge() )
	{
	    if ( writelocked0 )	triangles_[ti0].writeUnLock( condvar_ );
	    if ( writelocked1 )	triangles_[ti1].writeUnLock( condvar_ );

	    bool locked0 = triangles_[nti0].writeLock( condvar_, lockerid );
	    bool locked1 = nti1==-1 ? false : 
		triangles_[nti1].writeLock( condvar_, lockerid );

	    if ( nti1==-1 )
		splitTriangleInside( ci, nti0, lockerid );
	    else
		splitTriangleOnEdge( ci, nti0, nti1, lockerid );
	    
	    if ( locked0 ) triangles_[nti0].writeUnLock( condvar_ );
	    if ( locked1 ) triangles_[nti1].writeUnLock( condvar_ );
	    return true;
	}
	else
	{
	    if ( writelocked0 ) triangles_[ti0].writeUnLock( condvar_ );
	    if ( writelocked1 ) triangles_[ti1].writeUnLock( condvar_ );
	    if ( nres==cIsDuplicate() )
		return true;
	    else
	    {
		BufferString msg = "\n Insert point ";
		msg += ci;
		msg += "failed!";
		pErrMsg( msg );
		return false;
	    }
	}
    }
    else if ( res==cIsDuplicate() )
	return true;
    else  
    {
	BufferString msg = "\n Insert point ";
	msg += ci;
	msg += "failed!";
	pErrMsg( msg );
	return false;
    }

    return true;
}


#define mCrd( idx ) \
	(idx>=0 ? (*coordlist_)[idx] : initialcoords_[-idx-2])


char DAGTriangleTree::searchFurther( int ci, int ti0, int ti1, int& nti0, 
				     int& nti1, int& dupid,
				     unsigned char lockid ) 
{
    if ( ti1==-1 )
	return searchTriangle( ci, ti0, nti0, nti1, dupid, lockid );

    nti0 = ti0;
    nti1 = -1;
    if ( searchTriangleOnEdge( ci, ti0, nti0, dupid, lockid ) )
    {
	const bool didlock = triangles_[nti0].readLock( condvar_, lockid );
	const int* crd = triangles_[nti0].coordindices_;
	const int* nbor = triangles_[nti0].neighbors_;

	coordlock_.readLock();

	if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[1]), epsilon_ ) )
	    nti1 = searchChild( crd[0], crd[1], nbor[0], lockid );
	else if ( pointOnEdge2D(mCrd(ci),mCrd(crd[0]),mCrd(crd[2]),epsilon_) )
	    nti1 = searchChild( crd[0], crd[2], nbor[2], lockid );
	else
	    nti1 = searchChild( crd[1], crd[2], nbor[1], lockid );

	if ( didlock ) triangles_[nti0].readUnLock( condvar_ );
	coordlock_.readUnLock();

	return cIsOnEdge();
    }
    else
    {
	return searchTriangle( ci, nti0, nti0, nti1, dupid, lockid );
    }
}


char DAGTriangleTree::searchTriangle( int ci, int startti, int& ti0, int& ti1,
				      int& dupid, unsigned char lockid )
{
    if ( startti<0 )
	startti = 0;

    ti0 = startti;
    ti1 = cNoTriangle();

    bool didlock = triangles_[ti0].readLock( condvar_, lockid );
    const int* children = triangles_[startti].childindices_;
    
    for ( int childidx = 0; childidx<3; childidx++ )
    {
	const int curchild = children[childidx];
	if ( curchild==cNoTriangle() ) continue;

	const char mode = isInside( ci, curchild, dupid  );
	if ( mode==cIsOutside() ) 
	    continue;

	if ( didlock ) triangles_[ti0].readUnLock( condvar_ );
	if ( mode==cIsDuplicate() )
	    return mode;

	didlock = triangles_[curchild].readLock( condvar_, lockid );
	
	if ( mode==cIsInside() )
	{
	    children = triangles_[curchild].childindices_;
	    startti = curchild;
	    ti0 = curchild;
	    childidx = cNoTriangle();
	}
	else if ( mode==cIsOnEdge() )
	{
	    if ( !searchTriangleOnEdge( ci, curchild, ti0, dupid, lockid ) )
	    {
		if ( didlock ) triangles_[curchild].readUnLock( condvar_ );
		if ( ti0==cNoTriangle() ) return cError();

		didlock = triangles_[ti0].readLock( condvar_, lockid );
		
		children = triangles_[ti0].childindices_;
		childidx = cNoTriangle();
	    }
	    else
	    {
		if ( didlock ) triangles_[curchild].readUnLock( condvar_ );
		didlock = triangles_[ti0].readLock( condvar_, lockid );

		const int* crd = triangles_[ti0].coordindices_;
		const int* nbor = triangles_[ti0].neighbors_;
		coordlock_.readLock();
		if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[1]),
			    epsilon_ ) )
		    ti1 = searchChild( crd[0], crd[1], nbor[0], lockid );
		else if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[2]),
			    epsilon_ ) )
		    ti1 = searchChild( crd[0], crd[2], nbor[2], lockid );
		else
		    ti1 = searchChild( crd[1], crd[2], nbor[1], lockid );
		coordlock_.readUnLock();

		if ( didlock ) triangles_[ti0].readUnLock( condvar_ );
		return cIsOnEdge();
	    }
	}
    }

    if ( ti0==ti1 )
	ti1 = cNoTriangle();

    if ( didlock ) triangles_[ti0].readUnLock( condvar_ );
    if ( ti0==cNoTriangle() )
    {
	pErrMsg("Initial triangle is wrong");
	return cError();
    }

    return cIsInside();
}


bool DAGTriangleTree::searchTriangleOnEdge( int ci, int ti, int& resti,
					    int& dupid, unsigned char lockid ) 
{
    bool didlock = triangles_[ti].readLock( condvar_, lockid );
    const int* child = triangles_[ti].childindices_;
    if ( child[0]==cNoTriangle() && child[1]==cNoTriangle() &&
	 child[2]==cNoTriangle() )
    {
	resti = ti;
	if ( didlock ) triangles_[ti].readUnLock( condvar_ );

	return true;
    }

    for ( int idx=0; idx<3; idx++ )
    {
    	const char inchild = child[idx]!=cNoTriangle()
	    ? isInside(ci,child[idx],dupid)
	    : cNoTriangle();
    	if ( inchild==cIsOnEdge() )
	{
	    if ( didlock ) triangles_[ti].readUnLock( condvar_ );
	    return searchTriangleOnEdge( ci, child[idx], resti, dupid, lockid );
	}
	else if ( inchild==cIsInside() )
	{
	    resti = child[idx];
	    if ( didlock ) triangles_[ti].readUnLock( condvar_ );

	    return false;
	}
    }

    pErrMsg( "This should never happen." );
    resti = cNoTriangle();
    if ( didlock ) triangles_[ti].readUnLock( condvar_ );

    return false;
}


char DAGTriangleTree::isOnEdge( const Coord& p, const Coord& a, 
				const Coord& b, bool& duponfirst ) const
{
    const Coord pa = p-a;
    const Coord pb = p-b;
    const Coord ba = b-a;
    if ( mIsZero(ba.x, epsilon_) ) //AB is parallel to y-axis.
    {
	if ( fabs(pa.x)>epsilon_ )
	    return cNotOnEdge();
	else 
	{
	    if ( b.y>a.y && (pb.y>0 || pa.y<0) || 
		 b.y<a.y && (pb.y<0 || pa.y>0) )
		return cNotOnEdge();
	    else if mIsZero( pb.y, epsilon_ )
	    {
		duponfirst = false;
		return cIsDuplicate();
	    }
	    else if mIsZero( pa.y, epsilon_ )
	    {
		duponfirst = true;
		return cIsDuplicate();
	    }

	    return cIsOnEdge();
	}
    }

    const double slope = ba.y/ba.x;
    const double yonline = a.y+slope*pa.x;
    if ( fabs(yonline-p.y)>epsilon_ || a.x<b.x && (pa.x<0 || pb.x>0) ||
	 b.x<a.x && (pb.x<0 || pa.x>0) )
	return cNotOnEdge();
    else if ( mIsZero(pa.x,epsilon_) )
    {
	duponfirst = true;
	return cIsDuplicate();
    }
    else if ( mIsZero(pb.x,epsilon_) )
    {
	duponfirst = false;
	return cIsDuplicate();
    }

    return cIsOnEdge();
}


char DAGTriangleTree::isInside( int ci, int ti, int& dupid ) const
{
    if ( ti==cNoTriangle() )
	return cIsOutside();

    const int* crds = triangles_[ti].coordindices_;
    coordlock_.readLock();
    const Coord& coord = mCrd(ci);
    const Coord& tricoord0 = mCrd(crds[0]);
    const Coord& tricoord1 = mCrd(crds[1]);
    const Coord& tricoord2 = mCrd(crds[2]);
    if ( pointInTriangle2D( coord, tricoord0, tricoord1, tricoord2, 0 ) )
    {
	coordlock_.readUnLock();
	return cIsInside();
    }

    bool duponfirst;
    char res = isOnEdge( coord, tricoord0, tricoord1, duponfirst );
    if ( res!=cNotOnEdge() )
    {
	if ( res==cIsDuplicate() ) dupid = duponfirst ? crds[0] : crds[1];
	coordlock_.readUnLock();
	return res;
    }

    res = isOnEdge( coord, tricoord1, tricoord2, duponfirst );
    if ( res!=cNotOnEdge() )
    {
	if ( res==cIsDuplicate() ) dupid = duponfirst ? crds[1] : crds[2];
	coordlock_.readUnLock();
	return res;
    }

    res = isOnEdge( coord, tricoord2, tricoord0, duponfirst );
    coordlock_.readUnLock();

    if ( res!=cNotOnEdge() )
    {
	if ( res==cIsDuplicate() ) dupid = duponfirst ? crds[2] : crds[0];
	return res;
    }

    return cIsOutside();
}


void DAGTriangleTree::splitTriangleInside( int ci, int ti,unsigned char lockid )
{
    if ( ti<0 || ti>=triangles_.size() )
	return;

    const int crd0 = triangles_[ti].coordindices_[0];
    const int crd1 = triangles_[ti].coordindices_[1];
    const int crd2 = triangles_[ti].coordindices_[2];

    bool didlock = triangles_[ti].writeLock( condvar_, lockid );
    
    DAGTriangle triangle;
    triangle.coordindices_[0] = crd0;
    triangle.coordindices_[1] = crd1;
    triangle.coordindices_[2] = ci;
    triangles_ += triangle;
    const int ti0 = triangles_.size()-1;

    triangle.coordindices_[0] = crd0;
    triangle.coordindices_[1] = ci; 
    triangle.coordindices_[2] = crd2;
    triangles_ += triangle;
    const int ti1 = triangles_.size()-1;
    
    triangle.coordindices_[0] = ci; 
    triangle.coordindices_[1] = crd1;
    triangle.coordindices_[2] = crd2;
    triangles_ += triangle;
    const int ti2 = triangles_.size()-1;

    triangles_[ti].childindices_[0] = ti0;
    triangles_[ti].childindices_[1] = ti1;
    triangles_[ti].childindices_[2] = ti2;

    bool lockti0 = triangles_[ti0].writeLock( condvar_, lockid );
    bool lockti1 = triangles_[ti1].writeLock( condvar_, lockid );
    bool lockti2 = triangles_[ti2].writeLock( condvar_, lockid );
    const int* nbti = triangles_[ti].neighbors_;
    triangles_[ti0].neighbors_[0] = searchChild( crd0, crd1,nbti[0], lockid );
    triangles_[ti0].neighbors_[1] = ti2;
    triangles_[ti0].neighbors_[2] = ti1;
    triangles_[ti1].neighbors_[0] = ti0;
    triangles_[ti1].neighbors_[1] = ti2;
    triangles_[ti1].neighbors_[2] = searchChild( crd2, crd0, nbti[2], lockid );
    triangles_[ti2].neighbors_[0] = ti0;
    triangles_[ti2].neighbors_[1] = searchChild( crd1, crd2, nbti[1], lockid );
    triangles_[ti2].neighbors_[2] = ti1;
    
    if ( didlock ) triangles_[ti].writeUnLock( condvar_ );
    if ( lockti0 ) triangles_[ti0].writeUnLock( condvar_ );
    if ( lockti1 ) triangles_[ti1].writeUnLock( condvar_ );
    if ( lockti2 ) triangles_[ti2].writeUnLock( condvar_ );
    
    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 1; tis += ti0;
    v0s += 0; v1s += 2; tis += ti1;
    v0s += 1; v1s += 2; tis += ti2;

    legalizeTriangles( v0s, v1s, tis, lockid );
}


const int DAGTriangleTree::getNeighbor( const int v0, const int v1, 
					const int ti, unsigned char lockid )
{
    if ( ti==cNoTriangle() )
	return cNoVertex();

    const int* crds = triangles_[ti].coordindices_;
    int id0=-1;
    int id1=-1;
    for ( int idx=0; idx<3; idx++ )
    {
	if ( crds[idx]==v0 )
	    id0 = idx;
	
	if ( crds[idx]==v1 )
	    id1 = idx;
    }

    if ( id0==-1 || id1==-1 )
    {
	pErrMsg( "vertex is not on the triangle" );
	return cNoVertex();
    }

    bool didlock = triangles_[ti].readLock( condvar_, lockid );
    int res;
    
    if ( id0==0 && id1==1 || id0==1 && id1==0 )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[0], lockid );
    if ( id0==0 && id1==2 || id0==2 && id1==0 )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[2], lockid );
    else if ( id0==1 && id1==2 || id0==2 && id1==1 )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[1], lockid );

    if ( didlock ) triangles_[ti].readUnLock( condvar_ );
    return res;
}


void DAGTriangleTree::splitTriangleOnEdge( int ci, int ti0, int ti1, 
					   unsigned char lockid )
{
    if ( ti0==cNoTriangle() || ti1==cNoTriangle() )
	return; 

    const int* crds0 = triangles_[ti0].coordindices_;
    const int* crds1 = triangles_[ti1].coordindices_;
    int shared0 = cNoVertex();
    int shared1 = cNoVertex();
    int vti0 = cNoVertex();
    int vti1 = cNoVertex();

    for ( int idx=0; idx<3; idx++ )
    {
	if ( crds0[idx]!=crds1[0] && crds0[idx]!=crds1[1] && 
		crds0[idx]!=crds1[2] )
	{
	    vti0 = crds0[idx];
	    if ( idx==0 )
	    {
		shared0 = crds0[1];
		shared1 = crds0[2];
	    }
	    else if ( idx==1 )
	    {           
		shared0 = crds0[0];
		shared1 = crds0[2];
	    }
	    else
	    {
		shared0 = crds0[0];
		shared1 = crds0[1]; 
	    }
	}

	if ( crds1[idx]!=crds0[0] && crds1[idx]!=crds0[1] &&
		crds1[idx]!=crds0[2] )
	    vti1 = crds1[idx];
    }

    if ( shared0==cNoVertex() || shared1==cNoVertex() ||
	 vti0==cNoVertex() || vti1==cNoVertex() )
    {
	pErrMsg( "Two triangles don't share edge" );
	return;
    }

    bool didlock0 = triangles_[ti0].writeLock( condvar_, lockid );
    bool didlock1 = triangles_[ti1].writeLock( condvar_, lockid );
    
    DAGTriangle triangle;
    triangle.coordindices_[0] = shared0;
    triangle.coordindices_[1] = ci;
    triangle.coordindices_[2] = vti0;
    triangles_ += triangle;
    const int nti0 = triangles_.size()-1;

    triangle.coordindices_[0] = shared1;
    triangle.coordindices_[1] = vti0;
    triangle.coordindices_[2] = ci; 
    triangles_ += triangle;
    const int nti1 = triangles_.size()-1;

    triangle.coordindices_[0] = shared0;
    triangle.coordindices_[1] = vti1;
    triangle.coordindices_[2] = ci; 
    triangles_ += triangle;
    const int nti2 = triangles_.size()-1;

    triangle.coordindices_[0] = shared1;
    triangle.coordindices_[1] = ci;;
    triangle.coordindices_[2] = vti1; 
    triangles_ += triangle;
    const int nti3 = triangles_.size()-1;

    triangles_[ti0].childindices_[0] = nti0;
    triangles_[ti0].childindices_[1] = nti1;
    triangles_[ti1].childindices_[0] = nti2;
    triangles_[ti1].childindices_[1] = nti3;

    bool locknti0 = triangles_[nti0].writeLock( condvar_, lockid );
    bool locknti1 = triangles_[nti1].writeLock( condvar_, lockid );
    bool locknti2 = triangles_[nti2].writeLock( condvar_, lockid );
    bool locknti3 = triangles_[nti3].writeLock( condvar_, lockid );
    triangles_[nti0].neighbors_[0] = nti2;
    triangles_[nti0].neighbors_[1] = nti1;
    triangles_[nti0].neighbors_[2] = getNeighbor(vti0,shared0,ti0,lockid);
    triangles_[nti1].neighbors_[0] = getNeighbor(shared1,vti0,ti0,lockid);
    triangles_[nti1].neighbors_[1] = nti0;
    triangles_[nti1].neighbors_[2] = nti3;
    triangles_[nti2].neighbors_[0] = getNeighbor(shared0,vti1,ti1,lockid);
    triangles_[nti2].neighbors_[1] = nti3;
    triangles_[nti2].neighbors_[2] = nti0;
    triangles_[nti3].neighbors_[0] = nti1;
    triangles_[nti3].neighbors_[1] = nti2;
    triangles_[nti3].neighbors_[2] = getNeighbor(vti1,shared1,ti1,lockid);
    
    if ( didlock0 ) triangles_[ti0].writeUnLock( condvar_ );
    if ( didlock1 ) triangles_[ti1].writeUnLock( condvar_ );
    
    if ( locknti0 ) triangles_[nti0].writeUnLock( condvar_ );
    if ( locknti1 ) triangles_[nti1].writeUnLock( condvar_ );
    if ( locknti2 ) triangles_[nti2].writeUnLock( condvar_ );
    if ( locknti3 ) triangles_[nti3].writeUnLock( condvar_ );
 
    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 2; tis += nti0;
    v0s += 0; v1s += 1; tis += nti1;
    v0s += 0; v1s += 1; tis += nti2;
    v0s += 0; v1s += 2; tis += nti3;

    legalizeTriangles( v0s, v1s, tis, lockid );
}


void DAGTriangleTree::legalizeTriangles( TypeSet<char>& v0s, TypeSet<char>& v1s, 				TypeSet<int>& tis, unsigned char lockid )
{
    int start = 0;
    while ( v0s.size()>start )
    {
	const char v0 = v0s[start]; 
	const char v1 = v1s[start];
	const int ti = tis[start];
	if ( ti<0 )
	    continue;
	
	if ( start>10000 )
	{
	    v0s.remove( 0, start );
	    v1s.remove( 0, start );
	    tis.remove( 0, start );
	    start = 0;
	}
	else
  	    start++;

	int ci;
	int checkti = cNoTriangle();
	bool readlockti = triangles_[ti].readLock( condvar_, lockid );
	if ( v0==0 && v1==1 || v0==1 && v1==0 )
	{
	    checkti = triangles_[ti].neighbors_[0];
	    ci = 2;
	}
	else if ( v0==0 && v1==2 || v0==2 && v1==0 )
	{
	    checkti = triangles_[ti].neighbors_[2];
	    ci = 1;
	}
	else if ( v0==1 && v1==2 || v0==2 && v1==1 )
	{
	    checkti = triangles_[ti].neighbors_[1];
	    ci = 0;
	}
	
	if ( readlockti ) triangles_[ti].readUnLock( condvar_ );
	if ( checkti==cNoTriangle() )
	    continue;

	const int shared0 = triangles_[ti].coordindices_[v0];
	const int shared1 = triangles_[ti].coordindices_[v1];
	const int crdci = triangles_[ti].coordindices_[ci];
	const int* checkcrds = triangles_[checkti].coordindices_;

	int checkpt =cNoVertex();
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( checkcrds[idx] != shared0 && checkcrds[idx] != shared1 )
	    {
		checkpt = checkcrds[idx];
		break;
	    }
	}

	if ( checkpt==cNoVertex() )
	{
	    pErrMsg("Impossible case.");
	    continue;
	}

	coordlock_.readLock();
	if ( !isInsideCircle(mCrd(checkpt), mCrd(crdci), mCrd(shared0),
		    mCrd(shared1)) || checkpt==crdci )
	{
	    coordlock_.readUnLock();
	    continue;
	}

	coordlock_.readUnLock();
	
	bool writelockti = triangles_[ti].writeLock(condvar_,lockid); 
	bool writelockchkti = triangles_[checkti].writeLock(condvar_,lockid);
   
	DAGTriangle triangle;
	triangle.coordindices_[0] = crdci;
	triangle.coordindices_[1] = checkpt;
	triangle.coordindices_[2] = shared0; 
	triangles_ += triangle;
	const int nti1 = triangles_.size()-1;

	triangle.coordindices_[0] = shared1;
	triangle.coordindices_[1] = checkpt;
	triangle.coordindices_[2] = crdci; 
	triangles_ += triangle;
	const int nti2 = triangles_.size()-1;

	triangles_[ti].childindices_[0] = nti1;
	triangles_[ti].childindices_[1] = nti2;
	triangles_[checkti].childindices_[0] = nti1;
	triangles_[checkti].childindices_[1] = nti2;

	bool locknti1 = triangles_[nti1].writeLock( condvar_, lockid );
	bool locknti2 = triangles_[nti2].writeLock( condvar_, lockid );
	triangles_[nti1].neighbors_[0] = nti2;
	triangles_[nti1].neighbors_[1] = getNeighbor(checkpt,shared0,
						     checkti, lockid );
	triangles_[nti1].neighbors_[2] = getNeighbor(shared0,crdci,ti,lockid);
	triangles_[nti2].neighbors_[0] = getNeighbor(shared1,checkpt,
						     checkti,lockid);
	triangles_[nti2].neighbors_[1] = nti1;
	triangles_[nti2].neighbors_[2] = getNeighbor(crdci,shared1,ti,lockid);

	if ( writelockchkti ) triangles_[checkti].writeUnLock( condvar_ );
	if ( writelockti ) triangles_[ti].writeUnLock( condvar_ );
	if ( locknti1 ) triangles_[nti1].writeUnLock( condvar_ );
       	if ( locknti2 ) triangles_[nti2].writeUnLock( condvar_ );

	v0s += 1; v1s += 2; tis += nti1;
	v0s += 0; v1s += 1; tis += nti2;
    }
}


bool DAGTriangleTree::getCoordIndices( TypeSet<int>& result ) const
{
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]>=0 || child[1]>=0 || child[2]>=0 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	if ( c[0]<0 || c[1]<0 || c[2]<0 )
	    continue;
	
	result += c[0];
	result += c[1];
	result += c[2];
    }

    return result.size();
}


bool DAGTriangleTree::getConnections( int vertex, TypeSet<int>& result ) const
{
    result.erase();
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]>=0 || child[1]>=0 || child[2]>=0 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	if ( c[0]!=vertex && c[1]!=vertex && c[2]!=vertex )
	    continue;

	for ( int idy=0; idy<3; idy++ )
	{
	    if ( c[idy]<0 || c[idy]==vertex || result.indexOf(c[idy])!=-1 )
		continue;

	    result += c[idy];
	}
    }

    return result.size();
}


void DAGTriangleTree::dumpTo(std::ostream& stream) const
{
    for ( int idx=0; idx<triangles_.size(); idx++ )
    {
	const DAGTriangle& triangle = triangles_[idx];
	stream << (int) triangle.coordindices_[0] << '\t'
	       << (int) triangle.coordindices_[1] << '\t'
	       << (int) triangle.coordindices_[2] << '\n';
    }
}




#define mSearch( child ) \
if ( child!=cNoTriangle() ) \
{ \
    const int* gc = triangles_[child].coordindices_; \
    if ( gc[0]==v0 && gc[1]==v1 || gc[1]==v0 && gc[0]==v1 || \
	 gc[0]==v0 && gc[2]==v1 || gc[2]==v0 && gc[0]==v1 || \
	 gc[1]==v0 && gc[2]==v1 || gc[2]==v0 && gc[1]==v1 )  \
    { \
	const int res = searchChild( v0, v1, child, lockid ); \
	if ( didlock ) triangles_[ti].readUnLock( condvar_ ); \
	return res; \
    } \
}


int DAGTriangleTree::searchChild( int v0, int v1, int ti, unsigned char lockid) 
{
    if ( ti==cNoTriangle() )
	return cError();

    bool didlock = triangles_[ti].readLock( condvar_, lockid );
    const int* child = triangles_[ti].childindices_;

    if ( child[0]==cNoTriangle() && child[1]==cNoTriangle() &&
	 child[2]==cNoTriangle() )
    {
	if ( didlock ) triangles_[ti].readUnLock( condvar_ );
	return ti;
    }

    for ( int idx=0; idx<3; idx++ )
	mSearch( child[idx] );

    if ( didlock ) triangles_[ti].readUnLock( condvar_ );
    return cError();
}


DAGTriangleTree::DAGTriangle::DAGTriangle()
{
    coordindices_[0] = DAGTriangleTree::cNoVertex();
    coordindices_[1] = DAGTriangleTree::cNoVertex();
    coordindices_[2] = DAGTriangleTree::cNoVertex();
    childindices_[0] = DAGTriangleTree::cNoTriangle();
    childindices_[1] = DAGTriangleTree::cNoTriangle();
    childindices_[2] = DAGTriangleTree::cNoTriangle();
    neighbors_[0] = DAGTriangleTree::cNoTriangle();
    neighbors_[1] = DAGTriangleTree::cNoTriangle();
    neighbors_[2] = DAGTriangleTree::cNoTriangle();
    lockcounts_ = 0;
}


bool DAGTriangleTree::DAGTriangle::readLock( Threads::ConditionVar& cv,
       					     unsigned char lockerid )
{
    return false; //only works for one processor now, to be removed.
    const char mylockcount = -lockerid-1;
    cv.lock();
    if ( lockcounts_==mylockcount )
    {
	cv.unLock();
	return false;
    }

    while ( lockcounts_<0 )
	cv.wait();

    lockcounts_++;
    cv.unLock();
    return true;
}


void DAGTriangleTree::DAGTriangle::readUnLock( Threads::ConditionVar& cv )
{
    cv.lock();
    if ( lockcounts_<=0 )
	pErrMsg( "Error!" );

    lockcounts_--;
    if ( !lockcounts_ ) cv.signal( false );
    cv.unLock();
}


bool DAGTriangleTree::DAGTriangle::writeLock( Threads::ConditionVar& cv,
       					      unsigned char lockerid )
{
    return false; //only works for one processor now, to be removed.
    cv.lock();
    const char mylockcount = -lockerid-1;
    if ( lockcounts_==mylockcount )
    {
	cv.unLock();
	return false;
    }

    while ( lockcounts_!=0 )
	cv.wait();

    lockcounts_ = mylockcount;
    cv.unLock();
    return true;
}
    

void DAGTriangleTree::DAGTriangle::writeUnLock( Threads::ConditionVar& cv )
{
    cv.lock();
    if ( lockcounts_>=0 )
    	pErrMsg( "Error!" );

    lockcounts_ = 0;
    cv.signal( true );
    cv.unLock();
}


bool DAGTriangleTree::DAGTriangle::operator==( 
	const DAGTriangle& dag ) const
{
    const int d0 = dag.coordindices_[0];
    const int d1 = dag.coordindices_[1];
    const int d2 = dag.coordindices_[2];
    return d0==coordindices_[0] && d1==coordindices_[1] && d2==coordindices_[2]
	|| d0==coordindices_[0] && d1==coordindices_[2] && d2==coordindices_[1]
	|| d0==coordindices_[1] && d1==coordindices_[0] && d2==coordindices_[2] 	|| d0==coordindices_[1] && d1==coordindices_[2] && d2==coordindices_[0] 	|| d0==coordindices_[2] && d1==coordindices_[1] && d2==coordindices_[0] 	|| d0==coordindices_[2] && d1==coordindices_[0] && d2==coordindices_[1];
}


DAGTriangleTree::DAGTriangle&
DAGTriangleTree::DAGTriangle::operator=( const DAGTriangleTree::DAGTriangle& b )
{
    for ( int idx=0; idx<3; idx++ )
    {
	coordindices_[idx] = b.coordindices_[idx];
	childindices_[idx] = b.childindices_[idx];
	neighbors_[idx] = b.neighbors_[idx];
    }

    return *this;
}
