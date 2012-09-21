#ifndef i_qlistbox_h
#define i_qlistbox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uilistbox.h"

#include <QListWidget>
#include <QObject>

//! Helper class for uiListBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_listMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListBoxBody;

protected:
			i_listMessenger( QListWidget* sndr,
					 uiListBox* receiver )
			: sender_( sndr )
			, receiver_( receiver )
			{
			    connect( sndr,
				SIGNAL(itemDoubleClicked(QListWidgetItem*)),
				this,
				SLOT(itemDoubleClicked(QListWidgetItem*)) );

			    connect( sndr,
				SIGNAL(itemClicked(QListWidgetItem*)),
				this,
				SLOT(itemClicked(QListWidgetItem*)) );

			    connect( sndr, SIGNAL(itemSelectionChanged()),
				     this, SLOT(itemSelectionChanged()) );

			    connect( sndr,
				 SIGNAL(itemEntered(QListWidgetItem*)),
				 this, SLOT(itemEntered(QListWidgetItem*)) );

			    connect( sndr,
				SIGNAL(itemChanged(QListWidgetItem*)),
				this, SLOT(itemChanged(QListWidgetItem*)) );
			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		receiver_;
    QListWidget*  	sender_;


#define mTrigger( notifier, itm ) \
{ \
    BufferString msg = #notifier; \
    if ( itm ) \
    { \
	QListWidgetItem* qlwi = itm; \
	msg += " "; msg += qlwi->listWidget()->row( qlwi ); \
    } \
    const int refnr = receiver_->beginCmdRecEvent( msg ); \
    receiver_->notifier.trigger( *receiver_ ); \
    receiver_->endCmdRecEvent( refnr, msg ); \
}

private slots:

void itemDoubleClicked( QListWidgetItem* itm )
{ mTrigger( doubleClicked, itm ); }


void itemClicked( QListWidgetItem* itm )
{
    if ( receiver_->buttonstate_ == OD::RightButton )
	mTrigger( rightButtonClicked, itm )
    else if ( receiver_->buttonstate_ == OD::LeftButton )
	mTrigger( leftButtonClicked, itm );
}

void itemSelectionChanged()
{
    mTrigger( selectionChanged, 0 );
}


void itemEntered( QListWidgetItem* itm )
{
    const int refnr = receiver_->beginCmdRecEvent( "itemEntered" );
    receiver_->endCmdRecEvent( refnr, "itemEntered" );
}


void itemChanged( QListWidgetItem* itm )
{
    receiver_->handleCheckChange( itm );
}

};

QT_END_NAMESPACE

#endif
