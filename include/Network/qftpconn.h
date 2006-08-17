#ifndef qftpconn_h
#define qftpconn_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: qftpconn.h,v 1.1 2006-08-17 19:49:36 cvsnanne Exp $
________________________________________________________________________

-*/

#include "qtdefs.h"
#include "odftp.h"

#include <QFtp>

class QFtpConnector : public QObject
{
    Q_OBJECT

protected:
		QFtpConnector( QFtp* sender, ODFtp* receiver )
		    : sender_(sender), receiver_(receiver)
		{
		    mConnect( commandFinished(int,bool) );
		    mConnect( commandStarted(int) );
		    mConnect( dataTransferProgress(qint64,qint64) );
		    mConnect( done(bool) );
		    mConnect( listInfo(const QUrlInfo&) );
		    mConnect( rawCommandReply(int,const QString&) );
		    mConnect( readyRead() );
		    mConnect( stateChanged(int) );
		}

private slots:
    void	commandFinished( int id, bool error )
    		{
		    receiver_->commandid_ = id;
		    receiver_->error_ = error;
		    receiver_->commandFinished.trigger( *receiver_ );
		}

    void	commandStarted( int id ) 
    		{
		    receiver_->commandid_ = id;
		    receiver_->commandStarted.trigger( *receiver_ );
		}

    void	dataTransferProgress( qint64 done, qint64 total ) 
    		{
		    receiver_->nrdone_ = done;
		    receiver_->totalnr_ = total;
		    receiver_->dataTransferProgress.trigger( *receiver_ );
		}

    void	done( bool error ) 
    		{
		    receiver_->error_ = error;
		    receiver_->done.trigger( *receiver_ );
		}

    void	listInfo( const QUrlInfo& i ) 
    		{
		    receiver_->listInfo.trigger( *receiver_ );
		}

    void	rawCommandReply( int replyCode, const QString & detail ) {}
    void	readyRead()
		{ receiver_->readyRead.trigger( *receiver_ ); }

    void	stateChanged( int state )
		{
		    receiver_->connectionstate_ = state;
		    receiver_->stateChanged.trigger( *receiver_ ); }
		}

private:

    QFtp*	sender_;
    ODFtp*	receiver_;
};

#endif
