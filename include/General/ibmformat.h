#ifndef ibmformat_h
#define ibmformat_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: ibmformat.h,v 1.1 2001-02-13 17:46:49 bert Exp $
________________________________________________________________________

-*/
 
#include <gendefs.h>


class IbmFormat
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
