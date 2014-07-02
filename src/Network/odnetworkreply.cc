/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkreply.h"

#ifndef OD_NO_QT
#include "qnetworkaccessconn.h"

#include <QEventLoop>
#endif


ODNetworkReply::ODNetworkReply( QNetworkReply* qnr, QEventLoop* qel )
    : downloadProgress( this )
    , finished( this )
    , metaDataChanged( this )
    , error( this )
    , uploadProgress( this )
    , aboutToClose( this )
    , bytesWritten( this )
    , readyRead( this )
#ifndef OD_NO_QT
    , qeventloop_( qel )
#else
    , qeventloop_(0)
    , qnetworkreply_(0)
    , qnetworkreplyconn_(0)
#endif
    , bytesuploaded_( 0 )
{
#ifndef OD_NO_QT
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);
#endif
    error.notify( mCB(this,ODNetworkReply,errorOccurred) );
    finished.notify( mCB(this,ODNetworkReply,finish) );
    readyRead.notify( mCB(this,ODNetworkReply,dataAvailable) );
    uploadProgress.notify( mCB(this,ODNetworkReply,uploadStatus) );
}


ODNetworkReply::~ODNetworkReply()
{ 
#ifndef OD_NO_QT
    delete qnetworkreply_;
    delete qnetworkreplyconn_;
#endif
}


bool ODNetworkReply::errorOccurred(CallBacker*)
{
#ifndef OD_NO_QT
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
#else
    return false;
#endif
}


bool ODNetworkReply::finish(CallBacker*)
{
#ifndef OD_NO_QT
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();
		
    return true;
#else
    return false;
#endif
}


bool ODNetworkReply::dataAvailable( CallBacker* )
{
#ifndef OD_NO_QT
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
#else
    return false;
#endif
}


bool ODNetworkReply::uploadStatus( CallBacker* )
{
#ifndef OD_NO_QT
    if ( qeventloop_->isRunning() )
	qeventloop_->exit();

    return true;
#else
    return false;
#endif
}
