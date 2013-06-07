#ifndef qftpconn_h
#define qftpconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "odftp.h"
#include <QFtp>


class QFtpConnector : public QObject
{
    Q_OBJECT
    friend class ODFtp;

protected:

QFtpConnector( QFtp* snder, ODFtp* receiver )
    : sender_(snder), receiver_(receiver)
{
    connect( sender_, SIGNAL(commandFinished(int,bool)),
	     this, SLOT(commandFinished(int,bool)) );

    connect( sender_, SIGNAL(commandStarted(int)),
	     this, SLOT(commandStarted(int)) );

    connect( sender_, SIGNAL(dataTransferProgress(qint64,qint64)),
	     this, SLOT(dataTransferProgress(qint64,qint64)) );

    connect( sender_, SIGNAL(readyRead()),
	     this, SLOT(readyRead()) );

    connect( sender_, SIGNAL(stateChanged(int)),
	     this, SLOT(stateChanged(int)) );

    connect( sender_, SIGNAL(listInfo(const QUrlInfo&)),
	     this, SLOT(listInfo(const QUrlInfo&)) );

    connect( sender_, SIGNAL(done(bool)),
	     this, SLOT(done(bool)) );
}

private slots:

void commandFinished( int id, bool error )
{
    receiver_->commandid_ = id;
    receiver_->error_ = error;
    if ( error )
    {
	receiver_->commandFinished.trigger( *receiver_ );
	receiver_->setMessage( sender_->errorString().toAscii().data() );
	return;
    }

    if ( sender_->currentCommand() == QFtp::ConnectToHost )
    {
	receiver_->connected.trigger( *receiver_ );
	receiver_->setMessage( "Connected" );
    }
    else if ( sender_->currentCommand() == QFtp::Login )
    {
	receiver_->loginDone.trigger( *receiver_ );
	receiver_->setMessage( "Login succesful" );
    }
    else if ( sender_->currentCommand() == QFtp::Cd )
	receiver_->setMessage( "Changing directory done" );
    else if ( sender_->currentCommand() == QFtp::Get )
    {
	receiver_->setMessage( "Download done" );
	receiver_->transferDone.trigger( *receiver_ );
    }
    else if ( sender_->currentCommand() == QFtp::List )
    {
	receiver_->setMessage( "Getting filelist done" );
	receiver_->listReady.trigger( *receiver_ );
    }
    else if ( sender_->currentCommand() == QFtp::Close )
	receiver_->setMessage( "Connection closed" );
    else
	receiver_->setMessage( BufferString("Command (",id,") finished") );

    receiver_->commandFinished.trigger( *receiver_ );
}

void commandStarted( int id ) 
{
    receiver_->commandid_ = id;
    if ( sender_->currentCommand() == QFtp::ConnectToHost )
	receiver_->setMessage( "Connecting ..." );
    else if ( sender_->currentCommand() == QFtp::Close )
	receiver_->setMessage( "Closing ..." );
    else if ( sender_->currentCommand() == QFtp::Login )
	receiver_->setMessage( "Login ..." );
    else if ( sender_->currentCommand() == QFtp::Cd )
	receiver_->setMessage( "Changing directory ..." );
    else if ( sender_->currentCommand() == QFtp::Get )
	receiver_->setMessage( "Starting download ..." );
    else if ( sender_->currentCommand() == QFtp::List )
    {
	receiver_->files_.erase();
	receiver_->setMessage( "Getting filelist ..." );
    }

    receiver_->commandStarted.trigger( *receiver_ );
}


void dataTransferProgress( qint64 dne, qint64 total ) 
{
    receiver_->nrdone_ = dne;
    receiver_->totalnr_ = total;
    receiver_->dataTransferProgress.trigger( *receiver_ );
}


void readyRead()
{ receiver_->readyRead.trigger( *receiver_ ); }

void stateChanged( int state )
{
    receiver_->connectionstate_ = state;
    if ( state == QFtp::Unconnected )
    {
	receiver_->disconnected.trigger( *receiver_ );
	receiver_->setMessage( "Connection closed" );
    }
}


void listInfo( const QUrlInfo& info )
{
    if ( info.isFile() )
	receiver_->files_.add( info.name().toAscii().data() );
}

void done( bool error )
{
    receiver_->error_ = error;
    receiver_->message_ = error ? sender_->errorString().toAscii().data()
				: "Sucessfully finished";
    receiver_->done.trigger( *receiver_ );
}

private:

    QFtp*	sender_;
    ODFtp*	receiver_;
};

#endif
