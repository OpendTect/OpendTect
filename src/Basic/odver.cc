/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: odver.cc,v 1.1 2007-03-13 11:39:30 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: odver.cc,v 1.1 2007-03-13 11:39:30 cvsbert Exp $";

#include "odver.h"
#include "oddirs.h"
#include "filepath.h"
#include "filegen.h"
#include "strmprov.h"
#include "bufstring.h"


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

    const char* swdir = GetSoftwareDir();
    BufferString fnm = FilePath( swdir ).add( ".rel.od" ).fullPath();
    fnm += "."; fnm += typ;
    if ( !File_exists(fnm) )
    {
	fnm = FilePath( swdir ).add( ".rel.od" ).fullPath();
	if ( !File_exists(fnm) )
	    fnm = FilePath( swdir ).add( ".rel" ).fullPath();
    }

    res = "";
    if ( File_exists(fnm) )
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
