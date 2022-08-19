#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
