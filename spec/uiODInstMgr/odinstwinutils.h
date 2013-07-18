#ifndef odinstwinutils_h
#define odinstwinutils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Feb 2012
 RCS:           $Id: odinstwinutils.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "bufstring.h"

namespace ODInst
{

class AppData;
class PkgProps;
class Platform;


class WinUtils
{
public:
			WinUtils(const AppData&,const Platform&);
			~WinUtils();

    bool		createStartMenuLinks(const BufferString& pckgnm, 
						const char* desc, bool isw32,
						bool inrel);
    bool		createDeskTopLinks(const BufferString& pckgnm);
    bool		makeUninstallScripts(const char*, const char*,
						const char*, const char*);
    BufferString		errMsg() const { return errmsg_; }

protected:

    const AppData&		appdata_;
    BufferString		errmsg_;
    const Platform&		platform_;
    BufferString		uninstscript_;
};

} // namespace ODInst

#endif



