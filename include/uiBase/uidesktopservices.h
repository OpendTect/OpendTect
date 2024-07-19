#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "gendefs.h"

mExpClass(uiBase) uiDesktopServices
{
public:
    static bool		openUrl(const char* url);
			//!< url has to start with https://, http://, file://,
			//!< ftp:// or mailto:
			//!< ftp://user:passwd@ftp.example.com
			//!< mailto:user@foo.com?subject=Test&body=Just a test
    static bool		showInFolder(const char* file);
			//!< Open the file explorer window and highlight
			//!< the given file


protected:
};
