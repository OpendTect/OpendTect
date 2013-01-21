#ifndef linsolv_h
#define linsolv_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

@$
*/

#include "math.h"
#include "string.h"
#include "arrayndimpl.h"
#include "sets.h"
#define TINY 1.0e-20

/*!
\brief LinSolver - Solves linear systems of equations on the form A*x=B. A is
a matrix of the size N*N, x is a column vector of the size N and B is a column
vector of the size N.
*/

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
    : croutsmatrix ( A )
    , croutsidx ( new int[A.info().getSize(0)] )
    , n ( A.info().getSize(0) )
    , ready_( false )
    , parity( true )
{
    if ( A.info().getSize(0) != A.info().getSize(1) )
	return;

    int imax = mUdf(int);

    TypeSet<T> vv( n, 0 );

    for ( int i=0; i<n; i++ )
    {
	T big=0;
	for ( int j=0; j<n; j++ )
	{
	    T temp = fabs( croutsmatrix.get(i,j) );
	    if ( temp > big)
		big=temp;
	}

	if ( mIsZero(big,mDefEps) )
	{
	    ready_ = false;
	    return;
	}

	vv[i]=1.0f/big;
    }

    for ( int j=0; j<n; j++)
    {
	for (int i=0; i<j; i++ )
	{
	    T sum=croutsmatrix.get(i,j);
	    for ( int k=0; k<i; k++ )
		sum -=  croutsmatrix.get(i,k) * croutsmatrix.get(k,j);

	    croutsmatrix.set(i,j,sum);
	}

	T big=0.0;
	for ( int i=j; i<n; i++ )
	{
	    T sum=croutsmatrix.get(i,j);
	    for ( int k=0; k<j; k++ )
	    {
		sum -=  croutsmatrix.get(i,k)*croutsmatrix.get(k,j);
	    }

	    croutsmatrix.set(i,j,sum);

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
		T dum=croutsmatrix.get(imax,k);
		croutsmatrix.set(imax,k,croutsmatrix.get(j,k));
		croutsmatrix.set(j,k,dum);
	    }

	    parity = !parity;
	    vv[imax]=vv[j];
	}

	croutsidx[j]=imax;

	if ( mIsZero(croutsmatrix.get(j,j),mDefEps) )
	    croutsmatrix.set(j,j,TINY);

	if ( j != n-1 )
	{
	    T dum=1.0f/(croutsmatrix.get(j,j));

	    for ( int i=j+1; i<n; i++ )
		croutsmatrix.set(i,j,dum * croutsmatrix.get(i,j));
	}
    }

    ready_ = true;
}

#undef TINY



template <class T> inline
LinSolver<T>::~LinSolver( )
{   
    delete [] croutsidx;
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
		sum -= croutsmatrix.get(i,j)*x[j];
	    }
	}
	else if ( !mIsZero(sum,mDefEps) )
	    ii=i;

	x[i]=sum;
    }

    for ( int i=n-1; i>=0; i-- )
    {
	T sum=x[i];
	for ( int j=i+1; j<n; j++ )
	    sum -= croutsmatrix.get(i,j)*x[j];

	x[i]=sum/croutsmatrix.get(i,i);
    }

}

#endif
