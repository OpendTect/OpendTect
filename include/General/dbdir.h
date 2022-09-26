#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "iodir.h"

#include "dbkey.h"

mExpClass(General) DBDir : public IODir
{
public:
			DBDir(const char* dirnm);
			DBDir(const DBKey&);
			~DBDir();
protected:
};
