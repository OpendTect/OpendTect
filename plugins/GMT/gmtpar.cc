/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtpar.cc,v 1.7 2010/04/14 05:54:28 cvsraman Exp $";


#include "gmtpar.h"

#include "debug.h"
#include "envvars.h"
#include "keystrs.h"
#include "oddirs.h"
#include "strmdata.h"
#include "strmprov.h"
#include <iostream>


GMTParFactory& GMTPF()
{
    static PtrMan<GMTParFactory> inst = 0;
    if ( !inst )
	inst = new GMTParFactory;

    return *inst;
}


int GMTParFactory::add( const char* nm, GMTParCreateFunc fn )
{
    Entry* entry = getEntry( nm );
    if ( entry )
	entry->crfn_ = fn;
    else
    {
	entry = new Entry( nm, fn );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


GMTPar* GMTParFactory::create( const IOPar& iop ) const
{
    const char* grpname = iop.find( ODGMT::sKeyGroupName );
    if ( !grpname || !*grpname ) return 0;

    Entry* entry = getEntry( grpname );
    if ( !entry ) return 0;

    GMTPar* grp = entry->crfn_( iop );
    return grp;
}


const char* GMTParFactory::name( int idx ) const
{
    if ( idx < 0 || idx >= entries_.size() )
	return 0;

    return entries_[idx]->name_.buf();
}


GMTParFactory::Entry* GMTParFactory::getEntry( const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->name_ == nm )
	    return const_cast<GMTParFactory*>( this )->entries_[idx];
    }

    return 0;
}


BufferString GMTPar::fileName( const char* fnm ) const
{
    BufferString fnmchg;
    if ( __iswin__ ) fnmchg += "\"";
    fnmchg += fnm;
    if ( __iswin__ ) fnmchg += "\"";
    return fnmchg;
}


bool GMTPar::execCmd( const BufferString& comm, std::ostream& strm )
{
    DBG::message( comm );
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !strstr(shellnm,"bash");
    if ( needsbash )
	cmd += "bash -c \'";

    cmd += comm;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    if ( system(cmd) )
    {
	StreamData sd = StreamProvider( errfilenm ).makeIStream();
	if ( !sd.usable() )
	    return false;

	char buf[256];
	strm << std::endl;
	while ( sd.istrm->getline(buf,256) )
	    strm << buf << std::endl;

	sd.close();
	return false;
    }

    return true;
}


StreamData GMTPar::makeOStream( const BufferString& comm, std::ostream& strm )
{
    DBG::message( comm );
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !strstr(shellnm,"bash");
    const char* commptr = comm.buf();
    if ( needsbash )
    {
	cmd += "@bash -c \'";
	if ( commptr[0] == '@' )
	    commptr ++;
    }

    cmd += commptr;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    StreamData sd = StreamProvider(cmd).makeOStream();
    if ( !sd.usable() )
    {
	StreamData errsd = StreamProvider( errfilenm ).makeIStream();
	if ( !errsd.usable() )
	    return sd;

	char buf[256];
	strm << std::endl;
	while ( errsd.istrm->getline(buf,256) )
	    strm << buf << std::endl;

	errsd.close();
    }

    return sd;
}
