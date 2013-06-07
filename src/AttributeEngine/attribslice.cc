/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";

#include "attribslice.h"

namespace Attrib
{

Slice::Slice( int nrows, int ncols, float udfval )
    : Array2DImpl<float>(nrows,ncols)
{
    setUndefValue( udfval, true );
}


float Slice::undefValue() const
{
    return udfval_;
}


void Slice::setUndefValue( float udfval, bool initdata )
{
    udfval_ = udfval;
    if ( initdata )
	setAll( udfval );
}


SliceSet::SliceSet()
{
    allowNull( true );
}


SliceSet::~SliceSet()
{ deepUnRef( *this ); }


#define mInlIdx ((inl-sampling_.hrg.start.inl) / sampling_.hrg.step.inl)
#define mCrlIdx ((crl-sampling_.hrg.start.crl) / sampling_.hrg.step.crl)
#define mZIdx (sampling_.zrg.nearestIndex(z))

void SliceSet::getIdxs( int inl, int crl, float z,
			 int& i0, int& i1, int& i2 ) const
{
    if ( direction_ == CubeSampling::Z )
	{ i0 = mZIdx; i1 = mInlIdx; i2 = mCrlIdx; }
    else if ( direction_ == CubeSampling::Inl )
	{ i0 = mInlIdx; i1 = mCrlIdx; i2 = mZIdx; }
    else if ( direction_ == CubeSampling::Crl )
	{ i0 = mCrlIdx; i1 = mInlIdx; i2 = mZIdx; }
}


void SliceSet::getIdx( int nr, int inl, int crl, float z, int& iout ) const
{
    if ( direction_ == CubeSampling::Z )
	iout = nr ? (nr == 1 ? mInlIdx : mCrlIdx) : mZIdx;
    else if ( direction_ == CubeSampling::Inl )
	iout = nr ? (nr == 1 ? mCrlIdx : mZIdx) : mInlIdx;
    else if ( direction_ == CubeSampling::Crl )
	iout = nr ? (nr == 1 ? mInlIdx : mZIdx) : mCrlIdx;
}


Array3D<float>* SliceSet::createArray( int inldim, int crldim,
					 int depthdim) const
{
    int size0=-1, size1=-1;
    float udefval;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx] )
	{
	    size0 = (*this)[idx]->info().getSize(0);
	    size1 = (*this)[idx]->info().getSize(1);
	    udefval = (*this)[idx]->undefValue();
	    break;
	}
    }

    if ( size0==-1 ) return 0;

    if ( direction_== CubeSampling::Z )
    {
	const int inlsz = size0;
	const int crlsz = size1;
	const int zsz = size();
	Array3DInfoImpl arraysize;
	arraysize.setSize( inldim, inlsz );
	arraysize.setSize( crldim, crlsz );
	arraysize.setSize( depthdim, zsz );
	Array3D<float>* res = new Array3DImpl<float>( arraysize );

	int pos[3];
	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    for ( int inl=0; inl<inlsz; inl++ )
	    {
		for ( int crl=0; crl<crlsz; crl++ )
		{
		    const float val =
			(*this)[zidx] ?(*this)[zidx]->get( inl, crl ) : udefval;
		    pos[inldim] = inl; pos[crldim]=crl; pos[depthdim]=zidx;
		    res->setND( pos, val );
		}
	    }
	}

	return res;
    }

    if ( direction_ == CubeSampling::Crl )
    {
	const int inlsz = size0;
	const int crlsz = size();
	const int zsz = size1;
	Array3DInfoImpl arraysize;
	arraysize.setSize( inldim, inlsz );
	arraysize.setSize( crldim, crlsz );
	arraysize.setSize( depthdim, zsz );
	Array3D<float>* res = new Array3DImpl<float>( arraysize );

	int pos[3];
	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    for ( int inl=0; inl<inlsz; inl++ )
	    {
		for ( int crl=0; crl<crlsz; crl++ )
		{
		    const float val =
			(*this)[crl] ? (*this)[crl]->get( inl, zidx ) : udefval;
		    pos[inldim]=inl; pos[crldim]=crl; pos[depthdim]=zidx;
		    res->setND( pos, val );
		}
	    }
	}

	return res;
    }

    const int inlsz = size();
    const int crlsz = size0;
    const int zsz = size1;

    Array3DInfoImpl arraysize;
    arraysize.setSize( inldim, inlsz );
    arraysize.setSize( crldim, crlsz );
    arraysize.setSize( depthdim, zsz );
    Array3D<float>* res = new Array3DImpl<float>( arraysize );

    int pos[3];
    for ( int zidx=0; zidx<zsz; zidx++ )
    {
	for ( int inl=0; inl<inlsz; inl++ )
	{
	    for ( int crl=0; crl<crlsz; crl++ )
	    {
		const float val =
			(*this)[inl] ? (*this)[inl]->get( crl, zidx ) : udefval;
		pos[inldim]=inl; pos[crldim]=crl; pos[depthdim]=zidx;
		res->setND( pos, val );
	    }
	}
    }

    return res;
}


#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

float SliceSet::getValue( int inl, int crl, float z ) const
{
    int idx0, idx1, idx2;
    getIdxs( inl, crl, z, idx0, idx1, idx2 );

    const int sz0 = size();
    if ( !mIsValid(idx0,sz0) ) return mUdf(float);

    const Slice* slice = (*this)[idx0];
    if ( !slice ) return mUdf(float);

    const int sz1 = slice->info().getSize(0);
    const int sz2 = slice->info().getSize(1);
    if ( !mIsValid(idx1,sz1) || !mIsValid(idx2,sz2) ) return mUdf(float);

    return slice->get( idx1, idx2 );
}

}; //namespacer

