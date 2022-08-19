#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "math2.h"


typedef Array1DImpl<float> Array1DVector;

/*!\brief Matrix class based on Array2D. Initialized to 0.

The general policy is to handle stupidities like dimension size errors with a
pErrMsg, and do whatever seems to be reasonable then.

*/


template <class fT>
mClass(Algo) Array2DMatrix
{
public:
			Array2DMatrix( int sz0=1, int sz1=0 )
			    : a2d_(sz0<1?1:sz0,
				   sz1<1?(sz0<1?1:sz0):sz1)	{ setAll(); }
			Array2DMatrix( const Array2DMatrix& oth )
			    : a2d_(oth.a2d_)		{}
			Array2DMatrix( const Array2DImpl<fT>& a2d )
			    : a2d_(a2d)			{}
			Array2DMatrix( const Array2D<fT>& a2d )
			    : a2d_(a2d)			{}
    inline Array2DMatrix& operator =( const Array2DMatrix& oth )
				    { return a2d_ = oth.a2d_; }
    inline Array2DMatrix& operator =( const Array2DImpl<fT>& a2d )
				    { return a2d_ = a2d; }
    inline Array2DMatrix& operator =( const Array2D<fT>& a2d )
				    { return a2d_ = a2d; }
    inline bool		operator ==( const Array2DMatrix<fT>& oth ) const
				    { return isEq( oth.a2d_, fT(1e-6)) ; }
    inline bool		isEq(const Array2D<fT>&,fT eps=fT(1e-6)) const;

    inline int		size(bool dim1=false) const;
    inline void		set( int i0, int i1, fT v ) { a2d_.set(i0,i1,v); }
    inline fT&		get(int i0,int i1);
    inline fT		get( int i0, int i1 ) const { return a2d_.get(i0,i1); }

    inline void		setAll( fT v=fT(0) )	{ a2d_.setAll( v ); }
    inline void		setDiagonal(fT);
    inline void		setToIdentity()		{ setAll(0); setDiagonal(1); }

    inline void		add(fT);
    inline void		add(const Array2DMatrix&);
    inline void		multiply(fT);
    inline void		multiply(const Array2DMatrix&);
    inline void		transpose();

    inline void		getSum(const Array2DMatrix&,Array2DMatrix&) const;
    inline void		getProduct(const Array1DVector&,Array1DVector&) const;
    inline void		getProduct(const Array2DMatrix&,Array2DMatrix&) const;
    inline void		getTransposed(Array2DMatrix&);
    inline bool		getCholesky(Array2DMatrix&) const;

    Array2DImpl<fT>	a2d_;

};


//!\brief easily define the matrix dimension sizes
#define mDefineA2DMatSizes(m,nm) \
    const int nm##0 = (m).size( false ); const int nm##1 = (m).size( true )


template <class fT>
inline int Array2DMatrix<fT>::size( bool dim1 ) const
{
    return a2d_.info().getSize( dim1 ? 1 : 0 );
}


template <class fT>
inline fT& Array2DMatrix<fT>::get( int i0, int i1 )
{
    const od_int64 offset = a2d_.info().getOffset( i0, i1 );
    return a2d_.getData()[offset];
}


#define mDefineImplA2DMatSizes mDefineA2DMatSizes(*this,sz)


template <class fT>
inline bool Array2DMatrix<fT>::isEq( const Array2D<fT>& a2d, fT eps ) const
{
    mDefineImplA2DMatSizes;
    if ( a2d_.info().getSize(0) != sz0 || a2d_.info().getSize(1) != sz1 )
	return false;

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<sz1; idx1++ )
	{
	    if ( !isFPEqual( get(idx0,idx1), a2d.get(idx0,idx1), eps ) )
		return false;
	}
    }
    return true;
}


template <class fT>
inline void Array2DMatrix<fT>::setDiagonal( fT v )
{
    mDefineImplA2DMatSizes;
    const int sz = sz0 > sz1 ? sz1 : sz0;

    for ( int idx=0; idx<sz; idx++ )
	set( idx, idx, v );
}


template <class fT>
inline void Array2DMatrix<fT>::add( fT val )
{
    mDefineImplA2DMatSizes;
    for ( int idx0=0; idx0<sz0; idx0++ )
	for ( int idx1=0; idx1<sz1; idx1++ )
	    get( idx0, idx1 ) += val;
}


template <class fT>
inline void Array2DMatrix<fT>::add( const Array2DMatrix& in )
{
    mDefineImplA2DMatSizes;
    mDefineA2DMatSizes( in, insz );
    const int outsz0 = insz0 > sz0 ? sz0 : insz0;
    const int outsz1 = insz1 > sz1 ? sz1 : insz1;

    for ( int idx0=0; idx0<outsz0; idx0++ )
    {
	for ( int idx1=0; idx1<outsz1; idx1++ )
	{
	    fT res = 0;
	    for ( int idx=0; idx<sz1; idx++ )
		get( idx0, idx1 ) += in.get( idx0, idx1 );
	}
    }
}


template <class fT>
inline void Array2DMatrix<fT>::multiply( fT fac )
{
    mDefineImplA2DMatSizes;
    for ( int idx0=0; idx0<sz0; idx0++ )
	for ( int idx1=0; idx1<sz1; idx1++ )
	    get( idx0, idx1 ) *= fac;
}


template <class fT>
inline void Array2DMatrix<fT>::multiply( const Array2DMatrix& in )
{
    const Array2DMatrix copy( *this );
    copy.getProduct( in, *this );
}


#define mA2DMatHandleDimErr(v1,v2) \
if ( v1 != v2 ) \
{ \
    BufferString emsg( "Dim error: " #v1 "=", v1, " " #v2 "=" ); \
    emsg.add( v2 ); pErrMsg( emsg ); \
    if ( v2 > v1 ) \
	const_cast<int&>( v2 ) = v1; \
    else \
	const_cast<int&>( v1 ) = v2; \
}


template <class fT>
inline void Array2DMatrix<fT>::getSum( const Array2DMatrix& in,
					Array2DMatrix& out ) const
{
    mDefineImplA2DMatSizes;
    mDefineA2DMatSizes( in, insz );
    const int outsz0 = insz0 > sz0 ? sz0 : insz0;
    const int outsz1 = insz1 > sz1 ? sz1 : insz1;
    out.a2d_.setSize( outsz0, outsz1 );

    for ( int idx0=0; idx0<outsz0; idx0++ )
    {
	for ( int idx1=0; idx1<outsz1; idx1++ )
	{
	    fT res = 0;
	    for ( int idx=0; idx<sz1; idx++ )
		out.set( idx0, idx1, get(idx0,idx1) + in.get(idx0,idx1) );
	}
    }
}


template <class fT>
inline void Array2DMatrix<fT>::getProduct( const Array1DVector& vin,
				    Array1DVector& vout ) const
{
    mDefineImplA2DMatSizes;
    const int vsz = vin.info().getSize(0);
    mA2DMatHandleDimErr(vsz,sz1)
    vout.setSize( vsz );

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	fT res = 0;
	for ( int idx1=0; idx1<sz1; idx1++ )
	    res += get( idx0, idx1 ) * vin.get( idx1 );
	vout.set( idx0, res );
    }
}


template <class fT>
inline void Array2DMatrix<fT>::getProduct( const Array2DMatrix& in,
				    Array2DMatrix& out ) const
{
    mDefineImplA2DMatSizes;
    mDefineA2DMatSizes( in, insz );
    mA2DMatHandleDimErr(sz1,insz0)
    out.a2d_.setSize( sz0, insz1 );

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<insz1; idx1++ )
	{
	    fT res = 0;
	    for ( int idx=0; idx<sz1; idx++ )
		res += in.get( idx, idx1 ) * get( idx0, idx );
	    out.set( idx0, idx1, res );
	}
    }
}


template <class fT>
inline void Array2DMatrix<fT>::transpose()
{
    const Array2DMatrix copy( *this );
    copy.getTransposed( *this );
}


template <class fT>
inline void Array2DMatrix<fT>::getTransposed( Array2DMatrix<fT>& out )
{
    mDefineImplA2DMatSizes;
    out.a2d_.setSize( sz1, sz0 );

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<sz1; idx1++ )
	    out.set( idx1, idx0, get( idx0, idx1 ) );
    }
}


template <class fT>
inline bool Array2DMatrix<fT>::getCholesky( Array2DMatrix& out ) const
{
    mDefineImplA2DMatSizes;
    mA2DMatHandleDimErr(sz0,sz1)
    out.a2d_.setSize( sz0, sz0 );

    out.setAll( 0 );

    for( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<=idx0; idx1++ )
	{
	    fT sum = 0;
	    for ( int j=0; j<idx1; j++ )
		sum += out.get( idx0, j ) * out.get( idx1, j );

	    fT val = 0;
	    if ( idx0 == idx1 )
	    {
		val = get( idx0, idx0 ) - sum;
		if ( val < 0 )
		    return false;
		val = Math::Sqrt( val );
	    }
	    else
	    {
		const fT dividend = get( idx0, idx1 ) - sum;
		const fT divideby = out.get( idx1, idx1 );
		if ( divideby )
		    val = dividend / divideby;
		else if ( dividend )
		    return false;
	    }
	    out.set( idx0, idx1, val );
	}
    }

    return true;
}
