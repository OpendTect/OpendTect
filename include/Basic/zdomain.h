#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009
 RCS:		$Id: zdomain.h,v 1.3 2009-06-22 15:17:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

namespace ZDomain
{
    mBasicExtern const char*	sKey();
    mBasicExtern const char*	sKeyID();
    mBasicExtern const char*	sKeyTWT();
    mBasicExtern const char*	sKeyDepth();
    mBasicExtern const char*	getDefault();
};



#endif
