#ifndef ibmformat_h
#define ibmformat_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: ibmformat.h,v 1.3 2008-12-25 11:25:59 cvsranojay Exp $
________________________________________________________________________

-*/
 
#include <gendefs.h>


mClass IbmFormat
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
