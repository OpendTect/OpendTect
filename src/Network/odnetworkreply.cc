/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
    error.notify( mCB(this,ODNetworkReply,errorOccurred) );
    finished.notify( mCB(this,ODNetworkReply,finish) );
    readyRead.notify( mCB(this,ODNetworkReply,dataAvailable) );
    uploadProgress.notify( mCB(this,ODNetworkReply,uploadStatus) );
}


ODNetworkReply::~ODNetworkReply()
{ 
    delete qnetworkreply_;
    delete qnetworkreplyconn_;
}


bool ODNetworkReply::errorOccurred(CallBacker*)
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
}


bool ODNetworkReply::finish(CallBacker*)
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
		
    return true;
}


bool ODNetworkReply::dataAvailable( CallBacker* )
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
}


bool ODNetworkReply::uploadStatus( CallBacker* )
{
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
}
