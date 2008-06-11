/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: delaunay.cc,v 1.6 2008-06-11 08:53:11 cvsbert Exp $";

#include "delaunay.h"

#include "positionlist.h"
#include "sorting.h"
#include "trigonometry.h"


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


ParallelDelaunayTriangulator::ParallelDelaunayTriangulator(
	const TypeSet<Coord>& coordlist )
    : ParallelTask( "Delaunay Triangulator" )
    , dagtritree_( coordlist )
    , nrcoords_( coordlist.size() )
    , israndom_( false )
{}


bool ParallelDelaunayTriangulator::doPrepare( int )
{
    if ( israndom_ )
	permutation_.erase();
    else
    {
	int arr[nrcoords_];
	for ( int idx=0; idx<nrcoords_; idx++ )
	    arr[idx] = idx;

	std::random_shuffle( arr, arr+nrcoords_ );
	for ( int idx=0; idx<nrcoords_; idx++ )
	    permutation_ += arr[idx];
    }
    
    return true;
}


bool ParallelDelaunayTriangulator::doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++, reportNrDone(1) )
    {
	if ( !dagtritree_.insertPoint( permutation_.size() ? permutation_[idx] 
		    					   : idx ) )
	    return false;
    }

    return true;
}


bool ParallelDelaunayTriangulator::getCoordIndices( TypeSet<int>& res ) const
{
    return dagtritree_.getCoordIndices(res);
}


DAGTriangleTree::DAGTriangleTree( const TypeSet<Coord>& coordlist )
    : coordlist_( &coordlist )
    , epsilon_( 1e-5 )
{
    Coord min = (*coordlist_)[0];
    Coord max = (*coordlist_)[0];
    for ( int idx=1; idx<(*coordlist_).size(); idx++ )
    {
	if ( (*coordlist_)[idx].x < min.x )
	    min.x = (*coordlist_)[idx].x;

	if ( (*coordlist_)[idx].y < min.y )
	    min.y = (*coordlist_)[idx].y;

	if ( (*coordlist_)[idx].x > max.x )
	    max.x = (*coordlist_)[idx].x;

	if ( (*coordlist_)[idx].y > max.y )
	    max.y = (*coordlist_)[idx].y;
    }

    Coord center = (min+max)/2;
    const float initialk = 3 * (max.x-min.x) * (max.y-min.y);
    initialcoords_[0] = Coord( 0, initialk )+center;
    initialcoords_[1] = Coord( initialk, 0 )+center;
    initialcoords_[2] = Coord( -initialk, -initialk )+center;

    DAGTriangle initnode;
    initnode.coordindices_[0] = -2;
    initnode.coordindices_[1] = -3;
    initnode.coordindices_[2] = -4;
    triangles_ +=initnode;
}


bool DAGTriangleTree::insertPoint( int ci )
{
    if ( mIsUdf((*coordlist_)[ci].x) || mIsUdf((*coordlist_)[ci].y) ) 
    {
	BufferString msg = "The point ";
	msg += ci;
	msg +=" is not defined!";
	pErrMsg( msg );
	return true; //For undefined point, skip.
    }

    int ti0, ti1;
    rwlock_.readLock();
    const char res = searchTriangle( ci, 0, ti0, ti1 );
    
    if ( res==1 )
    {
	int nti0, nti1;
	if ( rwlock_.convToWriteLock() )
	{
	    if ( ti1==-1 )
		splitTriangleInside( ci, ti0 );
	    else
		splitTriangleOnEdge( ci, ti0, ti1 );

	    rwlock_.writeUnLock();
	    return true;
	}
	else
	{
	    const int nres = searchFurther( ci, ti0, ti1, nti0, nti1 );
	    if ( nres==1 )
	    {
		if ( nti1==-1 )
		    splitTriangleInside( ci, nti0 );
		else
		    splitTriangleOnEdge( ci, nti0, nti1 );

		rwlock_.writeUnLock();
		return true;
	    }
	    else
	    {
		rwlock_.writeUnLock();
		if ( !nres )
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
    }
    else 
    {
	rwlock_.readUnLock();
	if ( !res )
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

    return true;
}


#define mCrd( idx ) \
	(idx>=0 ? (*coordlist_)[idx] : initialcoords_[-idx-2])


char DAGTriangleTree::searchFurther( int ci, int ti0, int ti1, 
				     int& nti0, int& nti1 ) const
{
    if ( ti1==-1 )
	return searchTriangle( ci, ti0, nti0, nti1 );

    nti0 = ti0;
    nti1 = -1;
    if ( searchTriangleOnEdge( ci, ti0, nti0 ) )
    {
	const int* crd = triangles_[nti0].coordindices_;
	const int* nbor = triangles_[nti0].neighbors_;
	if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[1]), epsilon_ ) )
	    nti1 = searchChild( crd[0], crd[1], nbor[0] );
	else if ( pointOnEdge2D(mCrd(ci),mCrd(crd[0]),mCrd(crd[2]),epsilon_) )
	    nti1 = searchChild( crd[0], crd[2], nbor[2] );
	else
	    nti1 = searchChild( crd[1], crd[2], nbor[1] );

	return 1;
    }
    else
    {
	return searchTriangle( ci, nti0, nti0, nti1 );
    }
}


char DAGTriangleTree::searchTriangle( int ci, int startti, 
				      int& ti0, int& ti1 ) const
{
    if ( startti<0 )
	startti = 0;

    ti0 = startti;
    ti1 = -1;
    const int* children = triangles_[startti].childindices_;
    for ( int childidx = 0; childidx<3; childidx++ )
    {
	const int curchild = children[childidx];
	if ( curchild==-1 )
	    continue;

	const char mode = isInside( ci, curchild );
	if ( mode==-1 )
	    continue;
	
	if ( mode )
	{
	    ti0 = curchild;
	    children = triangles_[curchild].childindices_;
	    childidx = -1;
	}
	else
	{
	    const int* crds = triangles_[curchild].coordindices_;
	    const Coord c0 = mCrd(ci)-mCrd(crds[0]);
	    const Coord c1 = mCrd(ci)-mCrd(crds[1]);
	    const Coord c2 = mCrd(ci)-mCrd(crds[2]);

	    if ( mIsZero( c0.x*c0.x+c0.y*c0.y, epsilon_ ) ||
		 mIsZero( c1.x*c1.x+c1.y*c1.y, epsilon_ ) ||
   		 mIsZero( c2.x*c2.x+c2.y*c2.y, epsilon_ ) )
	    return 0; //ci is the same as one of the inserted points, skip.

	    if ( !searchTriangleOnEdge( ci, curchild, ti0 ) )
	    {
		if ( ti0==-1 )
		    return -1;

		children = triangles_[ti0].childindices_;
		childidx = -1;
	    }
	    else
	    {
		const int* crd = triangles_[ti0].coordindices_;
		const int* nbor = triangles_[ti0].neighbors_;
		if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[1]),
			    epsilon_ ) )
		    ti1 = searchChild( crd[0], crd[1], nbor[0] );
		else if ( pointOnEdge2D( mCrd(ci), mCrd(crd[0]), mCrd(crd[2]),
			    epsilon_ ) )
		    ti1 = searchChild( crd[0], crd[2], nbor[2] );
		else
		    ti1 = searchChild( crd[1], crd[2], nbor[1] );

		return 1;
	    }
	}
    }

    if ( ti0==ti1 )
	ti1 = -1;

    if ( ti0==-1 )
    {
	pErrMsg("Initial triangle is wrong");
	return -1;
    }

    return 1;
}


bool DAGTriangleTree::searchTriangleOnEdge( int ci, int ti, int& resti ) const
{
    const int* child = triangles_[ti].childindices_;
    if ( child[0]==-1 && child[1]==-1 && child[2]==-1 )
    {
	resti = ti;
	return true;
    }

    for ( int idx=0; idx<3; idx++ )
    {
    	const int inchild = child[idx]!=-1 ? isInside(ci,child[idx]) : -1;
    	if ( !inchild )
    	    return searchTriangleOnEdge( ci, child[idx], resti );
	else if ( inchild==1 )
	{
	    resti = child[idx];
	    return false;
	}
    }

    pErrMsg( "This should never happen." );
    resti = -1;
    return false;
}


char DAGTriangleTree::isInside( int ci, int ti ) const
{
    if ( ti==-1 )
	return -1;

    const int* crds = triangles_[ti].coordindices_;
    if ( pointInTriangle2D(mCrd(ci),mCrd(crds[0]),mCrd(crds[1]),mCrd(crds[2]),
         mDefEps) )
	return 1;

    if ( pointOnEdge2D( mCrd(ci), mCrd(crds[0]), mCrd(crds[1]), epsilon_ ) || 
	 pointOnEdge2D( mCrd(ci), mCrd(crds[1]), mCrd(crds[2]), epsilon_ ) ||
	 pointOnEdge2D( mCrd(ci), mCrd(crds[0]), mCrd(crds[2]), epsilon_ ) )
	return 0;

    return -1;
}


void DAGTriangleTree::splitTriangleInside( int ci, int ti )
{
    if ( ti<0 || ti>=triangles_.size() )
	return;

    const int crd0 = triangles_[ti].coordindices_[0];
    const int crd1 = triangles_[ti].coordindices_[1];
    const int crd2 = triangles_[ti].coordindices_[2];

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

    const int* nbti = triangles_[ti].neighbors_;
    triangles_[ti0].neighbors_[0] = searchChild( crd0, crd1,nbti[0] );
    triangles_[ti0].neighbors_[1] = searchChild( crd1, ci, ti2 );
    triangles_[ti0].neighbors_[2] = searchChild( ci, crd0, ti1 );
    triangles_[ti1].neighbors_[0] = searchChild( crd0, ci, ti0 );
    triangles_[ti1].neighbors_[1] = searchChild( ci, crd2, ti2 );
    triangles_[ti1].neighbors_[2] = searchChild( crd2, crd0, nbti[2] );
    triangles_[ti2].neighbors_[0] = searchChild( ci, crd1, ti0 );
    triangles_[ti2].neighbors_[1] = searchChild( crd1, crd2, nbti[1] );
    triangles_[ti2].neighbors_[2] = searchChild( crd2, ci, ti1 );
    
    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 1; tis += ti0;
    v0s += 0; v1s += 2; tis += ti1;
    v0s += 1; v1s += 2; tis += ti2;
    
    legalizeTriangles( v0s, v1s, tis );
}

void DAGTriangleTree::setNeighbors( const int& vetexidx, const int& crd, 
				    const int& ti, int& nb0, int& nb1 ) 
{ 
    const int* crds = triangles_[ti].coordindices_;
    const int* nbs = triangles_[ti].neighbors_;
    if ( vetexidx==0 ) 
    { 
	if ( crd==crds[1] ) 
	{ 
	    nb0 = nbs[0]; 
	    nb1 = nbs[2]; 
	} 
	else if ( crd==crds[2] ) 
	{ 
	    nb0 = nbs[2]; 
	    nb1 = nbs[0]; 
	} 
    } 
    else if ( vetexidx==1 ) 
    { 
 	if ( crd==crds[0] ) 
	{ 
	    nb0 = nbs[0]; 
	    nb1 = nbs[1]; 
	} 
	else if ( crd==crds[2] ) 
	{ 
	    nb0 = nbs[1]; 
	    nb1 = nbs[0]; 
	} 
    } 
    else if ( vetexidx==2 ) 
    { 
	if ( crd==crds[1] ) 
	{ 
	    nb0 = nbs[1]; 
	    nb1 = nbs[2]; 
	} 
	else if ( crd==crds[0] ) 
	{ 
	    nb0 = nbs[2]; 
	    nb1 = nbs[1]; 
	} 
    } 
}

void DAGTriangleTree::splitTriangleOnEdge( int ci, int ti0, int ti1 )
{
    if ( ti0==-1 || ti1==-1 )
	return; 

    const int* crds0 = triangles_[ti0].coordindices_;
    const int* nb0 = triangles_[ti0].neighbors_;
    int shared0, shared1;
    int vti0;
    int vti1 = -1;
    int nbti00, nbti01;
    if ( pointOnEdge2D( mCrd(ci), mCrd(crds0[0]), mCrd(crds0[1]), epsilon_ ) )
    {
	shared0 = crds0[0];
	shared1 = crds0[1];
	vti0 = crds0[2];
	nbti00 = nb0[2];
	nbti01 = nb0[1];
    }
    else if ( pointOnEdge2D(mCrd(ci),mCrd(crds0[1]),mCrd(crds0[2]),epsilon_) )
    {
	shared0 = crds0[1];
	shared1 = crds0[2];
	vti0 = crds0[0];
	nbti00 = nb0[0];
	nbti01 = nb0[2];
    }
    else
    {
	shared0 = crds0[2];
	shared1 = crds0[0];
	vti0 = crds0[1];
	nbti00 = nb0[1];
	nbti01 = nb0[0];
    }

    const int* crds1 = triangles_[ti1].coordindices_;
    int vetexidx;
    if ( crds1[0] != shared0 && crds1[0] != shared1 )
    {
	vti1 = crds1[0];
	vetexidx = 0;
    }
    else if ( crds1[1] != shared0 && crds1[1] != shared1 )
    {
	vti1 = crds1[1];
	vetexidx = 1;
    }
    else if ( crds1[2] != shared0 && crds1[2] != shared1 )
    {
	vti1 = crds1[2];
	vetexidx = 2;
    }

    int nbti10, nbti11; 
    setNeighbors( vetexidx, shared0, ti1, nbti10, nbti11 );
    
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

    triangles_[nti0].neighbors_[0] = searchChild( shared0, ci, nti2 );
    triangles_[nti0].neighbors_[1] = searchChild( ci, vti0, nti1 );
    triangles_[nti0].neighbors_[2] = searchChild( vti0, shared0, nbti00 );
    triangles_[nti1].neighbors_[0] = searchChild( shared1, vti0, nbti01 );
    triangles_[nti1].neighbors_[1] = searchChild( vti0, ci, nti0 );
    triangles_[nti1].neighbors_[2] = searchChild( ci, shared1, nti3 );
    triangles_[nti2].neighbors_[0] = searchChild( shared0, vti1, nbti10 );
    triangles_[nti2].neighbors_[1] = searchChild( vti1, ci, nti3 );
    triangles_[nti2].neighbors_[2] = searchChild( ci, shared0, nti0 );
    triangles_[nti3].neighbors_[0] = searchChild( shared1, ci, nti1 );
    triangles_[nti3].neighbors_[1] = searchChild( ci, vti1, nti2 );
    triangles_[nti3].neighbors_[2] = searchChild( vti1, shared1, nbti11 );
 
    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 2; tis += nti0;
    v0s += 0; v1s += 1; tis += nti1;
    v0s += 0; v1s += 1; tis += nti2;
    v0s += 0; v1s += 2; tis += nti3;

    legalizeTriangles( v0s, v1s, tis );
}


void DAGTriangleTree::legalizeTriangles( TypeSet<char>& v0s, TypeSet<char>& v1s,
					 TypeSet<int>& tis )
{
    int start = 0;
    while ( v0s.size()>start )
    {
	const char v0 = v0s[start]; 
	const char v1 = v1s[start];
	const int ti = tis[start];
	if ( start>10000 )
	{
	    v0s.remove( 0, start );
	    v1s.remove( 0, start );
	    tis.remove( 0, start );
	    start = 0;
	}
	else
	{
	    start++;
	}
	
	if ( ti<0 )
	    continue;

	int ci;
	int checkti = -1;
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
	
	if ( checkti==-1 )
	    continue;

	const int shared0 = triangles_[ti].coordindices_[v0];
	const int shared1 = triangles_[ti].coordindices_[v1];
	const int crdci = triangles_[ti].coordindices_[ci];
	const int* checkcrds = triangles_[checkti].coordindices_;
	int checkpt =-1;
	int vetexidx;
	if ( checkcrds[0] != shared0 && checkcrds[0] != shared1 )
	{
	    checkpt = checkcrds[0];
	    vetexidx = 0;
	}
	else if ( checkcrds[1] != shared0 && checkcrds[1] != shared1 )
	{
	    checkpt = checkcrds[1];
	    vetexidx = 1;
	}
	else if ( checkcrds[2] != shared0 && checkcrds[2] != shared1 )
	{
	    checkpt = checkcrds[2];
	    vetexidx = 2;
	}

	if ( checkpt==-1 )
	    continue;

	if ( !isInsideCircle( mCrd(checkpt), mCrd(crdci), 
			      mCrd(shared0), mCrd(shared1) ) )
	    continue;
	    
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

	int nb0, nb1;
	int nbti0, nbti1;
	setNeighbors( vetexidx, shared0, checkti, nb0, nb1 );
	setNeighbors( ci, shared0, ti, nbti0, nbti1 );

	triangles_[nti1].neighbors_[0] = searchChild( crdci, checkpt, nti2 );
	triangles_[nti1].neighbors_[1] = searchChild( checkpt, shared0, nb0 );
	triangles_[nti1].neighbors_[2] = searchChild( shared0, crdci, nbti0 );
	triangles_[nti2].neighbors_[0] = searchChild( shared1, checkpt, nb1 );
	triangles_[nti2].neighbors_[1] = searchChild( checkpt, crdci, nti1 );
	triangles_[nti2].neighbors_[2] = searchChild( crdci, shared1, nbti1 );

	v0s += 1; v1s += 2; tis += nti1;
	v0s += 0; v1s += 1; tis += nti2;
    }
}


bool DAGTriangleTree::getCoordIndices( TypeSet<int>& result ) const
{
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]!=-1 || child[1]!=-1 || child[2]!=-1 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	if ( c[0]==-4 || c[0]==-2 || c[0]==-3 || c[1]==-4 || 
	     c[1]==-2 || c[1]==-3 || c[2]==-4 || c[2]==-2 || c[2]==-3 )
	    continue;
	
	result += c[0];
	result += c[1];
	result += c[2];
    }

    return result.size();
}


#define mSearch( child ) \
if ( child!=-1 ) \
{ \
    const int* gc = triangles_[child].coordindices_; \
    if ( gc[0]==v0 && gc[1]==v1 || gc[1]==v0 && gc[0]==v1 || \
	 gc[0]==v0 && gc[2]==v1 || gc[2]==v0 && gc[0]==v1 || \
	 gc[1]==v0 && gc[2]==v1 || gc[2]==v0 && gc[1]==v1 )  \
	    return searchChild( v0, v1, child ); \
}


int DAGTriangleTree::searchChild( int v0, int v1, int ti ) const
{
    if ( ti==-1 )
	return -1;

    const int* child = triangles_[ti].childindices_;
    if ( child[0]==-1 && child[1]==-1 && child[2]==-1 )
	return ti;
    
    mSearch( child[0] );
    mSearch( child[1] );
    mSearch( child[2] );

    return -1;
}


DAGTriangleTree::DAGTriangle::DAGTriangle()
{
    coordindices_[0] = -1;
    coordindices_[1] = -1;
    coordindices_[2] = -1;
    childindices_[0] = -1;
    childindices_[1] = -1;
    childindices_[2] = -1;
    neighbors_[0] = -1;
    neighbors_[1] = -1;
    neighbors_[2] = -1;
}


bool DAGTriangleTree::DAGTriangle::operator==( 
	const DAGTriangle& dag ) const
{
    const int d0 = dag.coordindices_[0];
    const int d1 = dag.coordindices_[1];
    const int d2 = dag.coordindices_[2];
    const int c0 = coordindices_[0];
    const int c1 = coordindices_[1];
    const int c2 = coordindices_[2];
    return d0==c0 && d1==c1 && d2==c2 ||
	   d0==c0 && d1==c2 && d2==c1 ||
	   d0==c1 && d1==c0 && d2==c2 ||
	   d0==c1 && d1==c2 && d2==c0 ||
	   d0==c2 && d1==c1 && d2==c0 ||
	   d0==c2 && d1==c0 && d2==c1; 
}



