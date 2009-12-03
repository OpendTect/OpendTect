#ifndef qftpconn_h
#define qftpconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: qftpconn.h,v 1.4 2009-12-03 03:18:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "odftp.h"
#include <QFtp>


class QFtpConnector : public QObject
{
    Q_OBJECT

protected:

QFtpConnector( QFtp* sender, ODFtp* receiver )
    : sender_(sender), receiver_(receiver)
{
    connect( sender_, SIGNAL(commandFinished(int,bool)),
	     this, SLOT(commandFinished(int,bool)) );

    connect( sender_, SIGNAL(commandStarted(int)),
	     this, SLOT(commandStarted(int)) );

    connect( sender_, SIGNAL(dataTransferProgress(qint64,qint64)),
	     this, SLOT(dataTransferProgress(qint64,qint64)) );

    connect( sender_, SIGNAL(done(bool)),
	     this, SLOT(done(bool)) );

    connect( sender_, SIGNAL(listInfo(const QUrlInfo&)),
	     this, SLOT(listInfo(const QUrlInfo&)) );

    connect( sender_, SIGNAL(rawCommandReply(int,const QString&)),
	     this, SLOT(rawCommandReply(int,const QString&)) );

    connect( sender_, SIGNAL(readyRead()),
	     this, SLOT(readyRead()) );

    connect( sender_, SIGNAL(stateChanged(int)),
	     this, SLOT(stateChanged(int)) );
}

private slots:

void commandFinished( int id, bool error )
{
    receiver_->commandid_ = id;
    receiver_->error_ = error;

    if ( sender_->currentCommand() == QFtp::ConnectToHost )
	receiver_->connected.trigger( *receiver_ );
    else if ( sender_->currentCommand() == QFtp::Login )
	receiver_->loginDone.trigger( *receiver_ );
    else if ( sender_->currentCommand() == QFtp::List )
	receiver_->listDone.trigger( *receiver_ );

    receiver_->commandFinished.trigger( *receiver_ );
}

void commandStarted( int id ) 
{
    receiver_->commandid_ = id;
    receiver_->commandStarted.trigger( *receiver_ );
}

void dataTransferProgress( qint64 done, qint64 total ) 
{
    receiver_->nrdone_ = done;
    receiver_->totalnr_ = total;
    receiver_->dataTransferProgress.trigger( *receiver_ );
}

void done( bool error ) 
{
    receiver_->error_ = error;
    receiver_->done.trigger( *receiver_ );
}

void listInfo( const QUrlInfo& info ) 
{
    if ( !info.isValid() ) return;

    if ( info.isDir() )
	receiver_->dirlist_.add( info.name().toAscii().data() );
    else if ( info.isFile() )
	receiver_->filelist_.add( info.name().toAscii().data() );
    else
	return;
}

void rawCommandReply( int replyCode, const QString & detail ) {}

void readyRead()
{ receiver_->readyRead.trigger( *receiver_ ); }

void stateChanged( int state )
{
    receiver_->connectionstate_ = state;
    receiver_->stateChanged.trigger( *receiver_ );
}

private:

    QFtp*	sender_;
    ODFtp*	receiver_;
};

#endif
