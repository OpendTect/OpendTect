/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika S
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "SbImagei32.h"

#include <Inventor/errors/SoDebugError.h>
#include <string.h>

// Default constructor.
SbImagei32::SbImagei32() : bytes( NULL ), datatype( SETVALUEPTR_DATA ), 
       			   size( 0, 0, 0 ), bpp( 0 )
#ifdef SBIMAGEI32_THREADSAFE
			   , rwmutex( SbRWMutex::READ_PRECEDENCE )
#endif
{ }


// Constructor which sets 2D data using setValue().
SbImagei32::SbImagei32( const unsigned char* data, const SbVec2i32& sz, 
	const int bytesperpixel ) : bytes( NULL )
#ifdef SBIMAGEI32_THREADSAFE
				    , rwmutex( SbRWMutex::READ_PRECEDENCE )
#endif
{
    setValue( sz, bytesperpixel, data );
}


// Constructor which sets 3D data using setValue().
SbImagei32::SbImagei32( const unsigned char *data, const SbVec3i32& sz, 
	const int bytesperpixel ) : bytes( NULL )
#ifdef SBIMAGEI32_THREADSAFE
				    , rwmutex( SbRWMutex::READ_PRECEDENCE )
#endif
{
    setValue( sz, bytesperpixel, data );
}


SbImagei32::~SbImagei32()
{
    freeData();
}


void SbImagei32::freeData()
{
    if ( bytes )
	switch ( datatype )
	{
	    default:
		SoDebugError::postWarning( "SbImagei32::freeData",
					   "Unknown data type!" );
		break;

	    case INTERNAL_DATA:
		delete[] bytes;
		bytes = NULL;
		break;

	    case SETVALUEPTR_DATA:
		bytes = NULL;
		break;
	}

    datatype = SETVALUEPTR_DATA;
}


// Convenience 2D version of setValuePtr.
void SbImagei32::setValuePtr( const SbVec2i32& sz, const int bytesperpixel,
	const unsigned char* data )
{
    SbVec3i32 tmpsize( sz[0], sz[1], 0 );
    setValuePtr( tmpsize, bytesperpixel, data );
}


// Sets the image data without copying the data. "data" will be used directly,
// and will not be freed when the image instance is destructed. If the depth 
// of the image (size[2]) is zero, the image is considered a 2D image.
void SbImagei32::setValuePtr( const SbVec3i32& sz, const int bytesperpixel,
	const unsigned char* data )
{
    writeLock();
    freeData();
    bytes = const_cast<unsigned char *> (data);
    datatype = SETVALUEPTR_DATA;
    size = sz;
    bpp = bytesperpixel;
    writeUnlock();
}


// Convenience 2D version of setValue.
bool SbImagei32::setValue( const SbVec2i32& sz, const int bytesperpixel,
	const unsigned char* data )
{
    SbVec3i32 tmpsize( sz[0], sz[1], 0 );
    return setValue( tmpsize, bytesperpixel, data );
}


// Sets the image to size and bytesperpixel. If bytes != NULL, data are copied 
// from data into this class' image data. If bytes == NULL, the image data are
// left uninitialized.
// If the depth of the image (size[2]) is zero, the image is considered a 2D 
// image.
bool SbImagei32::setValue( const SbVec3i32& sz, const int bytesperpixel,
	const unsigned char* data )
{
    bool ret = true;
    int buffersize = int(sz[0]) * int(sz[1]) * int(sz[2]==0?1:sz[2]) * 
	bytesperpixel;
    
    writeLock();
    if ( bytes && datatype == INTERNAL_DATA )
    {
	// check for special case where we don't have to reallocate
	if ( data && ( sz == size ) && 
	     ( bytesperpixel == bpp ) )
	{
	    memcpy( bytes, data, buffersize );
	    writeUnlock();
	    return ret;
	}
    }

    freeData();
    size = sz;
    bpp = bytesperpixel;

    if ( buffersize )
    {
	bytes = new unsigned char[buffersize];
	datatype = INTERNAL_DATA;

	if ( bytes )
	{
	    if ( data )
		memcpy( bytes, data, buffersize );
	}
	else
	{
	    SbString msg( "Unable to allocate memory! ");
	    msg += sz[0];
	    msg += " ";
	    msg += sz[1];
	    msg += " ";
	    msg += sz[2];
	    SoDebugError::postWarning( "SbImagei32::setValue", msg.getString());
	    ret = false;
	}
    }

    writeUnlock();
    return ret;
}


// Returns the 2D image data.
unsigned char* SbImagei32::getValue( SbVec2i32& sz, int& bytesperpixel ) const
{
    SbVec3i32 tmpsize;
    unsigned char *data = getValue( tmpsize, bytesperpixel );
    sz.setValue( tmpsize[0], tmpsize[1] );
    return data;
}


// Returns the 3D image data.
unsigned char* SbImagei32::getValue( SbVec3i32& sz, int& bytesperpixel ) const
{
    readLock();
    sz = size;
    bytesperpixel = bpp;
    unsigned char *data = bytes;
    readUnlock();
    return data;
}


// Compare image with the image in this class and return TRUE if they are equal.
int SbImagei32::operator == ( const SbImagei32& image ) const
{
    readLock();
    
    int ret = 0;

    if ( size != image.size ) ret = 0;
    else if ( bpp != image.bpp ) ret = 0;
    else if ( bytes == NULL || 
	    image.bytes == NULL )
	ret = ( bytes == image.bytes );
    else
    {
	ret = memcmp( bytes, image.bytes,
		int(size[0]) *
		int(size[1]) *
		int(size[2]==0?1:size[2]) * 
		bpp ) == 0;
    }

    readUnlock();
    return ret;
}


//  Assignment operator.
SbImagei32& SbImagei32::operator = ( const SbImagei32& image )
{
    if ( *this != image )
    {
	writeLock();
	freeData();
	writeUnlock();

	if ( image.bytes )
	{
	    image.readLock();

	    switch ( image.datatype )
	    {
		default:
		    SoDebugError::postWarning( "SbImagei32::freeData",
					       "Unknown data type!" );
		    break;

		case INTERNAL_DATA:
		    // need to copy data 
		    setValue( image.size, image.bpp,
			      image.bytes );
		    break;

		case SETVALUEPTR_DATA:
		    // just set the data ptr
		    setValuePtr( image.size, image.bpp,
			    image.bytes );
		    break;
	    }
	    image.readUnlock();
	}
    }

    return *this;
}


// Returns TRUE if the image is not empty. 
SbBool SbImagei32::hasData() const
{
    SbBool ret;
    readLock();
    ret = bytes != NULL;
    readUnlock();
    return ret;
}


// Returns the size of the image. If this is a 2D image, the z component is 
// zero. If this is a 3D image, the z component is  >= 1.
SbVec3i32 SbImagei32::getSize() const
{
    return size;
}


void SbImagei32::readLock() const
{
#ifdef SBIMAGEI32_THREADSAFE
    rwmutex.readLock();
#endif
}

void SbImagei32::readUnlock() const
{
#ifdef SBIMAGEI32_THREADSAFE
    rwmutex.readUnlock();
#endif
}
  
void SbImagei32::writeLock()
{
#ifdef SBIMAGEI32_THREADSAFE
    rwmutex.writeLock();
#endif
}

void SbImagei32::writeUnlock()
{
#ifdef SBIMAGEI32_THREADSAFE
    rwmutex.writeUnlock();
#endif
}

