/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: delaunay.cc,v 1.3 2008-01-31 21:33:49 cvsyuancheng Exp $";

#include "delaunay.h"
#include "sorting.h"
#include "positionlist.h"

#define mTolerance 0.0000001


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
    tolabs = mTolerance * ( tolabs > fabs(vt.y) ? tolabs : fabs(vt.y) );
    
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


