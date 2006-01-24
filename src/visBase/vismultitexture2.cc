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
    : onoff( new SoSwitch )
    , texture( new SoMultiTexture2 )
    , complexity( new SoComplexity )
    , size( 0, 0 )
{
    onoff->ref();
    onoff->addChild( complexity );
    complexity->type.setIgnored( true );
    complexity->value.setIgnored( true );
    onoff->addChild( texture );
}


MultiTexture2::~MultiTexture2()
{ onoff->unref(); }


SoNode* MultiTexture2::getInventorNode()
{ return onoff; }


bool MultiTexture2::turnOn( bool yn )
{
    const bool res = isOn();
    onoff->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;

    return res;
}


bool MultiTexture2::isOn() const
{
    return onoff->whichChild.getValue()==SO_SWITCH_ALL;
}


void MultiTexture2::setTextureRenderQuality( float val )
{
    complexity->textureQuality.setValue( val );
}


float MultiTexture2::getTextureRenderQuality() const
{
    return complexity->textureQuality.getValue();
}


bool MultiTexture2::setData( int texture, int version,
			     const Array2D<float>* data )
{
    if ( (data && size.row!=data->info().getSize(0)) ||
         (data && size.col!=data->info().getSize(1)) )
    {
	if ( nrTextures()>1 || (nrTextures() && nrVersions(0)>1) )
	{
	    pErrMsg("Invalid size" );
	    return false;
	}
	else
	{
	    size[0] = data->info().getSize(0);
	    size[1] = data->info().getSize(1);
	}
    }

    const int size = data ? data->info().getTotalSz() : 0;
    const float* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	float* arr = new float[size];
	ArrayNDIter iter( data->info() );
	int idx=0;
	do
	{
	    arr[idx++] = data->get(iter.getPos());
	} while ( iter.next() );

	manage = true;
	dataarray = arr;
    }

    return setTextureData( texture, version, dataarray, size, manage );
}


bool MultiTexture2::setIndexData( int texture, int version,
				  const Array2D<unsigned char>* data )
{
    const int size = data ? data->info().getTotalSz() : 0;
    const unsigned char* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	unsigned char* arr = new unsigned char[size];
	ArrayNDIter iter( data->info() );
	int idx=0;
	do
	{
	    arr[idx++] = data->get(iter.getPos());
	} while ( iter.next() );

	manage = true;
	dataarray = arr;
    }

    return setTextureIndexData( texture, version, dataarray, size, manage );
}


void MultiTexture2::updateSoTextureInternal( int texturenr )
{
    const SbImage image( getCurrentTextureIndexData(texturenr),
	    		 SbVec2s(size[1],size[0]), 1 );
    texture->image.set1Value( texturenr, image );
    updateColorTables();
}


void MultiTexture2::updateColorTables()
{
    int totalnr = 0;
    const int nrtextures = nrTextures();
    for ( int idx=0; idx<nrtextures; idx++ )
	totalnr += getColorTab( idx ).nrSteps();

    unsigned char* arrstart = 0;

    SbVec2s cursize;
    int curnc;
    bool finishedit = false;
    unsigned char* curarr = texture->colors.startEditing( cursize, curnc );
    if ( curnc==4 && cursize[1]==totalnr )
    {
	arrstart = curarr;
	finishedit = true;
    }
    else
	arrstart = new unsigned char[totalnr*4];

    unsigned char* arr = arrstart;

    texture->numcolor.deleteValues( nrtextures, -1 );
    texture->operation.deleteValues( nrtextures, -1 );
    texture->component.deleteValues( nrtextures, -1 );

    for ( int idx=0; idx<nrtextures; idx++ )
    {
	const VisColorTab& ctab = getColorTab( idx );
	const int nrsteps = ctab.nrSteps();

	texture->numcolor.set1Value( idx, nrsteps );

	SoMultiTexture2::Operator op = SoMultiTexture2::BLEND;
	if ( !idx || getOperation(idx)==MultiTexture::REPLACE)
	    op = SoMultiTexture2::REPLACE;
	else if ( getOperation(idx)==MultiTexture::ADD )
	    op = SoMultiTexture2::ADD;

	texture->operation.set1Value( idx, op );
	texture->component.set1Value( idx, getComponents(idx) );
	
	for ( int idy=0; idy<nrsteps; idy++ )
	{
	    const Color col = ctab.tableColor( idy );
	    *(arr++) = col.r();
	    *(arr++) = col.g();
	    *(arr++) = col.b();
	    *(arr++) = 255-col.t();
	}
    }

    if ( finishedit )
	texture->colors.finishEditing();
    else
	texture->colors.setValue( SbVec2s(totalnr,1), 4, arrstart,
				  SoSFImage::NO_COPY_AND_DELETE );
}

	
void MultiTexture2::insertTextureInternal( int texturenr )
{
    texture->image.insertSpace( texturenr, 1 );
    updateSoTextureInternal( texturenr );
}


void MultiTexture2::removeTextureInternal( int texturenr )
{
    texture->image.deleteValues( texturenr, 1 );
}


}; //namespace




