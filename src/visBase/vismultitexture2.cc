/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vismultitexture2.cc,v 1.18 2007-01-24 20:06:51 cvskris Exp $";


#include "vismultitexture2.h"

#include "arrayndimpl.h"
#include "array2dresample.h"
#include "interpol2d.h"
#include "errh.h"
#include "simpnumer.h"
#include "thread.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "SoMultiTexture2.h"


mCreateFactoryEntry( visBase::MultiTexture2 );


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
    turnOn( true );
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
    while ( texture_->opacity.getNum()<texturenr )
	texture_->opacity.set1Value( texture_->opacity.getNum(), 255 );

    texture_->opacity.set1Value( texturenr, 255-trans );
}


unsigned char MultiTexture2::getTextureTransparency( int texturenr ) const
{
    if ( texturenr>=texture_->opacity.getNum() )
	return 0;

    return 255-texture_->opacity[texturenr];
}


void MultiTexture2::setOperation( int texturenr, MultiTexture::Operation op )
{
    SoMultiTexture2::Operator nop = SoMultiTexture2::BLEND;
    if ( op==MultiTexture::REPLACE)
	nop = SoMultiTexture2::REPLACE;
    else if ( op==MultiTexture::ADD )
	nop = SoMultiTexture2::ADD;

    while ( texture_->operation.getNum()<texturenr )
	texture_->operation.set1Value( texture_->operation.getNum(),
				       SoMultiTexture2::BLEND  );

    texture_->operation.set1Value( texturenr, nop );
}


MultiTexture::Operation MultiTexture2::getOperation( int texturenr ) const
{
    if ( texturenr>=texture_->operation.getNum() ||
	 texture_->operation[texturenr]==SoMultiTexture2::BLEND )
	return MultiTexture::BLEND;
    else if ( texture_->operation[texturenr]==SoMultiTexture2::REPLACE )
	return MultiTexture::REPLACE;

    return MultiTexture::ADD;
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
    Array2DReSampler<float,float> resampler( inp, out, true );
    resampler.execute();
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
	texture_->enabled.set1Value( texturenr, false );
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

    if ( texture_->numcolor.getNum()>nrtextures )
	texture_->numcolor.deleteValues( nrtextures, -1 );
    if ( texture_->component.getNum()>nrtextures )
	texture_->component.deleteValues( nrtextures, -1 );
    if ( texture_->enabled.getNum()>nrtextures )
	texture_->enabled.deleteValues( nrtextures, -1 );

    for ( int idx=0; idx<nrtextures; idx++ )
    {
	if ( !isTextureEnabled(idx) || !getCurrentTextureIndexData(idx) )
	{
	    texture_->enabled.set1Value( idx, false );
	    continue;
	}

	texture_->enabled.set1Value( idx, true );

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
    if ( texture_->image.getNum()>texturenr )
	texture_->image.deleteValues( texturenr, 1 );

    updateColorTables();
}


}; //namespace




