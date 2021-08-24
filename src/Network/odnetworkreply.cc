/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/

#include "odnetworkreply.h"

#include "qnetworkaccessconn.h"

#include <QEventLoop>


ODNetworkReply::ODNetworkReply( QNetworkReply* qnr, QEventLoop* qel )
    : downloadProgress( this )
    , finished( this )
    , metaDataChanged( this )
    , error( this )
    , uploadProgress( this )
    , aboutToClose( this )
    , bytesWritten( this )
    , readyRead( this )
    , qeventloop_( qel )
    , bytesuploaded_( 0 )
{
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);
    mAttachCB( error, ODNetworkReply::errorOccurred );
    mAttachCB( finished, ODNetworkReply::finish );
    mAttachCB( readyRead, ODNetworkReply::dataAvailable );
    mAttachCB( uploadProgress, ODNetworkReply::uploadStatus );
}


ODNetworkReply::~ODNetworkReply()
{
    detachAllNotifiers();
    delete qnetworkreply_;
    delete qnetworkreplyconn_;
}


void ODNetworkReply::errorOccurred(CallBacker*)
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
}


void ODNetworkReply::finish(CallBacker*)
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
}


void ODNetworkReply::dataAvailable( CallBacker* )
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
}


void ODNetworkReply::uploadStatus( CallBacker* )
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
}
