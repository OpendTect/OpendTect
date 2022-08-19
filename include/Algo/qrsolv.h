#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"

#define TINY 1.0e-20

/*!
\brief QRSolver - QR decomposition of a matrix A
For an m-by-n matrix A, with m&gt;=n, the QR decomposition is A = Q*R,
where Q is an m-by-n orthogonal matrix, and R is an n-by-n upper-triangular
matrix
*/

template <class T>
mClass(Algo) QRSolver
{
public:
				QRSolver(const Array2D<T>& A);
				~QRSolver();

    bool			isFullRank()	{ return isfullrank_; }

    const Array1DImpl<T>*	apply(const Array1D<T>& B) const;
    const Array1DImpl<T>*	apply(const Array2D<T>& B) const;
    const Array2DImpl<T>*	getQ() const;
    const Array2DImpl<T>*	getR() const;

protected:
    Array2DImpl<T>		qr_;
    Array1DImpl<T>*		rdiag_;
    int				m_;
    int				n_;
    bool			isfullrank_;

};


template <class T> inline
QRSolver<T>::QRSolver( const Array2D<T>& A )
    : qr_(A)
    , m_(A.info().getSize(0))
    , n_(A.info().getSize(1))
    , isfullrank_(true)
{
    if ( A.info().getSize(0) < A.info().getSize(1) )
	return;

    rdiag_ = new Array1DImpl<T>( n_ );
    for ( int k=0; k<n_; ++k )
    {
	double norm = 0.;
	for ( int i=k; i<m_; ++i )
	    norm = Math::Sqrt( norm*norm + qr_.get( i, k )*qr_.get( i, k ) );

	if ( mIsZero(norm,TINY) )
	{
	    rdiag_->set( k, 0. );
	    isfullrank_ = false;
	    continue;
	}

	if ( qr_.get( k, k ) < TINY )
	    norm *= -1.;

	for ( int i=k; i<m_; ++i )
	    qr_.set( i, k, qr_.get( i, k )/norm );

	qr_.set( k, k, qr_.get( k, k ) + 1. );

	for ( int j=k+1; j<n_; ++j )
	{
	    double s = 0.;
	    for ( int i=k; i<m_; ++i )
		s += qr_.get( i, k ) * qr_.get( i, j );

	    s = -s / qr_.get( k, k );
	    for ( int i=k; i<m_; ++i )
		qr_.set( i, j, qr_.get( i, j ) + s * qr_.get( i, k ) );
	}

	rdiag_->set( k, -norm );
    }
}


template <class T> inline
QRSolver<T>::~QRSolver()
{
    delete rdiag_;
}


template <class T> inline
const Array1DImpl<T>* QRSolver<T>::apply( const Array1D<T>& b ) const
{
    if( !rdiag_ )
	return 0;

    const int sz = b.info().getSize(0);
    Array2DImpl<T>* arr = new  Array2DImpl<T>( sz, 1 );
    for ( int idx=0; idx<sz; idx++ )
	arr->set( idx, 0, b.get( idx ) );

    const Array1DImpl<T>* out = apply( *arr );
    delete arr;
    return out;
}


template <class T> inline
const Array1DImpl<T>* QRSolver<T>::apply( const Array2D<T>& b ) const
{
    if ( b.info().getSize(0) != m_ || !isfullrank_ || !rdiag_ )
	return 0;

    const int nx = b.info().getSize(1);
    Array2DImpl<T>* arr = new  Array2DImpl<T>( b );

    // Compute Y = transpose(Q)*B.
    for ( int k=0; k<n_; ++k )
    {
	for ( int j=0; j<nx; ++j )
	{
	    double s = 0.;
	    for ( int i=k; i<m_; ++i )
		s += qr_.get( i, k ) * arr->get( i, j );

	    s = -s / qr_.get( k, k );
	    for ( int i=k; i<m_; ++i )
		arr->set( i, j, arr->get( i, j ) + s * qr_.get( i, k ) );
	}
    }

    // Solve R*X = Y
    for ( int k=n_-1; k>=0; --k )
    {
	for ( int j=0; j<nx; ++j )
	    arr->set( k, j, arr->get( k, j ) / rdiag_->get( k ) );

	for ( int i=0; i<k; ++i )
	    for ( int j=0; j<nx; ++j )
		arr->set( i, j, arr->get( i, j ) -
				arr->get( k, j ) * qr_.get( i, k ) );
    }


    Array1DImpl<T>* out = new Array1DImpl<T>( n_ );
    for ( int idx=0; idx<n_; idx++ )
	out->set( idx, arr->get(idx,0) );

    delete arr;
    return out;
}


template <class T> inline
const Array2DImpl<T>* QRSolver<T>::getQ() const
{
    if( !rdiag_ )
	return 0;

     Array2DImpl<T>* arr = new Array2DImpl<T>( m_, n_ );
     for ( int k=n_-1; k>=0; --k )
     {
	 for ( int i=0; i<m_; ++i )
	     arr->set( i, k, 0. );

	 arr->set( k, k, 1. );
	 for ( int j=k; j<n_; ++j )
	 {
	     if ( mIsZero(qr_.get( k, k ),TINY) )
		 continue;

	     double s = 0.;
	     for ( int i=k; i<m_; ++i )
		 s += qr_.get( i, k) * arr->get( i, j );

	     s = -s / qr_.get( k, k );
	     for ( int i=k; i<m_; ++i )
		 arr->set( i, j, arr->get( i, j ) + s * qr_.get( i, k ) );
	 }
     }

     return arr;
}

#undef TINY


template <class T> inline
const Array2DImpl<T>* QRSolver<T>::getR() const
{
    if( !rdiag_ )
	return 0;

     Array2DImpl<T>* arr = new Array2DImpl<T>( n_, n_ );
     for ( int i=0; i<n_; ++i )
     {
	 arr->set( i, i, rdiag_->get( i ) );
	 for ( int j=i+1; j<n_; ++j )
	     arr->set( i, j, qr_.get( i, j ) );
     }

     return arr;
}
