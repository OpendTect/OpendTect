/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/


static const char* rcsID = "$Id: attribdatacubes.cc,v 1.29 2010-03-09 08:01:56 cvsbert Exp $";

#include "attribdatacubes.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "idxable.h"
#include "convmemvalseries.h"

namespace Attrib
{

DataCubes::DataCubes()
    : inlsampling( SI().inlRange(true).start, SI().inlRange(true).step )
    , crlsampling( SI().crlRange(true).start, SI().crlRange(true).step )
    , z0( mNINT(SI().zRange(true).start/SI().zRange(true).step) )
    , zstep( SI().zRange(true).step )
    , inlsz_( 0 )
    , crlsz_( 0 )
    , zsz_( 0 )
{}


DataCubes::~DataCubes()
{ deepErase( cubes_ ); }


bool DataCubes::addCube( const  BinDataDesc* desc )
{
    float dummy;
    const BinDataDesc floatdesc( dummy );
    Array3DImpl<float>* arr = 0;
    if ( !desc || (*desc)==floatdesc )
    {
        arr = new Array3DImpl<float>( inlsz_, crlsz_, zsz_ );
	if ( !arr->isOK() )
	{
	    delete arr;
	    return false;
	}
    }
    else
    {
	 arr = new Array3DImpl<float>( 0, 0, 0 );
	 ConvMemValueSeries<float>* stor= new ConvMemValueSeries<float>(0,*desc);
	 arr->setStorage( stor );
	 arr->setSize( inlsz_, crlsz_, zsz_ );
	 if ( !stor->storArr() )
	 {
	     delete arr;
	     return false;
	 }
    }
	
    cubes_ += arr;
    return true;
}


bool DataCubes::addCube( float val, const BinDataDesc* t )
{
    if ( !addCube( t ) )
	return false;

    setValue( cubes_.size()-1, val );
    return true;
}
    

void DataCubes::removeCube( int idx )
{
    delete cubes_[idx];
    cubes_.remove(idx);
}


bool DataCubes::setSizeAndPos( const CubeSampling& cs )
{
    inlsampling.start = cs.hrg.start.inl;
    crlsampling.start = cs.hrg.start.crl;
    inlsampling.step = cs.hrg.step.inl;
    crlsampling.step = cs.hrg.step.crl;
    z0 = mNINT(cs.zrg.start/cs.zrg.step);
    zstep = cs.zrg.step;

    return setSize( cs.nrInl(), cs.nrCrl(), cs.nrZ() );
}



bool DataCubes::setSize( int nrinl, int nrcrl, int nrz )
{
    inlsz_ = nrinl;
    crlsz_ = nrcrl;
    zsz_ = nrz;

    bool res = true;
    for ( int idx=0; idx<cubes_.size() && res; idx++ )
	res = cubes_[idx]->setSize( inlsz_, crlsz_, zsz_ );

    if ( res )
	return true;

    while ( cubes_.size() )
	removeCube( 0 );

    return false;
}


void DataCubes::setValue( int array, int inlidx, int crlidx, int zidx,
			  float val )
{
    cubes_[array]->set( inlidx, crlidx, zidx, val );
}


void DataCubes::setValue( int array, float val )
{
    cubes_[array]->setAll( val );
}


bool DataCubes::getValue( int array, const BinIDValue& bidv, float* res,
			  bool interpolate ) const
{
    const int inlidx = inlsampling.nearestIndex( bidv.binid.inl );
    if ( inlidx<0 || inlidx>=inlsz_ ) return false;
    const int crlidx = crlsampling.nearestIndex( bidv.binid.crl );
    if ( crlidx<0 || crlidx>=crlsz_ ) return false;

    if ( cubes_.size() <= array ) return false;

    const OffsetValueSeries<float> data(*cubes_[array]->getStorage(),
		     cubes_[array]->info().getOffset( inlidx, crlidx, 0 ) );

    const float zpos = bidv.value/zstep-z0;

    if ( !interpolate )
    {
	const int zidx = mNINT( zpos );
	if ( zidx < 0 || zidx >= zsz_ ) return false;
	*res = data.value( zidx );
	return true;
    }

    float interpval;
    if ( !IdxAble::interpolateRegWithUdfWithOff( data, zsz_, 0, zpos,
						 interpval, false ) )
	return false;

    *res = interpval;
    return true;
}


bool DataCubes::includes( const BinIDValue& bidv ) const
{
    if ( !includes( bidv.binid ) )
	return false;

    const float zpos = bidv.value/zstep-z0;
    const float eps = bidv.sCompareEpsilon()/zstep;
    return zpos>-eps && zpos<=zsz_-1+eps;
}


bool DataCubes::includes( const BinID& binid ) const
{
    const int inlidx = inlsampling.nearestIndex( binid.inl );
    if ( inlidx<0 || inlidx>=inlsz_ ) return false;
    const int crlidx = crlsampling.nearestIndex( binid.crl );
    if ( crlidx<0 || crlidx>=crlsz_ ) return false;
    return true;
}


bool DataCubes::includes( const CubeSampling& cs ) const
{
    return includes( BinIDValue( cs.hrg.start, cs.zrg.start ) ) &&
           includes( BinIDValue( cs.hrg.stop, cs.zrg.stop ) );
}


const Array3D<float>& DataCubes::getCube( int idx ) const
{ return *cubes_[idx]; }


Array3D<float>& DataCubes::getCube( int idx )
{ return *cubes_[idx]; }


void DataCubes::setCube( int idx, const Array3D<float>& cube )
{ cubes_[idx]->copyFrom( cube ); }


CubeSampling DataCubes::cubeSampling() const
{
    CubeSampling res(false);

    if ( inlsz_ && crlsz_ && zsz_ )
    {
	res.hrg.start = BinID( inlsampling.start, crlsampling.start );
	res.hrg.stop = BinID( inlsampling.atIndex(inlsz_-1),
			      crlsampling.atIndex(crlsz_-1) );
	res.hrg.step = BinID( inlsampling.step, crlsampling.step );

	res.zrg.start = z0 * zstep;
	res.zrg.stop = (z0+zsz_-1) * zstep;
	res.zrg.step = zstep;
    }

    return res;
}


}; // namespace Attrib
