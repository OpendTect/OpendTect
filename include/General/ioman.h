#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		3-8-1995 / Oct 2016
________________________________________________________________________

-*/

#warning "IOMan and IOM() are replaced by DBMan and DBM(). Include dbman.h"
#include "dbman.h"
typedef DBMan IOMan;
mGlobal(General) mDeprecated inline DBMan&	IOM()	{ return DBM(); }
