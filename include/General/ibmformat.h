#ifndef ibmformat_h
#define ibmformat_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "generalmod.h"
#include "gendefs.h"

/*!
\brief IBM Format.
*/

mExpClass(General) IbmFormat
{
public:

    static int			asInt(const void*);
    static short		asShort(const void*);
    static unsigned short	asUnsignedShort(const void*);
    static float		asFloat(const void*);

    static void			putInt(int,void*);
    static void			putShort(short,void*);
    static void			putUnsignedShort(unsigned short,void*);
    static void			putFloat(float,void*);

};


#endif

