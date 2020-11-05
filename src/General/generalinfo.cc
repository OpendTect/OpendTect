/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		06-03-2020
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalinfo.h"

#include "bufstringset.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"


namespace OD
{

bool	getHostIDs( BufferStringSet& hostids,
		    BufferString& errmsg )
{
    hostids.setEmpty();
    errmsg.setEmpty();

#ifdef  __mac__
    File::Path lmutilfp( GetSoftwareDir(false), "Resources", "bin",
	    	         GetPlfSubDir() );
#else
    File::Path lmutilfp( GetSoftwareDir(false), "bin", GetPlfSubDir() );
#endif

    lmutilfp.add( "lm.dgb" ).add( "lmutil" );

#ifdef __win__
    hostids.setEmpty();
    errmsg.setEmpty();

    lmutilfp.add( "lm.dgb" ).add( "lmutil" );

#ifdef __win__
    lmutilfp.setExtension( "exe" );
#endif

    if ( !lmutilfp.exists() )
    {
	lmutilfp.setPath( lmutilfp.dirUpTo(lmutilfp.nrLevels()-3) );
	if ( !lmutilfp.exists() )
	{
	    errmsg.add( "Required executable not found." );
	    return false;
	}
    }

    OS::MachineCommand cmd( lmutilfp.fullPath().buf() );
    BufferString stdoutput, stderror;
    cmd.addArg( "lmhostid" ).addFlag( "n", OS::OldStyle );
    const bool res = cmd.execute( stdoutput, &stderror );
    if ( !res )
    {
	errmsg.add( "Failed to get HostID information." );
	errmsg.addNewLine().add(  stderror );
	return false;
    }

    BufferString& outputstr = stderror.isEmpty() ? stdoutput : stderror;
    if ( outputstr.isEmpty() )
	return false;

    outputstr.unQuote( '"' );
    hostids.unCat( outputstr, " " );

    return true;
}

} //namespace OD
