#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "odversion.h"
#include "bufstringset.h"

#include "gendefs.h"

extern "C"
{
    mGlobal(Basic) const char* GetFullODVersion();
}

mGlobal(Basic) const char* GetGCCVersion(); // Unix
mGlobal(Basic) const char* GetMSVCVersion(); // Windows
mGlobal(Basic) const char* GetMSVCVersionStr(); // Windows
mGlobal(Basic) const char* GetCompilerVersionStr();
mGlobal(Basic) const char* GetQtVersion();

void mGlobal(Basic) GetSpecificODVersion(const char* typ,BufferString&);
/*!< 'typ' can be "doc" or other like vendor name. if null -> platform */


mGlobal(Basic) const BufferStringSet&	GetLegalInformation();
mGlobal(Basic) void			AddLegalInformation(const char*);
