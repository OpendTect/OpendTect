#ifndef qnetworkaccessconn_h
#define qnetworkaccessconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id: qnetworkaccessconn.h,v 1.2 2012/01/09 23:40:55 cvsnanne Exp $
________________________________________________________________________

-*/


#include "odnetworkaccess.h"
#include <QNetworkAccessManager>


class QNAMConnector : public QObject
{
    Q_OBJECT
    friend class ODNetworkAccess;

protected:

QNAMConnector( QNetworkAccessManager* sndr, ODNetworkAccess* receiver )
    : sender_(sndr), receiver_(receiver)
{
    connect( sender_, SIGNAL(finished(QNetworkReply*)),
	     this, SLOT(finished(QNetworkReply*)) );
}

private slots:

void finished( QNetworkReply* reply )
{
}


private:

    QNetworkAccessManager*	sender_;
    ODNetworkAccess*		receiver_;
};


class QNetworkReplyConn : public QObject
{
    Q_OBJECT

protected:

QNetworkReplyConn( QNetworkReply* sndr, ODNetworkReply* rec )
    : sender_(sndr), receiver_(rec)
{
    connect( sender_, SIGNAL(downloadProgress(qint64,qint64)),
	     this, SLOT(downloadProgress(qint64,qint64)) );
    connect( sender_, SIGNAL(error(QNetworkReply::NetworkError)),
	     this, SLOT(error(QNetworkReply::NetworkError)) );
    connect( sender_, SIGNAL(finished()),
	     this, SLOT(finished()) );
    connect( sender_, SIGNAL(metaDataChanged()),
	     this, SLOT(metaDataChanged()) );
    connect( sender_, SIGNAL(uploadProgress(qint64,qint64)),
	     this, SLOT(uploadProgress(qint64,qint64)) );

    // From QIODevice
    connect( sender_, SIGNAL(aboutToClose()),
	     this, SLOT(aboutToClose()) );
    connect( sender_, SIGNAL(bytesWritten(qint64)),
	     this, SLOT(bytesWritten(qint64)) );
    connect( sender_, SIGNAL(readyRead()),
	     this, SLOT(readyRead()) );
}

private slots:

void downloadProgress(qint64,qint64)
{}

void error(QNetworkReply::NetworkError)
{}

void finished()
{}

void metaDataChanged()
{}

void uploadProgress(qint64,qint64)
{}

void aboutToClose()
{}

void bytesWritten(qint64)
{}

void readChannelFinished()
{}

void readyRead()
{}

private:

    QNetworkReply*	sender_;
    ODNetworkReply*	receiver_;

};

#endif
