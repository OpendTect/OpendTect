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

    File::Path lmutilfp( GetSoftwareDir(false), "bin", GetPlfSubDir() );
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

    if ( stderror.isEmpty() )
	hostids.add( stdoutput );
    else
	hostids.add( stderror );

    return true;
}

} //namespace OD
