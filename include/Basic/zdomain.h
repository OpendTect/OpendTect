#ifndef zdomain_h
#define zdomain_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra & K. Tingdahl
 Date:		April 2009
 RCS:		$Id: zdomain.h,v 1.2 2009-04-07 06:25:52 cvsranojay Exp $
________________________________________________________________________

-*/

namespace ZDomain
{
    mGlobal const char*		sKey()			{ return "ZDomain"; }
    mGlobal const char*		sKeyID()		{ return "ZDomain ID"; }
    mGlobal const char*		sKeyTWT()		{ return "TWT"; }
    mGlobal const char*		sKeyDepth()		{ return "Depth"; }
    mGlobal const char*		getDefault();
};



#endif
