#ifndef i_qptr_h
#define i_qptr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          November 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include <callback.h>
#include <thread.h>
#include <QObject>


//! Helper class for QPtr to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_QPtrImpl : public QObject, public CallBacker
{
    Q_OBJECT

public:

    Notifier<i_QPtrImpl>	notifier_;
    Threads::Mutex		lock_;

    QObject*			ptr() { return sender_; }
    const QObject*		ptr() const { return sender_; }
    
    void			set(QObject* qo )
				{
				    Threads::MutexLocker lock( lock_ );
				    
				    if ( sender_ ) sender_->disconnect( this );
				    
				    sender_ = qo;
				    if ( sender_ )
				    {
					connect( sender_,
					    SIGNAL(destroyed(QObject*)),
					    this, SLOT(destroyed(QObject*)) );
				    }
				}

				i_QPtrImpl( QObject* sndr )
				    : sender_(0)
				    , notifier_(this)
				{
				    set( sndr );
				}
    
				~i_QPtrImpl()
				{
				    set( 0 );
				}

private:

    QObject*  	sender_;

private slots:

    void			destroyed( QObject* )
				{
				    notifier_.trigger();
				    Threads::MutexLocker lock( lock_ );
				    sender_ = 0;
				}

};

QT_END_NAMESPACE

#endif
