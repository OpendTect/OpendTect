/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgserver.cc,v 1.2 2009-03-17 12:53:18 cvsbert Exp $";

#include "odusgserver.h"
#include "odusgbaseadmin.h"
#include "odusginfo.h"
#include "timefun.h"
#include "iopar.h"
#include <iostream>

const char* Usage::Server::sKeyPort()	{ return "Port"; }
int Usage::Server::DefaulPort()		{ return mUsageServerDefaulPort; }


Usage::Server::Server( const IOPar& iop, std::ostream& strm )
    : logstrm_(strm)
    , pars_(*new IOPar(iop))
    , port_(mUsageServerDefaulPort)
{
    pars_.get( sKeyPort(), port_ );
    logstrm_ << "OpendTect Usage server:\n\t" << rcsID << std::endl;
    logstrm_ << "Port nr: " << port_ << '\n' << std::endl;

    Administrator::setLogStream( &logstrm_ );
}


Usage::Server::~Server()
{
    delete &pars_;
}


bool Usage::Server::go()
{
    int itm = 0;
    while ( true )
    {
	if ( !(itm % 10) )
	{
	    logstrm_ << "\n\t" << Time_getFullDateString() << '\n';
	    Usage::Info inf( "dGB", itm % 30 ? "NN" : "DS" );
	    inf.aux_ = toString(itm);
	    Administrator::dispatch( inf );
	}
	Time_sleep( 1 );
	itm++;
    }
    return true;
}
