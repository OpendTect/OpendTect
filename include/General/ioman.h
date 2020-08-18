#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		3-8-1995 / Oct 2016
________________________________________________________________________

-*/

#ifdef  __win__
#pragma message ("IOMan and IOM() are replaced by DBMan and DBM(). Include dbman.h")
#else
#warning "IOMan and IOM() are replaced by DBMan and DBM(). Include dbman.h"
#endif
#include "dbman.h"
typedef DBMan IOMan;
mGlobal(General) mDeprecated inline DBMan&	IOM()	{ return DBM(); }
