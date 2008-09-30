#ifndef general_H
#define general_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.17 2008-09-30 11:48:43 cvsbert Exp $
________________________________________________________________________

-*/

#ifndef genc_H
#include <genc.h>
#endif

#ifdef __cpp__

#include "bufstring.h"

#define UserIDString	BufferString
#define FileNameString	BufferString

template <class T>
inline void Swap( T& a, T& b ) { T tmp = a; a = b; b = tmp; }

#include <typeinfo>

template <class T>
inline const char* className( const T& t )
{	//!< Also works for gcc that returns the size first e.g. 4Clss
    const char* nm = typeid(t).name();
    while ( *nm >= '0' && *nm <= '9' ) nm++;
    return nm;
}


//! Defines policy for 2D and 3D Data type
enum Pol2D3D	{ Only3D=-1, Both2DAnd3D=0, Only2D=1 };

//! Catches bad_alloc and sets ptr to null as normal.
#define mTryAlloc(var,stmt) \
{ try { var = new stmt; } catch ( std::bad_alloc ) { var = 0; } }

//!Creates variable, try to alloc and catch bad_alloc.
#define mDeclareAndTryAlloc(tp,var,stmt)				\
    tp var;								\
    mTryAlloc(var,stmt)

//! Define members in setup classes (see e.g. uidialog.h)
/* Usage typically like:

 class SomeClass
 {
 public:
 
	 class Setup
	 {
		 Setup()
		 : withformat_(true)	//!< Can user select storage?
		 , withstep_(true)	//!< Can user specify steps?
		 , title_("")		//!< Title for plots
				{}

		mDefSetupMemb(bool,withformat)
		mDefSetupMemb(bool,withstep)
		mDefSetupMemb(BufferString,title)
	 }
	
	SomeClass(const Setup&);
	// etc.
};

The point of this is clear when SomeClass is constructed:

    SomeClass sc( SomeClass::Setup().withformat(false).title("MyTitle") );

 */

#define mDefSetupClssMemb(clss,typ,memb) \
	typ	 memb##_; \
	clss&	memb( typ val )		{ memb##_ = val; return *this; }

#define mDefSetupMemb(typ,memb) mDefSetupClssMemb(Setup,typ,memb)


#endif

#endif
