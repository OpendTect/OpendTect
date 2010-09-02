/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika S
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SbImagei32.cc,v 1.2 2010-09-02 15:00:58 cvskris Exp $";

#include "SbImagei32.h"

#include <cstring>
#include <cstdlib>

#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec3i32.h>
#include <Inventor/SbString.h>
#include <Inventor/SoInput.h> // for SoInput::searchForFile()
#include <Inventor/lists/SbStringList.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>

/*#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
*/

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbRWMutex.h>
#endif // COIN_THREADSAFE

//#include "glue/simage_wrapper.h"
//#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::memcmp;
using std::memcpy;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS


class SbImagei32P
{
public:
    typedef struct
    {
	SbImagei32ReadImageCB*	cb;
	void*			closure;
    } ReadImageCBData;

    enum 	DataType { INTERNAL_DATA, SIMAGE_DATA, SETVALUEPTR_DATA };
    
    		SbImagei32P() : bytes( NULL ),
			datatype( SETVALUEPTR_DATA ), size( 0, 0, 0 ), 
			bpp( 0 ), schedulecb( NULL )
#ifdef COIN_THREADSAFE
			, rwmutex( SbRWMutex::READ_PRECEDENCE )
#endif // COIN_THREADSAFE
			{ }

    void		freeData()
    {
	if ( bytes )
	{
	    switch ( datatype )
	    {
		default:
		    assert(0 && "unknown data type");
		    break;

		case INTERNAL_DATA:
		    delete[] bytes;
		    bytes = NULL;
		    break;

		case SETVALUEPTR_DATA:
		    bytes = NULL;
		    break;
	    }
	}

	datatype = SETVALUEPTR_DATA;
    }

  unsigned char*	bytes;
  DataType		datatype;
  SbVec3i32		size;
  int			bpp;
  SbString		schedulename;
  SbImagei32ScheduleReadCB*	schedulecb;
  void*			scheduleclosure;

  static SbList <ReadImageCBData>*	readimagecallbacks;

#ifdef COIN_THREADSAFE
  SbRWMutex	rwmutex;
  void readLock()
  {
    //    fprintf(stderr,"readlock: %p\n", this);
    rwmutex.readLock();
    //fprintf(stderr,"readlock achieved: %p\n", this);
  }
  
  void readUnlock()
  {
    //fprintf(stderr,"readUnlock: %p\n", this);
    rwmutex.readUnlock();
  }
  
  void writeLock()
  {
    //fprintf(stderr,"writelock: %p\n", this);
    rwmutex.writeLock();
    //fprintf(stderr,"writelock achived: %p\n", this);
  }

  void writeUnlock()
  {
    //fprintf(stderr,"writeUnlock: %p\n", this);
    rwmutex.writeUnlock();
  }
#else // COIN_THREADSAFE

  void			readLock() { }
  void			readUnlock() { }
  void			writeLock() { }
  void			writeUnlock(void) { }
#endif // ! COIN_THREADSAFE

};

extern "C" {

static void		SbImagei32_cleanup_callback()
{
  delete SbImagei32P::readimagecallbacks;
  SbImagei32P::readimagecallbacks = NULL;
}

} // extern "C"

SbList <SbImagei32P::ReadImageCBData>* SbImagei32P::readimagecallbacks = NULL;


#define PRIVATE(image) ((image)->pimpl)


//  Default constructor.
SbImagei32::SbImagei32()
{
    PRIVATE(this) = new SbImagei32P;
}


//  Constructor which sets 2D data using setValue().
SbImagei32::SbImagei32( const unsigned char* bytes, const SbVec2i32& size, 
	const int bytesperpixel )
{
    PRIVATE(this) = new SbImagei32P;
    setValue( size, bytesperpixel, bytes );
}


//  Constructor which sets 3D data using setValue().
SbImagei32::SbImagei32( const unsigned char *bytes, const SbVec3i32& size, 
	const int bytesperpixel )
{
    PRIVATE(this) = new SbImagei32P;
    setValue(size, bytesperpixel, bytes);
}


SbImagei32::~SbImagei32()
{
    PRIVATE(this)->freeData();
    delete PRIVATE(this);
}


// Apply a read lock on this image. This will make it impossible for other 
// threads to change the image while this lock is active. Other threads can do 
// read-only operations on this image, of course.
// For the single thread version of Coin, this method does nothing.
void SbImagei32::readLock() const
{
    PRIVATE(this)->readLock();
}


// Release a read lock on this image.
//  For the single thread version of Coin, this method does nothing.
void SbImagei32::readUnlock() const
{
    PRIVATE(this)->readUnlock();
}


// Convenience 2D version of setValuePtr.
void SbImagei32::setValuePtr( const SbVec2i32& size, const int bytesperpixel,
	const unsigned char* bytes )
{
    SbVec3i32 tmpsize( size[0], size[1], 0 );
    setValuePtr( tmpsize, bytesperpixel, bytes );
}


// Sets the image data without copying the data. "bytes" will be used directly,
// and the data will not be freed when the image instance is destructed. If the
// depth of the image (size[2]) is zero, the image is considered a 2D image.
void SbImagei32::setValuePtr( const SbVec3i32& size, const int bytesperpixel,
	const unsigned char* bytes )
{
    PRIVATE(this)->writeLock();
    PRIVATE(this)->schedulename = "";
    PRIVATE(this)->schedulecb = NULL;
    PRIVATE(this)->freeData();
    PRIVATE(this)->bytes = const_cast<unsigned char *> (bytes) ;
    PRIVATE(this)->datatype = SbImagei32P::SETVALUEPTR_DATA;
    PRIVATE(this)->size = size;
    PRIVATE(this)->bpp = bytesperpixel;
    PRIVATE(this)->writeUnlock();
}


// Convenience 2D version of setValue.
void SbImagei32::setValue( const SbVec2i32& size, const int bytesperpixel,
	const unsigned char* bytes )
{
    SbVec3i32 tmpsize( size[0], size[1], 0 );
    setValue( tmpsize, bytesperpixel, bytes );
}


// Sets the image to size and bytesperpixel. If bytes != NULL, data are copied 
// from bytes into this class' image data. If bytes == NULL, the image data are
// left uninitialized.
// The image data will always be allocated in multiples of four. This means 
// that if you set an image with size == (1,1,1) and bytesperpixel == 1, four 
// bytes will be allocated to hold the data. This is mainly done to simplify 
// the export code in SoSFImage and normally you'll  not have to worry about 
// this feature.
// If the depth of the image (size[2]) is zero, the image is considered a 2D 
// image.
void SbImagei32::setValue( const SbVec3i32& size, const int bytesperpixel,
	const unsigned char* bytes )
{
    PRIVATE(this)->writeLock();
    PRIVATE(this)->schedulename = "";
    PRIVATE(this)->schedulecb = NULL;
    if ( PRIVATE(this)->bytes && 
	 PRIVATE(this)->datatype == SbImagei32P::INTERNAL_DATA )
    {
	// check for special case where we don't have to reallocate
	if ( bytes && ( size == PRIVATE(this)->size) && 
	     ( bytesperpixel == PRIVATE(this)->bpp) )
	{
	    memcpy( PRIVATE(this)->bytes, bytes, 
		    int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) *
		    bytesperpixel );
	    PRIVATE(this)->writeUnlock();
	    return;
	}
    }

    PRIVATE(this)->freeData();
    PRIVATE(this)->size = size;
    PRIVATE(this)->bpp = bytesperpixel;
    int buffersize = int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) * 
	bytesperpixel;

    if (buffersize)
    {
	// Align buffers because the binary file format has the data aligned
	// (simplifies export code in SoSFImage).
	buffersize = ((buffersize + 3) / 4) * 4;
	PRIVATE(this)->bytes = new unsigned char[buffersize];
	PRIVATE(this)->datatype = SbImagei32P::INTERNAL_DATA;

	if (bytes)
	{
	    // Important: don't copy buffersize num bytes here!
	    (void)memcpy(PRIVATE(this)->bytes, bytes,
		    int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) * 
		    bytesperpixel);
	}
    }

    PRIVATE(this)->writeUnlock();
}


// Returns the 2D image data.
unsigned char* SbImagei32::getValue( SbVec2i32& size, int& bytesperpixel ) const
{
    SbVec3i32 tmpsize;
    unsigned char *bytes = getValue( tmpsize, bytesperpixel );
    size.setValue( tmpsize[0], tmpsize[1] );
    return bytes;
}


// Returns the 3D image data.
unsigned char* SbImagei32::getValue( SbVec3i32& size, int& bytesperpixel ) const
{
    PRIVATE(this)->readLock();
    if (PRIVATE(this)->schedulecb)
    {
	// start a thread to read the image.
	SbBool scheduled = PRIVATE(this)->schedulecb(
		PRIVATE(this)->schedulename, const_cast<SbImagei32 *>(this),
		PRIVATE(this)->scheduleclosure);
	if (scheduled) 
	{
	    PRIVATE(this)->schedulecb = NULL;
	}
    }

    size = PRIVATE(this)->size;
    bytesperpixel = PRIVATE(this)->bpp;
    unsigned char * bytes = PRIVATE(this)->bytes;
    PRIVATE(this)->readUnlock();
    return bytes;
}


// Given a basename for a file and and array of directories to search (in 
// dirlist, of length numdirs), returns the full name of the file found.
// In addition to looking at the root of each directory in dirlist, we also 
// look into the subdirectories texture/, textures/, images/, pics/ and 
// pictures/ of each dirlist directory.
// If no file matching basename could be found, returns an empty string.
SbString SbImagei32::searchForFile( const SbString& basename, \
	const SbString* const* dirlist, const int numdirs )
{
    int i;
    SbStringList directories;
    SbStringList subdirectories;

    for ( i = 0; i < numdirs; i++ )
	directories.append( const_cast<SbString *>(dirlist[i]) );
    
    subdirectories.append( new SbString("texture") );
    subdirectories.append( new SbString("textures") );
    subdirectories.append( new SbString("images") );
    subdirectories.append( new SbString("pics") );
    subdirectories.append( new SbString("pictures") );

    SbString ret = SoInput::searchForFile( basename, directories, 
	    subdirectories );
    
    for ( i = 0; i < subdirectories.getLength(); i++ )
	delete subdirectories[i];
    
    return ret;
}




// int SbImagei32::operator!=(const SbImagei32 & image) const
//  Compare image of with the image in this class and return FALSE if they are 
// equal.


// Compare image with the image in this class and return TRUE if they are equal.
int SbImagei32::operator == ( const SbImagei32& image ) const
{
    readLock();
    
    int ret = 0;

    if ( !PRIVATE(this)->schedulecb && !PRIVATE(&image)->schedulecb )
    {
	if ( PRIVATE(this)->size != PRIVATE(&image)->size ) ret = 0;
	else if ( PRIVATE(this)->bpp != PRIVATE(&image)->bpp ) ret = 0;
	else if ( PRIVATE(this)->bytes == NULL || 
		PRIVATE(&image)->bytes == NULL )
	    ret = (PRIVATE(this)->bytes == PRIVATE(&image)->bytes);
	else
       	{
	    ret = memcmp( PRIVATE(this)->bytes, PRIVATE(&image)->bytes,
		    int(PRIVATE(this)->size[0]) *
		    int(PRIVATE(this)->size[1]) *
		    int(PRIVATE(this)->size[2]==0?1:PRIVATE(this)->size[2]) * 
		    PRIVATE(this)->bpp ) == 0;
	}
    }

    readUnlock();
    return ret;
}


//  Assignment operator.
SbImagei32 & SbImagei32::operator = ( const SbImagei32& image )
{
    if ( *this != image )
    {
	PRIVATE(this)->writeLock();
	PRIVATE(this)->freeData();
	PRIVATE(this)->writeUnlock();

	if ( PRIVATE(&image)->bytes )
	{
	    PRIVATE(&image)->readLock();

	    switch ( PRIVATE(&image)->datatype )
	    {
		default:
		    assert(0 && "unknown data type");
		    break;

		case SbImagei32P::INTERNAL_DATA:

		case SbImagei32P::SIMAGE_DATA:
		    // need to copy data both for INTERNAL and SIMAGE data, 
		    // since we can only free the data once when the data are 
		    // of SIMAGE type.
		    setValue( PRIVATE(&image)->size, PRIVATE(&image)->bpp,
			      PRIVATE(&image)->bytes );
		    break;

		case SbImagei32P::SETVALUEPTR_DATA:
		    // just set the data ptr
		    setValuePtr( PRIVATE(&image)->size, PRIVATE(&image)->bpp,
			    PRIVATE(&image)->bytes );
		    break;
	    }
	    PRIVATE(&image)->readUnlock();
	}
    }
    return *this;
}


// Schedule a file for reading. cb will be called the first time getValue() is 
// called for this image, and the callback should then start a thread to read 
// the image. Do not read the image in the callback, as this will lock up the 
// application.
SbBool SbImagei32::scheduleReadFile( SbImagei32ScheduleReadCB* cb, 
	void* closure, const SbString& filename, 
	const SbString* const* searchdirectories, const int numdirectories )
{
    setValue( SbVec3i32( 0, 0, 0 ), 0, NULL );
    PRIVATE(this)->writeLock();
    PRIVATE(this)->schedulecb = NULL;
    PRIVATE(this)->schedulename =
	searchForFile( filename, searchdirectories, numdirectories );
    
    int len = PRIVATE(this)->schedulename.getLength();
    if ( len > 0 )
    {
	PRIVATE(this)->schedulecb = cb;
	PRIVATE(this)->scheduleclosure = closure;
    }

    PRIVATE(this)->writeUnlock();
    return len > 0;
}


// Returns TRUE if the image is not empty. This can be useful, since getValue()
// will start loading the image if scheduleReadFile() has been used to set the 
// image data.
SbBool SbImagei32::hasData() const
{
    SbBool ret;
    readLock();
    ret = PRIVATE(this)->bytes != NULL;
    readUnlock();
    return ret;
}


// Returns the size of the image. If this is a 2D image, the z component is 
// zero. If this is a 3D image, the z component is  >= 1.
SbVec3i32 SbImagei32::getSize() const
{
    return PRIVATE(this)->size;
}


// Add a callback which will be called whenever Coin wants to read an image 
// file.  The callback should return TRUE if it was able to successfully read 
// and set the image data, and FALSE otherwise. The callback(s) will be called 
// before attempting to use simage to load images.
void SbImagei32::addReadImageCB( SbImagei32ReadImageCB* cb, void* closure )
{
    if ( !SbImagei32P::readimagecallbacks )
    {
	SbImagei32P::readimagecallbacks = 
	    new SbList <SbImagei32P::ReadImageCBData>;
	cc_coin_atexit( static_cast<coin_atexit_f*>
		(SbImagei32_cleanup_callback) );
    }

    SbImagei32P::ReadImageCBData data;
    data.cb = cb;
    data.closure = closure;

    SbImagei32P::readimagecallbacks->append( data );
}


// Remove a read image callback added with addReadImageCB().
void SbImagei32::removeReadImageCB( SbImagei32ReadImageCB* cb, void* closure )
{
    if ( SbImagei32P::readimagecallbacks )
    {
	for ( int i=0; i < SbImagei32P::readimagecallbacks->getLength(); i++ )
	{
	    SbImagei32P::ReadImageCBData data = 
		(*SbImagei32P::readimagecallbacks)[i];
	    if ( data.cb == cb && data.closure == closure )
	    {
		SbImagei32P::readimagecallbacks->remove( i );
		return;
	    }
	}
    }
}

#undef PRIVATE

