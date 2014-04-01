#ifndef array2dmatrix_h
#define array2dmatrix_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
 RCS:           $Id$
________________________________________________________________________


@$*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "math2.h"


typedef Array1DImpl<float> Array1DVector;

/*!\brief Matrix class based on Array2D. Initialized to 0.

The general policy is to handle stupidities like dimension size errors with a
pErrMsg, and do whatever seems to be reasonable then.

*/


template <class fT>
mExpClass(Algo) Array2DMatrix
{
public:
			Array2DMatrix( int sz0, int sz1=-1 )
			    : a2d_(sz0,sz1<1?sz0:sz1)	{ setAll(); }
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

    inline void		setAll( fT v=fT(0) )		{ a2d_.setAll( v ); }
    inline void		setToIdentity();

    inline void		multiply(const Array1DVector&,Array1DVector&) const;
    inline void		multiply(const Array2DMatrix&,Array2DMatrix&) const;

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
inline void Array2DMatrix<fT>::setToIdentity()
{
    mDefineImplA2DMatSizes;

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<sz1; idx1++ )
	    set( idx0, idx1, idx0==idx1 ? fT(1) : fT(0) );
    }
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
inline void Array2DMatrix<fT>::multiply( const Array1DVector& vin,
				    Array1DVector& vout ) const
{
    //TODO: maybe this is mirrored

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
inline void Array2DMatrix<fT>::multiply( const Array2DMatrix& min,
				    Array2DMatrix& mout ) const
{
    //TODO: maybe this is mirrored

    mDefineImplA2DMatSizes;
    mDefineA2DMatSizes( min, insz );
    mA2DMatHandleDimErr(sz1,insz0)
    mout.a2d_.setSize( sz0, insz1 );

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<insz1; idx1++ )
	{
	    fT res = 0;
	    for ( int idx=0; idx<sz1; idx++ )
		res += min.get( idx, idx1 ) * get( idx0, idx );
	    mout.set( idx0, idx1, res );
	}
    }
}


template <class fT>
inline bool Array2DMatrix<fT>::getCholesky( Array2DMatrix& mout ) const
{
    mDefineImplA2DMatSizes;
    mA2DMatHandleDimErr(sz0,sz1)
    mout.a2d_.setSize( sz0, sz1 );

    mout.setAll();

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	for ( int idx1=0; idx1<sz1; idx1++ )
	{
	    fT val = get( idx0, idx1 );
	    for ( int idx=0; idx<idx0; idx++ )
		val -= mout.get(idx0,idx) * mout.get(idx1,idx);

	    fT& diagval = mout.get( idx0, idx0 );
	    if ( idx0 == idx1 )
	    {
		if ( val <= 0 )
		    return false;
		diagval = Math::Sqrt( val );
	    }
	    else if ( idx0 < idx1 )
	    {
		if ( !diagval )
		    return false;
		mout.set( idx1, idx0, val / diagval );
	    }
	}
    }

    return true;
}



#endif
