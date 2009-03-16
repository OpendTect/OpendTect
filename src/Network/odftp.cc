/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odftp.cc,v 1.3 2009-03-16 12:33:57 cvsnanne Exp $";

#include "odftp.h"

#include <QFtp>


ODFtp::ODFtp()
    : commandFinished(this)
    , commandStarted(this)
    , dataTransferProgress(this)
    , done(this)
    , listInfo(this)
    , readyRead(this)
    , stateChanged(this)
    , qftp_(new QFtp)
{
    error_ = false;
    nrdone_ = 0;
    totalnr_ = 0;
    commandid_ = 0;
    connectionstate_ = 0;
}
