#ifndef qlocalservercomm_h
#define qlocalservercomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include <QLocalServer>
#include "localserver.h"

/*\brief QLocalServer communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

class QLocalServerComm : public QObject 
{
    Q_OBJECT
    friend class	LocalServer;

protected:

QLocalServerComm( QLocalServer* qlocalserver, LocalServer* localserver )
    : qlocalserver_(qlocalserver)
    , localserver_(localserver)
{
    connect( qlocalserver, SIGNAL(newConnection()),
	     this, SLOT(newConnection()) );
}

private slots:

void newConnection()
{ 
    localserver_->newConnection.trigger( *localserver_ );
}

private:

    QLocalServer*		qlocalserver_;
    LocalServer*		localserver_;

};

#endif
