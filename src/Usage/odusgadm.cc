/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odusgbaseadmin.h"
#include "odusginfo.h"
#include "odusgserver.h"
#include "iopar.h"
#include "oddirs.h"
#include "filepath.h"
#include "ascstream.h"
#include "od_iostream.h"


od_ostream* Usage::Administrator::logstrm_ = 0;


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
    od_istream strm( fp.fullPath() );
    if ( !strm.isOK() ) return;

    ascistream astrm( strm );
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


od_ostream* Usage::Administrator::logStream()
{
    return logstrm_;
}


void Usage::Administrator::toLogFile( const char* msg ) const
{
    if ( !logstrm_ || !msg || !*msg ) return;

    const char* nm = name();
    if ( *nm )
	*logstrm_ << '[' << name() << "]: ";
    else if ( !*msg )
	return;

    *logstrm_ << msg << od_newline;
    logstrm_->flush();
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
