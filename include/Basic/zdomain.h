#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009
 RCS:		$Id: zdomain.h,v 1.8 2010-03-18 19:23:12 cvskris Exp $
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


mStruct Info
{
    			Info(bool fromsi=true);
    BufferString	name_;
    BufferString	unitstr_;
    BufferString	id_;
    float		zfactor_;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyUnit();
    static const char*	sKeyFactor();
};

} // namespace ZDomain

#endif
