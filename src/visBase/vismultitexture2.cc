/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

#include "vismultitexture2.h"

#include "arraynd.h"
#include "errh.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "SoMultiTexture2.h"


mCreateFactoryEntry( visBase::MultiTexture2 );

namespace visBase
{

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


void MultiTexture2::setTextureRenderQuality( float val )
{
    complexity_->textureQuality.setValue( val );
}


float MultiTexture2::getTextureRenderQuality() const
{
    return complexity_->textureQuality.getValue();
}


bool MultiTexture2::setData( int texture, int version,
			     const Array2D<float>* data )
{
    if ( (data && size_.row!=data->info().getSize(0)) ||
         (data && size_.col!=data->info().getSize(1)) )
    {
	if ( size_.row>=0 && size_.col>=0 &&
		(nrTextures()>1 || (nrTextures() && nrVersions(0)>1)) )
	{
	    pErrMsg("Invalid size" );
	    return false;
	}
	else
	{
	    size_.row = data->info().getSize(0);
	    size_.col = data->info().getSize(1);
	}
    }

    const int totalsz = data ? data->info().getTotalSz() : 0;
    const float* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	float* arr = new float[totalsz];
	ArrayNDIter iter( data->info() );
	int idx=0;
	do
	{
	    arr[idx++] = data->get(iter.getPos());
	} while ( iter.next() );

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
    texture_->operation.deleteValues( nrtextures, -1 );
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

	texture_->operation.set1Value( idx, op );
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




