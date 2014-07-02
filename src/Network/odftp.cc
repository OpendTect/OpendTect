/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odftp.h"
#ifndef OD_NO_QT
#include "qftpconn.h"

#include <QFile>
#include <QFtp>
#endif


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
#ifndef OD_NO_QT
    , qftp_(new QFtp)
#else
    , qftp_(0)
    , qftpconn_(0)
#endif
{
#ifndef OD_NO_QT
    qftpconn_ = new QFtpConnector( qftp_, this );
#endif

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
#ifndef OD_NO_QT
    delete qftpconn_;
#endif
}


int ODFtp::connectToHost( const char* host, int port )
{
#ifndef OD_NO_QT
    return qftp_->connectToHost( host, port );
#else
    return 0;
#endif
}

int ODFtp::login( const char* usrnm, const char* passwd )
{
#ifndef OD_NO_QT
    return qftp_->login( usrnm, passwd );
#else
    return 0;
#endif
}

int ODFtp::close()
{
#ifndef OD_NO_QT
    return qftp_->close();
#else
    return 0;
#endif
}

void ODFtp::abort()
{
#ifndef OD_NO_QT
    qftp_->abort();
#else
    return;
#endif
}


int ODFtp::get( const char* file, const char* dest )
{
#ifndef OD_NO_QT
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
#else
    return 0;
#endif
}


void ODFtp::transferDoneCB( CallBacker* )
{
#ifndef OD_NO_QT
    const int cmdidx = getids_.indexOf( commandid_ );
    if ( qfiles_.validIdx(cmdidx) )
    {
	QFile* qfile = qfiles_[cmdidx];
	if ( qfile )
	    qfile->close();
	delete qfiles_.removeSingle( cmdidx );
	getids_.removeSingle( cmdidx );
    }
#else
    return;
#endif
}


// TODO: implement
int ODFtp::put( const char* file )
{
#ifndef OD_NO_QT
    return qftp_->put( 0, file );
#else
    return false;
#endif
}

int ODFtp::cd( const char* dir )
{
#ifndef OD_NO_QT
    return qftp_->cd( dir );
#else
    return false;
#endif
}

int ODFtp::list()
{
#ifndef OD_NO_QT
    return qftp_->list();
#else
    return 0;
#endif
}

int ODFtp::rename( const char* oldname, const char* newname )
{
#ifndef OD_NO_QT
    return qftp_->rename( oldname, newname );
#else
    return 0;
#endif
}

int ODFtp::remove( const char* file )
{
#ifndef OD_NO_QT
    return qftp_->remove( file );
#else
    return 0;
#endif
}

int ODFtp::mkdir( const char* dir )
{
#ifndef OD_NO_QT
    return qftp_->mkdir( dir );
#else
    return 0;
#endif
}

int ODFtp::rmdir( const char* dir )
{
#ifndef OD_NO_QT
    return qftp_->rmdir( dir );
#else
    return 0;
#endif
}

bool ODFtp::hasPendingCommands() const
{
#ifndef OD_NO_QT
    return qftp_->hasPendingCommands();
#else
    return false;
#endif
}

od_int64 ODFtp::bytesAvailable() const
{
#ifndef OD_NO_QT
    return qftp_->bytesAvailable();
#else
    return 0;
#endif
}

#ifndef OD_NO_QT
BufferString ODFtp::readBuffer() const
{
    QString result = qftp_->readAll();
    return result.toLatin1().data();
}
#endif

void ODFtp::setMessage( const char* msg )
{
    message_ = msg;
    messageReady.trigger();
}
