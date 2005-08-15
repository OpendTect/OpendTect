#ifndef general_H
#define general_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.9 2005-08-15 11:27:50 cvsbert Exp $
________________________________________________________________________

-*/

#ifndef genc_H
#include <genc.h>
#endif

#ifdef __cpp__

#include "fixstring.h"
#include "bufstring.h"

typedef BufferString			UserIDString;
typedef FixedString<PATH_LENGTH>	FileNameString;

template <class T>
inline void Swap( T& a, T& b ) { T tmp = a; a = b; b = tmp; }

#include <typeinfo>

template <class T>
inline const char* className( const T& t )
{	//!< Also works for gcc that returns the size first e.g. 4Clss
    const char* nm = typeid(t).name();
    while ( isdigit(*nm) ) nm++;
    return nm;
}


//! Defines policy for selection of 2D vs 3D seismics
enum Pol2D	{ No2D=-1, Both2DAnd3D=0, Only2D=1 };

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

#define mDefSetupMemb(typ,memb) \
	typ	 memb##_; \
	Setup&	memb( typ val )		{ memb##_ = val; return *this; }


#endif

#endif
