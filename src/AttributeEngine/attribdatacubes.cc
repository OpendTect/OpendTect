/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/


static const char* rcsID = "$Id: attribdatacubes.cc,v 1.3 2005-10-28 21:17:01 cvskris Exp $";

#include "attribdatacubes.h"
#include "survinfo.h"
#include "interpol.h"

namespace Attrib
{

DataCubes::DataCubes()
    : inlsampling( SI().inlRange(true).start, SI().inlRange(true).step )
    , crlsampling( SI().crlRange(true).start, SI().crlRange(true).step )
    , z0( mNINT(SI().zRange(true).start/SI().zRange(true).step) )
    , zstep( SI().zRange(true).step )
    , inlsz( 0 )
    , crlsz( 0 )
    , zsz( 0 )
{ mRefCountConstructor; }


DataCubes::~DataCubes()
{ deepErase( cubes ); }


void DataCubes::addCube()
{
    Array3DImpl<float>* arr = new Array3DImpl<float>( inlsz, crlsz, zsz, false);
    cubes += arr;
}


void DataCubes::removeCube( int idx )
{
    delete cubes[idx];
    cubes.remove(idx);
}


void DataCubes::setSizeAndPos( const CubeSampling& cs )
{
    inlsampling.start = cs.hrg.start.inl;
    crlsampling.start = cs.hrg.start.crl;
    inlsampling.step = cs.hrg.step.inl;
    crlsampling.step = cs.hrg.step.crl;
    z0 = mNINT(cs.zrg.start/cs.zrg.step);
    zstep = cs.zrg.step;

    setSize( cs.nrInl(), cs.nrCrl(), cs.nrZ() );
}



void  DataCubes::setSize( int nrinl, int nrcrl, int nrz )
{
    inlsz = nrinl;
    crlsz = nrcrl;
    zsz = nrz;

    for ( int idx=0; idx<cubes.size(); idx++ )
	cubes[idx]->setSize( inlsz, crlsz, zsz );
}


void DataCubes::setValue( int array, int inlidx, int crlidx, int zidx,
			  float val )
{
    cubes[array]->set( inlidx, crlidx, zidx, val );
}


bool DataCubes::getValue( int array, const BinIDValue& bidv, float* res ) const
{
    const int inlidx = inlsampling.nearestIndex( bidv.binid.inl );
    if ( inlidx<0 || inlidx>=inlsz ) return false;
    const int crlidx = crlsampling.nearestIndex( bidv.binid.crl );
    if ( crlidx<0 || crlidx>=crlsz ) return false;

    const float* data = cubes[array]->getData();
    data += cubes[array]->info().getMemPos( inlidx, crlidx, 0 );

    const float zpos = bidv.value/zstep-z0;

    float dummy;
    if ( !interpolateSampled( data, zsz, zpos, dummy, false ) )
	return false;

    *res = dummy;
    return true;
}


bool DataCubes::includes( const BinIDValue& bidv ) const
{
    if ( !includes( bidv.binid ) )
	return false;

    const float zpos = bidv.value/zstep-z0;
    return zpos<0 || zpos>zsz-1;
}


bool DataCubes::includes( const BinID& binid ) const
{
    const int inlidx = inlsampling.nearestIndex( binid.inl );
    if ( inlidx<0 || inlidx>=inlsz ) return false;
    const int crlidx = crlsampling.nearestIndex( binid.crl );
    if ( crlidx<0 || crlidx>=crlsz ) return false;
    return true;
}




const Array3D<float>& DataCubes::getCube( int idx ) const
{ return *cubes[idx]; }


CubeSampling DataCubes::cubeSampling() const
{
    CubeSampling res(false);

    if ( inlsz && crlsz && zsz )
    {
	res.hrg.start = BinID( inlsampling.start, crlsampling.start );
	res.hrg.stop = BinID( inlsampling.atIndex(inlsz-1),
	crlsampling.atIndex(crlsz-1) );

	res.zrg.start = z0 * zstep;
	res.zrg.stop = (z0+crlsz-1) * zstep;
    }

    return res;
}


}; // namespace Attrib
