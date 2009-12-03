/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odftp.cc,v 1.6 2009-12-03 03:18:45 cvsnanne Exp $";

#include "odftp.h"

#include <QFtp>


ODFtp::ODFtp()
    : commandFinished(this)
    , commandStarted(this)
    , dataTransferProgress(this)
    , done(this)
    , readyRead(this)
    , stateChanged(this)

    , connected(this)
    , loginDone(this)
    , listDone(this)
    , qftp_(new QFtp)
{
    error_ = false;
    nrdone_ = 0;
    totalnr_ = 0;
    commandid_ = 0;
    connectionstate_ = 0;
}


int ODFtp::connectToHost( const char* host, int port )
{ return qftp_->connectToHost( host, port ); }

int ODFtp::login( const char* usrnm, const char* passwd )
{ return qftp_->login( usrnm, passwd ); }

int ODFtp::close()
{ return qftp_->close(); }

void ODFtp::abort()
{ qftp_->abort(); }

int ODFtp::get( const char* file )
{ return qftp_->get( file ); }
// TODO: use QIODevice


int ODFtp::put( const char* file )
{ return qftp_->put( 0, file ); }
// TODO: use QIODevice

int ODFtp::cd( const char* dir )
{ return qftp_->cd( dir ); }

int ODFtp::rename( const char* oldname, const char* newname )
{ return qftp_->rename( oldname, newname ); }

int ODFtp::remove( const char* file )
{ return qftp_->remove( file ); }

int ODFtp::mkdir( const char* dir )
{ return qftp_->mkdir( dir ); }

int ODFtp::rmdir( const char* dir )
{ return qftp_->rmdir( dir ); }

bool ODFtp::hasPendingCommands() const
{ return qftp_->hasPendingCommands(); }

const char* ODFtp::error() const
{
    return 0;
}

const BufferStringSet& ODFtp::fileList() const
{ return filelist_; }

const BufferStringSet& ODFtp::dirList() const
{ return dirlist_; }
