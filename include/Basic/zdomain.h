#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009
 RCS:		$Id: zdomain.h,v 1.5 2009-07-25 01:09:11 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "fixedstring.h"

namespace ZDomain
{
    mBasicExtern FixedString	sKey();
    mBasicExtern FixedString	sKeyID();
    mBasicExtern FixedString	sKeyTWT();
    mBasicExtern FixedString	sKeyDepth();
    mBasicExtern FixedString	getDefault();
};



#endif
