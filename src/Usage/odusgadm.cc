/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id$";

#include "odusgbaseadmin.h"
#include "odusginfo.h"
#include "odusgserver.h"
#include "iopar.h"
#include "oddirs.h"
#include "filepath.h"
#include "strmprov.h"
#include "ascstream.h"
#include <iostream>



static ObjectSet<Usage::Administrator>& ADMS()
{
    static ObjectSet<Usage::Administrator>* adms = 0;
    if ( !adms )
    {
	adms = new ObjectSet<Usage::Administrator>;
	*adms += new Usage::BaseAdministrator;
    }
    return *adms;
}


int Usage::Administrator::add( Usage::Administrator* adm )
{
    if ( !adm ) return -1;

    ADMS() += adm;
    return ADMS().size() - 1;
}


bool Usage::Administrator::dispatch( Usage::Info& inf )
{
    ObjectSet<Administrator>& adms = ADMS();
    bool ret = true;
    for ( int idx=0; idx<adms.size(); idx++ )
	ret &= adms[idx]->handle( inf );

    return ret;
}


Usage::Administrator::Administrator( const char* nm )
    : NamedObject(nm)
{
    readPars();
}


Usage::Administrator::~Administrator()
{
    deepErase( pars_ );
}


void Usage::Administrator::readPars()
{
    deepErase( pars_ );
    const BufferString filenm( Usage::Server::setupFileName( name() ) );
    addPars( GetSetupDataFileDir(ODSetupLoc_ApplSetupPref,0), filenm );
    addPars( GetSettingsDir(), filenm );
}


void Usage::Administrator::addPars( const char* dir, const char* fnm )
{
    const FilePath fp( dir, fnm );
    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm );
    IOPar* newpar = new IOPar( astrm );
    if ( newpar->isEmpty() )
	delete newpar;
    else
	pars_ += newpar;
}


void Usage::Administrator::reInit()
{
    readPars();
    reset();
}


void Usage::Administrator::toLogFile( const char* msg ) const
{
    if ( !logstrm_ || !msg || !*msg ) return;

    const char* nm = name();
    if ( *nm )
	*logstrm_ << '[' << name() << "]: ";
    else if ( !*msg )
	return;

    *logstrm_ << msg << std::endl;
}


bool Usage::BaseAdministrator::handle( Usage::Info& inf )
{
    BufferString msg( "BaseAdministrator::handle - " );
    inf.dump( msg );
    toLogFile( msg );
    return true;
}


void Usage::BaseAdministrator::reset()
{
}
