#ifndef SbImagei32_h
#define SbImagei32_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika S
 Date:          August 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec3i32.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbRWMutex.h>
#endif

#include "soodbasic.h"

class SbImagei32;

// This class is a clone of SbImage, which allows images of large size (upto 
// 2^31 per dimension) to be handled.

mClass SbImagei32
{
public:

    			SbImagei32();
    			SbImagei32(const unsigned char* data, const SbVec2i32& 
				sz, const int bytesperpixel);
			SbImagei32(const unsigned char* data, const SbVec3i32& 
				sz, const int bytesperpixel);
			~SbImagei32();

    bool		setValue(const SbVec2i32& sz, const int bytesperpixel,
				const unsigned char* data);
    bool		setValue(const SbVec3i32& sz, const int bytesperpixel,
				const unsigned char* data);
    void 		setValuePtr(const SbVec2i32& sz, const int 
				bytesperpixel, const unsigned char* data);
    void 		setValuePtr(const SbVec3i32& sz, const int 
				bytesperpixel, const unsigned char* data);
    unsigned char* 	getValue(SbVec2i32& sz, int& bytesperpixel) const;
    unsigned char* 	getValue(SbVec3i32& sz, int& bytesperpixel) const;
    SbVec3i32 		getSize() const;

    int			operator==(const SbImagei32& image) const;
    int			operator!=(const SbImagei32& image) const {
	    		    return ! operator == (image); }
    SbImagei32& 	operator=(const SbImagei32& image);

    SbBool		hasData() const;

    // methods for delaying image loading until it is actually needed.
    void		readLock() const;
    void		readUnlock() const;
    
private:

    void		freeData();
    void                writeLock();
    void                writeUnlock();

    enum        	DataType { INTERNAL_DATA, SETVALUEPTR_DATA };
    DataType            datatype;
    unsigned char*	bytes;
    SbVec3i32           size;
    int                 bpp;
    
#ifdef COIN_THREADSAFE
    SbRWMutex		rwmutex;
#endif

};

#endif

