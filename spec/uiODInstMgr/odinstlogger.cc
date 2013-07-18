/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstlogger.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "odinstlogger.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include <fstream>


ODInst::Logger& ODInst::Logger::theInst( const char* fnm )
{
    static ODInst::Logger* l = 0;
    if ( !l ) l = new ODInst::Logger( fnm );
    return *l;
}


ODInst::Logger::Logger( const char* fnm )
    : logfnm_(fnm)
    , strm_(mkStream(fnm))
{
}


std::ostream& ODInst::Logger::mkStream( const char* fnm )
{
    if ( !fnm ) fnm = "";
    const FilePath fptmp( File::getTempPath() );
    BufferString usr( GetUserNm(), "_od_instmgr" );
    FilePath fpprev( fptmp, BufferString(usr,fnm,"_log_prev.txt") );
    const BufferString prevfnm( fpprev.fullPath() );
    FilePath fpcur( fptmp, BufferString(usr,fnm,"_log.txt") );
    BufferString curfnm( fpcur.fullPath() );

    if ( File::exists(curfnm) )
    {
	if ( File::exists(prevfnm) )
	    File::remove(prevfnm);
	if ( !File::exists(prevfnm) )
	    File::rename(curfnm,prevfnm);
	for ( int idx=1; File::exists(curfnm); idx++ )
	{
	    fpcur.setFileName( BufferString("od_instmgr_log_",idx,".txt") );
	    curfnm = fpcur.fullPath();
	}
    }

    logfnm_ = curfnm;
    return *new std::ofstream( curfnm );
}


void ODInst::Logger::flush()
{
    static_cast<std::ofstream&>( strm_ ).flush();
}


void ODInst::Logger::close()
{
    delete &strm_;
}
