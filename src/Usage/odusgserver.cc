/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgserver.cc,v 1.4 2009-07-16 07:55:07 cvsbert Exp $";

#include "odusgserver.h"
#include "odusgbaseadmin.h"
#include "odusginfo.h"
#include "timefun.h"
#include "iopar.h"
#include "dirlist.h"
#include "ascstream.h"
#include "strmprov.h"
#include "oddirs.h"
#include "ptrman.h"
#include <iostream>

const char* Usage::Server::sKeyPort()		{ return "Port"; }
const char* Usage::Server::sKeyFileBase()	{ return "Usage"; }
int Usage::Server::cDefaulPort()		{ return mUsgServDefaulPort; }
#define mBaseFileName \
    	GetSetupDataFileName(ODSetupLoc_ApplSetupPref,sKeyFileBase())


Usage::Server::Server( const IOPar* pars, std::ostream& strm )
    : logstrm_(strm)
    , pars_(pars ? *pars : *new IOPar)
    , port_(mUsgServDefaulPort)
    , thread_(0)
{
    if ( !pars )
    {
	pars = getPars();
	if ( pars ) const_cast<IOPar&>(pars_) = *pars;
    }

    if ( pars_.isEmpty() )
    {
	logstrm_ << "Cannot start OpendTect Usage server (" << "$Revision: 1.4 $"
	    	 << "):\n";
	if ( pars )
	    logstrm_ << "No input parameters" << std::endl;
	else
	    logstrm_ << "Cannot read: " << mBaseFileName << std::endl;
	return;
    }
    usePar();

    logstrm_ << "OpendTect Usage server (" << rcsID << ")" << std::endl;
    logstrm_ << "\non " << GetLocalHostName();
    if ( port_ > 0 )
	logstrm_ << " (port: " << port_ << ")";
    logstrm_ << "\nStarted: " << Time_getFullDateString() << '\n' << std::endl;

    Administrator::setLogStream( &logstrm_ );
}


Usage::Server::~Server()
{
    delete const_cast<IOPar*>( &pars_ );
    if ( thread_ )
	{ thread_->stop(); delete thread_; }
}


IOPar* Usage::Server::getPars()
{
    StreamData sd( StreamProvider(mBaseFileName).makeIStream() );
    if ( !sd.usable() )
	return 0;

    IOPar* iop = new IOPar( "Usage Monitor settings" );
    ascistream astrm( *sd.istrm );
    iop->getFrom( astrm );
    sd.close();

    if ( iop->isEmpty() )
	{ delete iop; return 0; }

    return iop;
}


void Usage::Server::usePar()
{
    pars_.get( sKeyPort(), port_ );
}


bool Usage::Server::go( bool inthr )
{
    if ( !inthr )
	return doWork( 0 );

    mThreadMutexedSet(thread_,
	    new Threads::Thread( mCB(this,Usage::Server,doWork) ) );
    return true;
}


void Usage::Server::addInfo( Usage::Info& inf )
{
    if ( thread_ )
	thread_mutex.lock();
    infos_ += &inf;
    if ( thread_ )
	thread_mutex.unLock();
}


void Usage::Server::getRemoteInfos()
{
    //TODO get infos from socket
}


bool Usage::Server::doWork( CallBacker* )
{
    while ( true )
    {
	if ( thread_ )
	    thread_mutex.lock();
	else
	    getRemoteInfos();

	while ( !infos_.isEmpty() )
	{
	    PtrMan<Usage::Info> inf = infos_[0];
	    infos_ -= inf;
	    if ( inf->group_.isEmpty() && inf->action_ == "Quit" )
		return inf->start_;

	    Administrator::dispatch( *inf );
	    if ( inf->withreply_ )
		sendReply( *inf );
	}
	if ( thread_ )
	    thread_mutex.unLock();
	Time_sleep( 0.1 );
    }
}


void Usage::Server::sendReply( const Usage::Info& inf )
{
    //TODO implement
}
