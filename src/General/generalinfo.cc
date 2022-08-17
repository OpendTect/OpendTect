/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		06-03-2020
________________________________________________________________________

-*/


#include "generalinfo.h"

#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "perthreadrepos.h"


namespace OD
{

const char* getLmUtilFilePath( uiString* errmsg )
{
    mDeclStaticString( ret );
    mIfNotFirstTime( return ret.buf() )

#ifdef __mac__
    FilePath lmutilfp( GetSoftwareDir( false ), "Resources", "bin",
		       GetPlfSubDir() );
#else
    FilePath lmutilfp( GetSoftwareDir( false ), "bin", GetPlfSubDir() );
#endif
    lmutilfp.add( "lm.dgb" ).add( "lmutil" );
#ifdef __win__
    lmutilfp.setExtension( "exe" );
#endif

    if ( !lmutilfp.exists() )
	lmutilfp.setPath( lmutilfp.dirUpTo(lmutilfp.nrLevels()-3) );

    const BufferString lmutils( lmutilfp.fullPath() );
    if ( File::exists(lmutils.buf()) )
	ret.set( lmutils );
    else
    {
	if ( errmsg )
	{
	    *errmsg = od_static_tr( "getLmUtilFilePath",
		"FlexNet tool not found at %1. "
		"Please check your installation"
		" or contact your system administrator "
		"for more details" ).arg( lmutils );
	}
    }

    return ret.buf();
}

bool	getHostIDs( BufferStringSet& hostids,
		    BufferString& errmsg )
{
    hostids.setEmpty();
    errmsg.setEmpty();

    uiString uierrmsg;
    const BufferString lmutils( getLmUtilFilePath(&uierrmsg) );
    if ( lmutils.isEmpty() )
    {
	errmsg.add( uierrmsg );
	return false;
    }

    OS::MachineCommand cmd( lmutils );
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

} // namespace OD
