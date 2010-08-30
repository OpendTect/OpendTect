#ifndef SbImagei32_h
#define SbImagei32_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika S
 Date:          August 2010
 RCS:           $Id: SbImagei32.h,v 1.1 2010-08-30 08:32:20 cvskarthika Exp $
________________________________________________________________________

-*/

#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec3i32.h>
#include <Inventor/SbString.h>
//#include <stddef.h> // for NULL
#include "soodbasic.h"

class SbImagei32;

typedef SbBool SbImagei32ScheduleReadCB(const SbString &, SbImagei32 *, void *);
typedef SbBool SbImagei32ReadImageCB(const SbString &, SbImagei32 *, void *);

// This class is a clone of SbImage, which allows images of large size (upto 
// 2^31 per dimension) to be handled.

mClass SbImagei32
{
public:

    			SbImagei32(void);
    			SbImagei32(const unsigned char* bytes, const SbVec2i32& 
				size, const int bytesperpixel);
			SbImagei32(const unsigned char* bytes, const SbVec3i32& 
				size, const int bytesperpixel);
			~SbImagei32();

	void 		setValue(const SbVec2i32& size, const int bytesperpixel,
				const unsigned char* bytes);
	void		setValue(const SbVec3i32& size, const int bytesperpixel,
				const unsigned char* bytes);
	void 		setValuePtr(const SbVec2i32& size, const int 
				bytesperpixel, const unsigned char* bytes);
	void 		setValuePtr(const SbVec3i32& size, const int 
				bytesperpixel, const unsigned char* bytes);
	unsigned char* 	getValue(SbVec2i32& size, int& bytesperpixel) const;
	unsigned char* 	getValue(SbVec3i32& size, int& bytesperpixel) const;
	SbVec3i32 	getSize(void) const;

	SbBool		readFile(const SbString& filename, const SbString* 
				const* searchdirectories=NULL, const int 
				numdirectories=0);

	int		operator==(const SbImagei32& image) const;
	int		operator!=(const SbImagei32& image) const {
	    		    return ! operator == (image); }
	SbImagei32& 	operator=(const SbImagei32& image);

	static void 	addReadImageCB(SbImagei32ReadImageCB* cb, 
				void* closure);
	static void 	removeReadImageCB(SbImagei32ReadImageCB* cb, 
				void* closure);

	static SbString	searchForFile(const SbString& basename, const SbString*
	       			const* dirlist, const int numdirs);

	SbBool		hasData(void) const;

private:

	class SbImagei32P*	pimpl;
  
public:

  // methods for delaying image loading until it is actually needed.
    void		readLock(void) const;
    void		readUnlock(void) const;

    SbBool 		scheduleReadFile(SbImagei32ScheduleReadCB* cb, void* 
	    			closure, const SbString& filename, const 
				SbString* const* searchdirectories = NULL,
				const int numdirectories = 0);

};

#endif

