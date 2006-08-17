/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: odftp.cc,v 1.1 2006-08-17 19:49:36 cvsnanne Exp $
________________________________________________________________________

-*/


ODFtp::ODFtp()
    : commandFinished(this)
    , commandStarted(this)
    , dataTransferProgress(this)
    , done(this)
    , listInfo(this)
    , readyRead(this)
    , stateChanged(this)
    , qftp_(*new QFtp)
{
    error_ = false;
    nrdone_ = 0;
    totalnr_ = 0;
    commandid_ = 0;
    connectionstate_ = 0;
}
