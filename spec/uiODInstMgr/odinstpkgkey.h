#ifndef odinstpkgkey_h
#define odinstpkgkey_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2010
 RCS:           $Id: odinstpkgkey.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "odinstplf.h"
#include "odinstver.h"


namespace ODInst
{

class PkgKey
{
public:

    			PkgKey(const char* filestr);
				//!< from server: "base_lux64: 4.0.1i"
    			PkgKey(const char* filename,const char* content);
				//!< local app: "ver.base_lux64.txt"
    			PkgKey( const char* nm, const Platform& p,
				 const Version& v )
			    : nm_(nm), plf_(p), ver_(v)	{}

    void		setFromFileNameBase(const char*);

    BufferString	fileNameBase() const;
    BufferString	zipFileName() const;
    BufferString	listFileName() const;

    BufferString	nm_;
    Platform		plf_;
    Version		ver_;

};

} // namespace

#endif
