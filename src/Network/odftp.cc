/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "odftp.h"
#include "qftpconn.h"

#include <QFile>
#include <QFtp>


ODFtp::ODFtp()
    : connected(this)
    , disconnected(this)
    , loginDone(this)
    , commandStarted(this)
    , commandFinished(this)
    , dataTransferProgress(this)
    , transferDone(this)
    , readyRead(this)
    , listReady(this)
    , messageReady(this)
    , done(this)
    , qftp_(new QFtp)
{
    qftpconn_ = new QFtpConnector( qftp_, this );

    error_ = false;
    nrdone_ = 0;
    totalnr_ = 0;
    commandid_ = 0;
    connectionstate_ = -1;
    qfiles_.allowNull();

    transferDone.notify( mCB(this,ODFtp,transferDoneCB) );
}


ODFtp::~ODFtp()
{
    delete qftpconn_;
}


int ODFtp::connectToHost( const char* host, int port )
{ return qftp_->connectToHost( host, port ); }

int ODFtp::login( const char* usrnm, const char* passwd )
{ return qftp_->login( usrnm, passwd ); }

int ODFtp::close()
{ return qftp_->close(); }

void ODFtp::abort()
{ qftp_->abort(); }


int ODFtp::get( const char* file, const char* dest )
{
    QFile* qfile = 0;
    if ( dest )
    {
	qfile = new QFile( dest );
	qfile->open( QIODevice::WriteOnly );
	qftp_->rawCommand( "TYPE I" );
	qftp_->rawCommand( BufferString("SIZE ",file).buf() );
    }

    qfiles_ += qfile;
    const int cmdid = qftp_->get( file, qfile, QFtp::Binary );
    getids_ += cmdid;
    return cmdid;
}


void ODFtp::transferDoneCB( CallBacker* )
{
    const int cmdidx = getids_.indexOf( commandid_ );
    if ( qfiles_.validIdx(cmdidx) )
    {
	QFile* qfile = qfiles_[cmdidx];
	if ( qfile )
	    qfile->close();
	delete qfiles_.remove( cmdidx );
	getids_.remove( cmdidx );
    }
}


// TODO: implement
int ODFtp::put( const char* file )
{ return qftp_->put( 0, file ); }

int ODFtp::cd( const char* dir )
{ return qftp_->cd( dir ); }

int ODFtp::list()
{ return qftp_->list(); }

int ODFtp::rename( const char* oldname, const char* newname )
{ return qftp_->rename( oldname, newname ); }

int ODFtp::remove( const char* file )
{ return qftp_->remove( file ); }

int ODFtp::mkdir( const char* dir )
{ return qftp_->mkdir( dir ); }

int ODFtp::rmdir( const char* dir )
{ return qftp_->rmdir( dir ); }

bool ODFtp::hasPendingCommands() const
{ return qftp_->hasPendingCommands(); }

od_int64 ODFtp::bytesAvailable() const
{ return qftp_->bytesAvailable(); }

BufferString ODFtp::readBuffer() const
{
    QString result = qftp_->readAll();
    return result.toAscii().data();
}


void ODFtp::setMessage( const char* msg )
{
    message_ = msg;
    messageReady.trigger();
}
