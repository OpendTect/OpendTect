#ifndef linsolv_h
#define linsolv_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: linsolv.h,v 1.1 2000-03-22 13:41:01 bert Exp $
________________________________________________________________________

LinSolver - Solves linear systems of equations on the form A*x=B. A is
a matrix of the size N*N, x is a column vector of the size N and B is a column
vector of the size N.
@$
*/

#include <math.h>
#include <string.h>
#include <arrayndimpl.h>
#define TINY 1.0e-20

template <class T>
class LinSolver
{
public:
    				LinSolver( const Array2D<T>& A );
    				~LinSolver( );
    
    bool			ready() const { return ready_; }
    int				size() const { return n; }

    void			apply( const T* b, T* x ) const;

protected:
    Array2DImpl<T>		croutsmatrix;
    int*			croutsidx;
    int				n;
    bool			parity;
    bool			ready_;

}; 


template <class T> inline
LinSolver<T>::LinSolver( const Array2D<T>& A )
    : croutsmatrix ( A.size().getSize(0),  A.size().getSize(1) )
    , croutsidx ( new int[A.size().getSize(0)] )
    , n ( A.size().getSize(0) )
    , ready_( false )
    , parity( true )
{
    memcpy( croutsmatrix.getData(), A.getData(),
	    sizeof(T) * A.size().getTotalSz() );

    croutsmatrix.dataUpdated();
    croutsmatrix.unlockData();
    A.unlockData();

    if ( A.size().getSize(0) != A.size().getSize(1) )
	return;

    int imax;

    TypeSet<T> vv( n );

    for ( int i=0; i<n; i++ )
    {
	T big=0;
	for ( int j=0; j<n; j++ )
	{
	    T temp = fabs( croutsmatrix.getVal(i,j) );
	    if ( temp > big)
		big=temp;
	}

	if ( mIS_ZERO( big ) )
	{
	    ready_ = false;
	    return;
	}

	vv[i]=1.0/big;
    }

    for ( int j=0; j<n; j++)
    {
	for (int i=0; i<j; i++ )
	{
	    T sum=croutsmatrix.getVal(i,j);
	    for ( int k=0; k<i; k++ )
		sum -=  croutsmatrix.getVal(i,k) * croutsmatrix.getVal(k,j);

	    croutsmatrix.setVal(i,j,sum);
	}

	T big=0.0;
	for ( int i=j; i<n; i++ )
	{
	    T sum=croutsmatrix.getVal(i,j);
	    for ( int k=0; k<j; k++ )
	    {
		sum -=  croutsmatrix.getVal(i,k)*croutsmatrix.getVal(k,j);
	    }

	    croutsmatrix.setVal(i,j,sum);

	    T dum = vv[i]*fabs(sum);

	    if ( dum >= big)
	    {
		big=dum;
		imax=i;
	    }

	}

	if ( j != imax )
	{
	    for ( int k=0; k<n; k++ )
	    {
		T dum=croutsmatrix.getVal(imax,k);
		croutsmatrix.setVal(imax,k,croutsmatrix.getVal(j,k));
		croutsmatrix.setVal(j,k,dum);
	    }

	    parity = !parity;
	    vv[imax]=vv[j];
	}

	croutsidx[j]=imax;

	if ( mIS_ZERO(croutsmatrix.getVal(j,j) ) )
	    croutsmatrix.setVal(j,j,TINY);

	if ( j != n-1 )
	{
	    T dum=1.0/(croutsmatrix.getVal(j,j));

	    for ( int i=j+1; i<n; i++ )
		croutsmatrix.setVal(i,j,dum * croutsmatrix.getVal(i,j));
	}
    }

    ready_ = true;
}

#undef TINY



template <class T> inline
LinSolver<T>::~LinSolver( )
{   
    delete croutsidx;
}


template <class T> inline
void LinSolver<T>::apply( const T* b, T* x ) const
{
    for ( int idx=0; idx<n; idx++ )
	x[idx] = b[idx];

    int ii=-1;
    
    for ( int i=0; i<n; i++ )
    {
	int ip=croutsidx[i];
	T sum=x[ip];
	x[ip]=x[i];

	if ( ii != -1 )
	{
	    for ( int j=ii; j<=i-1; j++ )
	    {
		sum -= croutsmatrix.getVal(i,j)*x[j];
	    }
	}
	else if ( !mIS_ZERO(sum) )
	    ii=i;

	x[i]=sum;
    }

    for ( int i=n-1; i>=0; i-- )
    {
	T sum=x[i];
	for ( int j=i+1; j<n; j++ )
	    sum -= croutsmatrix.getVal(i,j)*x[j];

	x[i]=sum/croutsmatrix.getVal(i,i);
    }

}

#endif
