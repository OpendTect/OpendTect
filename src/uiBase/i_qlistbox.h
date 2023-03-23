#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilistbox.h"

#include <QListWidget>

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
i_listMessenger( QListWidget* sndr, uiListBox* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QListWidget::itemDoubleClicked,
	     this, &i_listMessenger::itemDoubleClicked );

    connect( sndr, &QListWidget::itemClicked,
	     this, &i_listMessenger::itemClicked );

    connect( sndr, &QListWidget::itemSelectionChanged,
	     this, &i_listMessenger::itemSelectionChanged );

    connect( sndr, &QListWidget::itemEntered,
	     this, &i_listMessenger::itemEntered );
    connect( sndr, &QListWidget::itemChanged,
	     this, &i_listMessenger::itemChanged );
}

~i_listMessenger()
{}

private:

    QListWidget*	sender_;
    uiListBox*		receiver_;


#define mTrigger( notifier, itm ) \
{ \
    BufferString msg = #notifier; \
    if ( itm ) \
    { \
	QListWidgetItem* qlwi = itm; \
	msg += " "; msg += qlwi->listWidget()->row( qlwi ); \
    } \
    const int refnr = receiver_->box()->beginCmdRecEvent( msg ); \
    receiver_->notifier.trigger( *receiver_ ); \
    receiver_->box()->endCmdRecEvent( refnr, msg ); \
}

private slots:

void itemDoubleClicked( QListWidgetItem* itm )
{
    mTrigger( doubleClicked, itm );
}


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
    const int refnr = receiver_->box()->beginCmdRecEvent( "itemEntered" );
    receiver_->box()->endCmdRecEvent( refnr, "itemEntered" );
}


void itemChanged( QListWidgetItem* itm )
{
    receiver_->handleCheckChange( itm );
}

#undef mTrigger
};

QT_END_NAMESPACE
