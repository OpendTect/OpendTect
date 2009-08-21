/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: odver.cc,v 1.5 2009-08-21 11:10:10 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: odver.cc,v 1.5 2009-08-21 11:10:10 cvsbert Exp $";

#include "odver.h"
#include "oddirs.h"
#include "filepath.h"
#include "filegen.h"
#include "strmprov.h"
#include "bufstring.h"
#include <iostream>
#include "odlic.h"

#ifndef __isnotgpl__
# define __isgpl__ true
#else
# define __isgpl__ false
#endif

mExternC bool OD_IS_GPL_License() { return __isgpl__; }

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

    if ( OD_IS_GPL_License() )
	res += " [GPL]";
    return res.buf();
}


void GetSpecificODVersion( const char* typ, BufferString& res )
{
    if ( !typ ) typ = GetPlfSubDir();

    const char* swdir = GetSoftwareDir(0);
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
