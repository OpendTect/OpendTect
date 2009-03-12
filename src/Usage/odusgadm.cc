/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgadm.cc,v 1.1 2009-03-12 15:51:31 cvsbert Exp $";

#include "odusgbaseadmin.h"
#include "odusginfo.h"
#include "iopar.h"
#include <iostream>

std::ostream* Usage::Administrator::logstrm_ = 0;


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
    , pars_(*new IOPar)
{
    readPars();
}


Usage::Administrator::~Administrator()
{
    delete &pars_;
}


void Usage::Administrator::readPars()
{
    //TODO get all pars from repos sources
}


void Usage::Administrator::reInit()
{
    pars_.clear();
    readPars();
    reset();
}


void Usage::Administrator::toLogFile( const char* msg ) const
{
    if ( !logstrm_ || !msg ) return;

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
    msg += inf.group_; msg += "/"; msg += inf.action_;
    msg += " aux="; msg += inf.auxinfo_;
    toLogFile( msg );
    return true;
}


void Usage::BaseAdministrator::reset()
{
    //TODO use the pars
}
