/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoMFImage.cc,v 1.9 2010-10-26 21:23:11 cvskarthika Exp $";


#include "SoMFImage.h"

#include <Inventor/errors/SoReadError.h>

SO_MFIELD_SOURCE( SoMFImage, SbImage, const SbImage& );

void SoMFImage::initClass()
{
    SO_MFIELD_INIT_CLASS( SoMFImage, SoMField );
}


#define mErrRet { SoReadError::post(in, prematuremsg ); return false; }


SbBool SoMFImage::read1Value( SoInput* in, int index )
{
    SbVec3s size;
    int nc;

    static const char* prematuremsg = "Premature end of file";

    if ( !in->read( size[0] ) || !in->read( size[1] ) || !in->read( size[2] ) ||
         !in->read( nc ) )
	mErrRet;

    if ( size[0]<0 || size[1]<0 || size[2]<0 || nc<0 || nc>4 )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }

    const int buffersize = size[0] * size[1] * size[2] * nc;

    if ( !buffersize && (size[0] || size[1] || size[2] || nc) )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }


    if ( !buffersize ) 
    {
	values[index].setValue( SbVec3s(0,0,0), 0, NULL );
	return true;
    }

    values[index].setValue( size, nc, 0 );
    unsigned char* pixblock = values[index].getValue( size, nc );

    if ( in->isBinary() && in->getIVVersion() >= 2.1f)
    {
	if ( !in->readBinaryArray( pixblock,buffersize ) )
	    mErrRet;
    }
    else
    {
	int byte = 0;
	const int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int l;
	    if ( !in->read( l ) )
		mErrRet;

	    for ( int j=0; j<nc; j++ )
		pixblock[byte++] =
		    (unsigned char) ((l >> (8 * (nc-j-1))) & 0xFF);
	}
    }

    return true;
}


#define mWriteSpace if ( !binary ) out->write(' ')

void SoMFImage::write1Value( SoOutput* out, int index ) const
{
    int nc;
    SbVec3s size;
    unsigned char * pixblock = values[index].getValue( size, nc );

    const bool binary = out->isBinary();

    out->write( size[0] ); mWriteSpace;
    out->write( size[1] ); mWriteSpace;
    out->write( size[2] ); mWriteSpace;
    out->write( nc );


    if ( out->isBinary() )
    {
	int buffersize = size[0] * size[1] * size[2] * nc;
	if ( buffersize )
	{
	    out->writeBinaryArray( pixblock, buffersize );
	    int padsize = ((buffersize + 3) / 4) * 4 - buffersize;
	    if ( padsize )
	    {
		unsigned char pads[3] = {'\0','\0','\0'};
		out->writeBinaryArray( pads, padsize );
	    }
	}
    }
    else
    {
	out->write( '\n' );
	out->indent();

	int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int data = 0;
	    for ( int j=0; j<nc; j++ )
	    {
		if (j) data <<= 8;
		data |= (uint32_t)(pixblock[i * nc + j]);
	    }

	    out->write( data );
	    if ( ((i+1)%8 == 0) && (i+1 != numpixels))
	    {
		out->write( '\n' );
		out->indent();
	    }
	    else
	    {
		out->write( ' ' );
	    }
	}
    }
}



SO_MFIELD_SOURCE( SoMFImagei32, SbImagei32, const SbImagei32& );

void SoMFImagei32::initClass()
{
    SO_MFIELD_INIT_CLASS( SoMFImagei32, SoMField );
}


SbBool SoMFImagei32::read1Value( SoInput* in, int index )
{
    SbVec3i32 size;
    int nc;

    static const char* prematuremsg = "Premature end of file";

    if ( !in->read( size[0] ) || !in->read( size[1] ) || !in->read( size[2] ) ||
         !in->read( nc ) )
	mErrRet;

    if ( size[0]<0 || size[1]<0 || size[2]<0 || nc<0 || nc>4 )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }

    const double buffersize = size[0] * size[1] * size[2] * nc;

    const int maxint = (int) (pow( 2.0, (int) ((sizeof(int)*8)-1) ) - 1);
    if ( buffersize >= maxint )
    {
	SoReadError::post( in, "Image too large to be handled: %dx%dx%dx%d",
		size[0], size[1], size[2], nc );
	return false;
    }

    if ( !buffersize && (size[0] || size[1] || size[2] || nc) )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }


    if ( !buffersize ) 
    {
	values[index].setValue( SbVec3i32(0,0,0), 0, NULL );
	return true;
    }

    values[index].setValue(size, nc, 0 );
    unsigned char* pixblock = values[index].getValue( size, nc );

    if ( in->isBinary() && in->getIVVersion() >= 2.1f)
    {
	if ( !in->readBinaryArray( pixblock, (int) buffersize ) )
	    mErrRet;
    }
    else
    {
	int byte = 0;
	const int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int l;
	    if ( !in->read( l ) )
		mErrRet;

	    for ( int j=0; j<nc; j++ )
		pixblock[byte++] =
		    (unsigned char) ((l >> (8 * (nc-j-1))) & 0xFF);
	}
    }

    return true;
}


void SoMFImagei32::write1Value( SoOutput* out, int index ) const
{
    int nc;
    SbVec3i32 size;
    unsigned char * pixblock = values[index].getValue( size, nc );

    const bool binary = out->isBinary();

    out->write( size[0] ); mWriteSpace;
    out->write( size[1] ); mWriteSpace;
    out->write( size[2] ); mWriteSpace;
    out->write( nc );


    if ( out->isBinary() )
    {
	int buffersize = size[0] * size[1] * size[2] * nc;
	if ( buffersize )
	{
	    out->writeBinaryArray( pixblock, buffersize );
	    int padsize = ((buffersize + 3) / 4) * 4 - buffersize;
	    if ( padsize )
	    {
		unsigned char pads[3] = {'\0','\0','\0'};
		out->writeBinaryArray( pads, padsize );
	    }
	}
    }
    else
    {
	out->write( '\n' );
	out->indent();

	int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int data = 0;
	    for ( int j=0; j<nc; j++ )
	    {
		if ( j ) data <<= 8;
		data |= (uint32_t)(pixblock[i * nc + j]);
	    }

	    out->write( data );
	    if ( ((i+1)%8 == 0) && (i+1 != numpixels))
	    {
		out->write( '\n' );
		out->indent();
	    }
	    else
	    {
		out->write( ' ' );
	    }
	}
    }
}

