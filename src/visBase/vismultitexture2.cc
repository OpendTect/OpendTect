/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vismultitexture2.cc,v 1.11 2006-04-18 14:35:38 cvskris Exp $";


#include "vismultitexture2.h"

#include "arrayndimpl.h"
#include "interpol2d.h"
#include "errh.h"
#include "simpnumer.h"
#include "thread.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "SoMultiTexture2.h"


mCreateFactoryEntry( visBase::MultiTexture2 );

//Remove when interpol works
template <class T>
inline T polyInterpolateDual1D( T v00, T vm10, T v10, T v20,
				T v0m1, T v01, T v02, float x, float y )
{
    const T a1 = v10 - (3*v00 + 2*vm10 + v20) / 6;
    const T a2 = (vm10 + v10) * .5 - v00;
    const T a3 = (3 * v00 + v20 - vm10 - 3 * v10) / 6;

    const T a4 = v01 - (3*v00 + 2*v0m1 + v02) / 6;
    const T a5 = (v0m1 + v01) * .5 - v00;
    const T a6 = (3 * v00 + v02 - v0m1 - 3 * v01) / 6;

    const T sqx = x * x, sqy = y * y;
    return  v00 + a1 * x + a2 * sqx + a3 * x * sqx
		+ a4 * y + a5 * sqy + a6 * y * sqy;
}


template <class T>
inline T polyInterpolate2DDual1D( T v01, T v02,
	                          T v10, T v11, T v12, T v13,
				  T v20, T v21, T v22, T v23,
				  T v31, T v32, float x, float y )
{
    T v1 = polyInterpolateDual1D( v11, v01, v21, v31, v10, v12, v13, x, y );
    T v2 = polyInterpolateDual1D( v22, v32, v12, v02, v23, v21, v20, 1-x, 1-y );    T d1 = x * x + y * y;
    x = 1 - x; y = 1 - y; T d2 = x * x + y * y;
    return (v1 * d2 + v2 * d1) / (d1 + d2);
}



template <class T>
inline T linearInterpolate2D( T v00, T v01, T v10, T v11, float x, float y )
{
    const float xm = 1 - x;
    const float ym = 1 - y;

    return xm*ym*v00 + x*ym*v10 + x*y*v11 + xm*y*v01;
}

//end remove



namespace visBase
{

inline int getPow2Sz( int actsz, bool above=true, int minsz=1,
		      int maxsz=INT_MAX )
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz>1 )
    {
	if ( above && !npowextra && sz % 2 )
	npowextra = 1;
	sz /= 2; npow++;
    }

    sz = intpow( 2, npow + npowextra );
    if ( sz<minsz ) sz = minsz;
    if ( sz>maxsz ) sz = maxsz;
    return sz;
}


inline int nextPower2( int nr, int minnr, int maxnr )
{
    if ( nr > maxnr )
	return maxnr;

    int newnr = minnr;
    while ( nr > newnr )
	newnr *= 2;

    return newnr;
}



MultiTexture2::MultiTexture2()
    : onoff_( new SoSwitch )
    , texture_( new SoMultiTexture2 )
    , complexity_( new SoComplexity )
    , size_( -1, -1 )
{
    onoff_->ref();
    onoff_->addChild( complexity_ );
    complexity_->type.setIgnored( true );
    complexity_->value.setIgnored( true );

    texture_->setNrThreads( Threads::getNrProcessors() );
    onoff_->addChild( texture_ );
}


MultiTexture2::~MultiTexture2()
{ onoff_->unref(); }


SoNode* MultiTexture2::getInventorNode()
{ return onoff_; }


bool MultiTexture2::turnOn( bool yn )
{
    const bool res = isOn();
    onoff_->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;

    return res;
}


bool MultiTexture2::isOn() const
{
    return onoff_->whichChild.getValue()==SO_SWITCH_ALL;
}


void MultiTexture2::clearAll()
{
    size_.row = -1; size_.col = -1;

    for ( int idx=0; idx<nrTextures(); idx++ )
    {
	for ( int idy=0; idy<nrVersions(idx); idy++ )
	{
	    setData( idx, idy, 0 );
	}
    }

}


void MultiTexture2::setTextureTransparency( int texturenr, unsigned char trans )
{
    texture_->opacity.set1Value( texturenr, 255-trans );
}


unsigned char MultiTexture2::getTextureTransparency( int texturenr ) const
{
    return 255-texture_->opacity[texturenr];
}


void MultiTexture2::setOperation( int texturenr, MultiTexture::Operation op )
{
    SoMultiTexture2::Operator nop = SoMultiTexture2::BLEND;
    if ( op==MultiTexture::REPLACE)
	nop = SoMultiTexture2::REPLACE;
    else if ( op==MultiTexture::ADD )
	nop = SoMultiTexture2::ADD;

    texture_->operation.set1Value( texturenr, nop );
}


MultiTexture::Operation MultiTexture2::getOperation( int texturenr ) const
{
    MultiTexture::Operation res;
    if ( texture_->operation[texturenr]==SoMultiTexture2::BLEND )
	res = MultiTexture::BLEND;
    else if ( texture_->operation[texturenr]==SoMultiTexture2::REPLACE )
	res = MultiTexture::REPLACE;
    else
	res = MultiTexture::ADD;

    return res;
}


void MultiTexture2::setTextureRenderQuality( float val )
{
    complexity_->textureQuality.setValue( val );
}


float MultiTexture2::getTextureRenderQuality() const
{
    return complexity_->textureQuality.getValue();
}


bool MultiTexture2::setDataOversample( int texture, int version,
				       int resolution, bool interpol,
	                               const Array2D<float>* data, bool copy )
{
    if ( !data ) return setData( texture, version, data );

    const int datax0size = data->info().getSize(0);
    const int datax1size = data->info().getSize(1);
    if ( datax0size<2 || datax1size<2  )
	return setData( texture, version, data, copy );

    const static int minpix2d = 128;
    const static int maxpix2d = 1024;

    int newx0 = getPow2Sz( datax0size, true, minpix2d, maxpix2d );
    int newx1 = getPow2Sz( datax1size, true, minpix2d, maxpix2d );

    if ( resolution )
    {
	newx0 = nextPower2( datax0size, minpix2d, maxpix2d ) * resolution;
	newx1 = nextPower2( datax1size, minpix2d, maxpix2d ) * resolution;
    }

    if ( !setSize( newx0, newx1 ) )
	return false;

    Array2DImpl<float> interpoldata( newx0, newx1 );
    if ( interpol )
	polyInterp( *data, interpoldata );
    else
	nearestValInterp( *data, interpoldata );

    const int totalsz = interpoldata.info().getTotalSz();
    float* arr = new float[totalsz];
    memcpy( arr, interpoldata.getData(), totalsz*sizeof(float) );
    return setTextureData( texture, version, arr, totalsz, true );
}


bool MultiTexture2::setSize( int sz0, int sz1 )
{
    if ( size_.row==sz0 && size_.col==sz1 )
	return true;

    if ( size_.row>=0 && size_.col>=0 &&
		(nrTextures()>1 || (nrTextures() && nrVersions(0)>1)) )
    {
	pErrMsg("Invalid size" );
	return false;
    }

    size_.row = sz0;
    size_.col = sz1;
    return true;
}

void MultiTexture2::nearestValInterp( const Array2D<float>& inp,
				      Array2D<float>& out ) const
{   
    const int inpsize0 = inp.info().getSize( 0 );
    const int inpsize1 = inp.info().getSize( 1 );
    const int outsize0 = out.info().getSize( 0 );
    const int outsize1 = out.info().getSize( 1 );
    const float x0step = (inpsize0-1)/(float)(outsize0-1);
    const float x1step = (inpsize1-1)/(float)(outsize1-1);

    for ( int x0=0; x0<outsize0; x0++ )
    {
	const int x0sample = mNINT( x0*x0step );
	for ( int x1=0; x1<outsize1; x1++ )
	{
	    const float x1pos = x1*x1step;
	    out.set( x0, x1, inp.get( x0sample, mNINT(x1pos) ) );
	}
    }
}


void MultiTexture2::polyInterp( const Array2D<float>& inp,
				Array2D<float>& out ) const
{
    static const bool newinterpol =
	GetEnvVarYN("MULTI_TEXTURE_USE_NEW_INTERPOL");

    const int inpsize0 = inp.info().getSize( 0 );
    const int inpsize1 = inp.info().getSize( 1 );
    const int outsize0 = out.info().getSize( 0 );
    const int outsize1 = out.info().getSize( 1 );
    const float x0step = (inpsize0-1)/(float)(outsize0-1);
    const float x1step = (inpsize1-1)/(float)(outsize1-1);


    float val; const float udf = mUdf(float);
    Interpolate::PolyReg2DWithUdf<float> interpol;
    int interpolx1=-1;

    for ( int x0=0; x0<outsize0; x0++ )
    {
	const float x0pos=x0*x0step;
	const int x0idx = (int)x0pos;
	const float x0relpos = x0pos-x0idx;
	const bool x0m1udf = x0idx == 0;
	const bool x0p2udf = x0idx >= inpsize0-2;
	const bool x0p1udf = x0idx == inpsize0-1;

	interpolx1=-1;

	for ( int x1=0; x1<outsize1; x1++ )
	{
	    const float x1pos = x1*x1step;
	    const int x1idx = (int)x1pos;
	    const float x1relpos = x1pos-x1idx;

	    if ( newinterpol || interpolx1!=x1idx )
	    {
		const bool x1m1udf = x1idx == 0;
		const bool x1p2udf = x1idx >= inpsize1-2;
		const bool x1p1udf = x1idx == inpsize1-1;

		const float vm10 = x0m1udf ? udf
		    : inp.get( x0idx-1, x1idx );
		const float vm11 = x0m1udf || x1p1udf ? udf
		    : inp.get( x0idx-1, x1idx+1 );
		const float v0m1 = x1m1udf ? udf
		    : inp.get( x0idx, x1idx-1 );
		const float v00 =
		    inp.get( x0idx, x1idx );
		const float v01 = x1p1udf ? udf
		    : inp.get( x0idx, x1idx+1 );
		const float v02 = x1p2udf ? udf
		    : inp.get( x0idx, x1idx+2 );
		const float v1m1 = x0p1udf || x1m1udf ? udf
		    : inp.get( x0idx+1, x1idx-1 );
		const float v10 = x0p1udf ? udf
		    : inp.get( x0idx+1, x1idx );
		const float v11 = x0p1udf || x1p1udf ? udf
		    : inp.get( x0idx+1, x1idx+1 );
		const float v12 = x0p1udf || x1p2udf ? udf
		    : inp.get( x0idx+1, x1idx+2 );
		const float v20 = x0p2udf ? udf
		    : inp.get( x0idx+2, x1idx );
		const float v21 = x0p2udf || x1p1udf ? udf
		    : inp.get( x0idx+2, x1idx+1 );

		if ( newinterpol )
		{
		    interpol.set( vm10, vm11,
			    v0m1, v00,  v01,  v02,
			    v1m1, v10,  v11,  v12,
				  v20,  v21 );

		    interpolx1 = x1idx;
		}
		else
		{
		    if ( Values::isUdf(v00) || Values::isUdf(v01) ||
			Values::isUdf(v10) || Values::isUdf(v11) )
		    {
			val = x0relpos > 0.5 ? ( x1relpos > 0.5 ? v11 : v10 )
					    : ( x1relpos > 0.5 ? v01 : v00 );
		    }
		    else if ( Values::isUdf(vm10) || Values::isUdf(vm11) ||
			    Values::isUdf(v0m1) || Values::isUdf(v02) ||
			    Values::isUdf(v1m1) || Values::isUdf(v12) ||
			    Values::isUdf(v20) || Values::isUdf(v21) )
		    {
			val = linearInterpolate2D( v00, v01, v10, v11,
						    x0relpos, x1relpos );
		    }
		    else
		    {
			val = polyInterpolate2DDual1D( vm10, vm11, v0m1,
						       v00, v01, v02,
						       v1m1, v10, v11,
						       v12, v20, v21,
						       x0relpos, x1relpos );
		    }
		}
	    }

	    if ( newinterpol )
		val = interpol.apply( x0relpos, x1relpos );

	    out.set( x0, x1, val );
	}
    }
}


bool MultiTexture2::setData( int texture, int version,
			     const Array2D<float>* data, bool copy )
{
    if ( data && !setSize( data->info().getSize(0), data->info().getSize(1) ) )
	return false;

    const int totalsz = data ? data->info().getTotalSz() : 0;
    const float* dataarray = data ? data->getData() : 0;
    bool manage = false;
    if ( data && (!dataarray || copy ) )
    {
	float* arr = new float[totalsz];

	if ( data->getData() )
	    memcpy( arr, data->getData(), totalsz*sizeof(float) );
	else
	{
	    ArrayNDIter iter( data->info() );
	    int idx=0;
	    do
	    {
		arr[idx++] = data->get(iter.getPos());
	    } while ( iter.next() );
	}

	manage = true;
	dataarray = arr;
    }

    return setTextureData( texture, version, dataarray, totalsz, manage );
}


bool MultiTexture2::setIndexData( int texture, int version,
				  const Array2D<unsigned char>* data )
{
    const int totalsz = data ? data->info().getTotalSz() : 0;
    const unsigned char* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	unsigned char* arr = new unsigned char[totalsz];
	ArrayNDIter iter( data->info() );
	int idx=0;
	do
	{
	    arr[idx++] = data->get(iter.getPos());
	} while ( iter.next() );

	manage = true;
	dataarray = arr;
    }

    return setTextureIndexData( texture, version, dataarray, totalsz, manage );
}


void MultiTexture2::updateSoTextureInternal( int texturenr )
{
    const unsigned char* texture = getCurrentTextureIndexData(texturenr);
    if ( size_.row<0 || size_.col<0 || !texture )
    {
	texture_->component.set1Value( texturenr, 0 );
	return;
    }

    const SbImage image( texture, SbVec2s(size_.col,size_.row), 1 );
    texture_->image.set1Value( texturenr, image );
    updateColorTables();
}


void MultiTexture2::updateColorTables()
{
    int totalnr = 0;
    const int nrtextures = nrTextures();
    for ( int idx=0; idx<nrtextures; idx++ )
	totalnr += getColorTab( idx ).nrSteps() + 1;

    unsigned char* arrstart = 0;

    SbVec2s cursize;
    int curnc;
    bool finishedit = false;
    unsigned char* curarr = texture_->colors.startEditing( cursize, curnc );
    if ( curnc==4 && cursize[1]==totalnr )
    {
	arrstart = curarr;
	finishedit = true;
    }
    else
	arrstart = new unsigned char[totalnr*4];

    unsigned char* arr = arrstart;

    texture_->numcolor.deleteValues( nrtextures, -1 );
    texture_->component.deleteValues( nrtextures, -1 );

    for ( int idx=0; idx<nrtextures; idx++ )
    {
	if ( !isTextureEnabled(idx) || !getCurrentTextureIndexData(idx) )
	{
	    texture_->component.set1Value( idx, 0 );
	    continue;
	}

	const VisColorTab& ctab = getColorTab( idx );
	const int nrsteps = ctab.nrSteps();

	texture_->numcolor.set1Value( idx, nrsteps+1 ); //one extra for udf
	for ( int idy=0; idy<=nrsteps; idy++ )
	{
	    const Color col = ctab.tableColor( idy );
	    *(arr++) = col.r();
	    *(arr++) = col.g();
	    *(arr++) = col.b();
	    *(arr++) = 255-col.t();
	}

	SoMultiTexture2::Operator op = SoMultiTexture2::BLEND;
	if ( !idx || getOperation(idx)==MultiTexture::REPLACE)
	    op = SoMultiTexture2::REPLACE;
	else if ( getOperation(idx)==MultiTexture::ADD )
	    op = SoMultiTexture2::ADD;

	texture_->component.set1Value( idx, getComponents(idx) );
    }

    if ( finishedit )
	texture_->colors.finishEditing();
    else
	texture_->colors.setValue( SbVec2s(totalnr,1), 4, arrstart,
				  SoSFImage::NO_COPY_AND_DELETE );
}

	
void MultiTexture2::insertTextureInternal( int texturenr )
{
    texture_->image.insertSpace( texturenr, 1 );
    updateSoTextureInternal( texturenr );
}


void MultiTexture2::removeTextureInternal( int texturenr )
{
    texture_->image.deleteValues( texturenr, 1 );
}


}; //namespace




