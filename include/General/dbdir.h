#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "iodir.h"

#include "dbkey.h"

mExpClass(General) DBDir : public IODir
{
public:
			DBDir( const char* dirnm )
			   : IODir(dirnm)		{}
			DBDir( const DBKey& dbkey )
			   : IODir(dbkey)		{}
			~DBDir()			{}
protected:
};
