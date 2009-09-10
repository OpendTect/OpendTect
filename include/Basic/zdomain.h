#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009
 RCS:		$Id: zdomain.h,v 1.6 2009-09-10 13:02:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "fixedstring.h"
class IOPar;


namespace ZDomain
{
    mBasicExtern FixedString	sKey();
    mBasicExtern FixedString	sKeyID();
    mBasicExtern FixedString	sKeyTWT();
    mBasicExtern FixedString	sKeyDepth();
    mBasicExtern FixedString	getDefault();

    mBasicExtern bool		isSIDomain(const IOPar&);
    mBasicExtern void		setSIDomain(IOPar&,bool);
};



#endif
