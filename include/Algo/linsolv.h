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

#include "arrayndimpl.h"
#include "typeset.h"

#include <math.h>

#define TINY 1.0e-20

/*!
\brief LinSolver - Solves linear systems of equations on the form A*x=B. A is
a matrix of the size N*N, x is a column vector of the size N and B is a column
vector of the size N.
*/

template <class T>
mClass(Algo) LinSolver
{
public:
				LinSolver(const Array2D<T>& A);
				LinSolver(const LinSolver&);

				~LinSolver();

    bool			ready() const	{ return ready_; }
    int				size() const	{ return n_; }

    void			apply(const T* b,T* x) const;

protected:
    Array2DImpl<T>		croutsmatrix_;
    int*			croutsidx_;
    int				n_;
    bool			parity_;
    bool			ready_;

};


template <class T> inline
LinSolver<T>::LinSolver( const Array2D<T>& A )
    : croutsmatrix_(A)
    , croutsidx_(new int[A.info().getSize(0)])
    , n_(A.info().getSize(0))
    , ready_(false)
    , parity_(true)
{
    if ( A.info().getSize(0) != A.info().getSize(1) )
	return;

    int imax = mUdf(int);

    TypeSet<T> vv( n_, 0 );

    for ( int i=0; i<n_; i++ )
    {
	T big=0;
	for ( int j=0; j<n_; j++ )
	{
	    T temp = fabs( croutsmatrix_.get(i,j) );
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

    for ( int j=0; j<n_; j++)
    {
	for (int i=0; i<j; i++ )
	{
	    T sum=croutsmatrix_.get(i,j);
	    for ( int k=0; k<i; k++ )
		sum -=  croutsmatrix_.get(i,k) * croutsmatrix_.get(k,j);

	    croutsmatrix_.set(i,j,sum);
	}

	T big=0.0;
	for ( int i=j; i<n_; i++ )
	{
	    T sum=croutsmatrix_.get(i,j);
	    for ( int k=0; k<j; k++ )
	    {
		sum -=  croutsmatrix_.get(i,k)*croutsmatrix_.get(k,j);
	    }

	    croutsmatrix_.set(i,j,sum);

	    T dum = vv[i]*fabs(sum);

	    if ( dum >= big)
	    {
		big=dum;
		imax=i;
	    }

	}

	if ( j != imax )
	{
	    for ( int k=0; k<n_; k++ )
	    {
		T dum=croutsmatrix_.get(imax,k);
		croutsmatrix_.set(imax,k,croutsmatrix_.get(j,k));
		croutsmatrix_.set(j,k,dum);
	    }

	    parity_ = !parity_;
	    vv[imax]=vv[j];
	}

	croutsidx_[j]=imax;

	if ( mIsZero(croutsmatrix_.get(j,j),mDefEps) )
	    croutsmatrix_.set(j,j,TINY);

	if ( j != n_-1 )
	{
	    T dum=1.0f/(croutsmatrix_.get(j,j));

	    for ( int i=j+1; i<n_; i++ )
		croutsmatrix_.set(i,j,dum * croutsmatrix_.get(i,j));
	}
    }

    ready_ = true;
}

#undef TINY

template <class T> inline
LinSolver<T>::LinSolver( const LinSolver& s )
    : croutsmatrix_(s.croutsmatrix_)
    , croutsidx_(0)
    , n_(s.n_)
    , parity_(s.parity_)
    , ready_(s.ready_)
{
    if ( s.croutsidx_ )
    {
	croutsidx_ = new int[s.n_];
	for ( int idx=0; idx<s.n_; idx++ )
	    croutsidx_[idx] = s.croutsidx_[idx];
    }
}


template <class T> inline
LinSolver<T>::~LinSolver( )
{
    delete [] croutsidx_;
}


template <class T> inline
void LinSolver<T>::apply( const T* b, T* x ) const
{
    for ( int idx=0; idx<n_; idx++ )
	x[idx] = b[idx];

    int ii=-1;

    for ( int i=0; i<n_; i++ )
    {
	int ip=croutsidx_[i];
	T sum=x[ip];
	x[ip]=x[i];

	if ( ii != -1 )
	{
	    for ( int j=ii; j<=i-1; j++ )
	    {
		sum -= croutsmatrix_.get(i,j)*x[j];
	    }
	}
	else if ( !mIsZero(sum,mDefEps) )
	    ii=i;

	x[i]=sum;
    }

    for ( int i=n_-1; i>=0; i-- )
    {
	T sum=x[i];
	for ( int j=i+1; j<n_; j++ )
	    sum -= croutsmatrix_.get(i,j)*x[j];

	x[i]=sum/croutsmatrix_.get(i,i);
    }

}

#endif
