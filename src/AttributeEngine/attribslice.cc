/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: attribslice.cc,v 1.1 2005-02-04 09:28:29 kristofer Exp $";

#include "attribslice.h"

namespace Attrib
{

Slice::Slice( int nrows, int ncols, float udfval )
    : Array2DImpl<float>(nrows,ncols)
{
    mRefCountConstructor;
    setUndefValue( udfval, true );
}


float Slice::undefValue() const
{
    return udfval_;
}


void Slice::setUndefValue( float udfval, bool initdata )
{
    udfval_ = udfval;
    float* fp = getData(); if ( !fp ) return;
    float* endfp = fp + info().getTotalSz();
    while ( fp != endfp ) *fp++ = udfval_;
}


SliceSet::SliceSet()
{
    mRefCountConstructor;
    allowNull( true );
}


SliceSet::~SliceSet()
{ deepUnRef( *this ); }


int SliceSet::dim(int nr,Slice::Dir slcdir) const
{
    if ( nr == 1 )
	return slcdir == Slice::Inl ? sampling.nrCrl() : sampling.nrInl();
    if ( nr == 2 )
	return slcdir == Slice::Hor ? sampling.nrCrl() : sampling.nrZ();

    return slcdir == Slice::Inl ? sampling.nrInl()
	    : (slcdir == Slice::Crl ? sampling.nrCrl() : sampling.nrZ());
}


Slice::Dir
SliceSet::defaultDirection( const CubeSampling& cs )
{
    const int nrinl = cs.nrInl();
    const int nrcrl = cs.nrCrl();
    const int nrz = cs.nrZ();

    return	nrz < nrinl && nrz < nrcrl	? Slice::Hor
	    : ( nrinl < nrz && nrinl < nrcrl	? Slice::Inl
			    			: Slice::Crl );
}


int SliceSet::dimNr( Slice::Dir slcdir ) const
{
    if ( direction == slcdir )
	return 0;

    if ( direction == Slice::Hor )
	return slcdir == Slice::Inl ? 1 : 2;

    return slcdir == Slice::Hor ? 2 : 1;
}


#define mInlIdx ((inl-sampling.hrg.start.inl) / sampling.hrg.step.inl)
#define mCrlIdx ((crl-sampling.hrg.start.crl) / sampling.hrg.step.crl)
#define mZIdx (sampling.zrg.nearestIndex(z))

void SliceSet::getIdxs( int inl, int crl, float z,
	                                     int& i0, int& i1, int& i2 ) const
{
    if ( direction == Slice::Hor )
	{ i0 = mZIdx; i1 = mInlIdx; i2 = mCrlIdx; }
    else if ( direction == Slice::Inl )
	{ i0 = mInlIdx; i1 = mCrlIdx; i2 = mZIdx; }
    else if ( direction == Slice::Crl )
	{ i0 = mCrlIdx; i1 = mInlIdx; i2 = mZIdx; }
}


void SliceSet::getIdx( int nr, int inl, int crl, float z,
	                                    int& iout ) const
{
    if ( direction == Slice::Hor )
	iout = nr ? (nr == 1 ? mInlIdx : mCrlIdx) : mZIdx;
    else if ( direction == Slice::Inl )
	iout = nr ? (nr == 1 ? mCrlIdx : mZIdx) : mInlIdx;
    else if ( direction == Slice::Crl )
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

    if ( direction==Slice::Hor )
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
		    res->set( pos, val );
		}
	    }
	}

	return res;
    }

    if ( direction==Slice::Crl )
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
		    res->set( pos, val );
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
		res->set( pos, val );
	    }
	}
    }

    return res;
}


}; //namespacer

