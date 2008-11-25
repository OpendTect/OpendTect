/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odftp.cc,v 1.2 2008-11-25 15:35:22 cvsbert Exp $";


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
