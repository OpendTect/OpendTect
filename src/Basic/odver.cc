/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: odver.cc,v 1.8 2010-04-27 10:20:18 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: odver.cc,v 1.8 2010-04-27 10:20:18 cvsbert Exp $";

#include "odver.h"
#include "oddirs.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "strmprov.h"

#include <iostream>


extern "C" const char* GetFullODVersion()
{
    static BufferString res;
    if ( !res.isEmpty() ) return res.buf();

    GetSpecificODVersion( 0, res );

    if ( res.isEmpty() )
    {
	const char* pvnm = GetProjectVersionName();
	pvnm = strrchr( pvnm, 'V' );
	if ( pvnm )
	    res = pvnm + 1;

	if ( res.isEmpty() )
	    res = "0.0.0";
    }

    return res.buf();
}


void GetSpecificODVersion( const char* typ, BufferString& res )
{
    if ( !typ ) typ = GetPlfSubDir();

    FilePath fp( GetSoftwareDir(0) ); fp.add( "relinfo" ).add( "ver" );
    BufferString fnm = FilePath(fp).fullPath();
    fnm += "."; fnm += typ;
    if ( !File::exists(fnm) )
	fnm = fp.fullPath();

    res = "";
    if ( File::exists(fnm) )
    {
	StreamData sd = StreamProvider( fnm ).makeIStream();
	if ( sd.usable() )
	{
	    char vstr[80]; sd.istrm->getline( vstr, 80 );
	    if ( vstr[0] )
		res = vstr;
	}
	sd.close();
    }
}
