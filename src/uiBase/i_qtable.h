#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitable.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>


//! Helper class for uiTable to relay Qt's 'activated' messages to uiAction.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_tableMessenger : public QObject
{
Q_OBJECT
friend class uiTableBody;

protected:
i_tableMessenger( QTableWidget* sndr, uiTable* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QTableWidget::cellChanged,
	     this, &i_tableMessenger::valueChanged );

    connect( sndr, &QTableWidget::cellClicked,
	     this, &i_tableMessenger::clicked );

    connect( sndr, &QTableWidget::cellPressed,
	     this, &i_tableMessenger::cellPressed );

    connect( sndr, &QTableWidget::cellDoubleClicked,
	     this, &i_tableMessenger::doubleClicked );

    connect( sndr, &QTableWidget::itemSelectionChanged,
	     this, &i_tableMessenger::itemSelectionChanged );

    connect( sndr, &QTableWidget::cellEntered,
	     this, &i_tableMessenger::cellEntered );

    connect( sndr->verticalHeader(), &QHeaderView::sectionClicked,
	     this, &i_tableMessenger::rowClicked );
    connect( sndr->horizontalHeader(), &QHeaderView::sectionClicked,
	     this, &i_tableMessenger::columnClicked );

    connect( sndr->verticalHeader(), &QHeaderView::sectionPressed,
	     this, &i_tableMessenger::rowPressed );
    connect( sndr->horizontalHeader(), &QHeaderView::sectionPressed,
	     this, &i_tableMessenger::columnPressed );

    connect( sndr->verticalHeader(), &QHeaderView::sectionDoubleClicked,
	     this, &i_tableMessenger::rowDoubleClicked );
    connect( sndr->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
	     this, &i_tableMessenger::columnDoubleClicked );
}


~i_tableMessenger()
{}


private:

    QTableWidget*	sender_;
    uiTable*		receiver_;
    int			lastpressedheaderidx_	= -1;

private slots:

#define mTriggerBody( notifier, row, col, triggerstatement ) \
{ \
    BufferString msg = #notifier; \
    msg += " "; msg += row;  \
    msg += " "; msg += col; \
    const int refnr = receiver_->beginCmdRecEvent( msg ); \
    triggerstatement; \
    receiver_->endCmdRecEvent( refnr, msg ); \
}

#define mNoTrigger( notifier, row, col ) \
    mTriggerBody( notifier, row, col, )

#define mTrigger( notifier, row, col ) \
    mTriggerBody( notifier, row, col, receiver_->notifier.trigger(*receiver_) )

#define mHeaderTriggerBody( notifier, idx, vertical, triggerstatement ) \
    mTriggerBody( notifier, (vertical ? idx : -1), (vertical ? -1 : idx), \
		  triggerstatement )

#define mNoHeaderTrigger( notifier, idx, vertical ) \
    mHeaderTriggerBody( notifier, idx, vertical, )

#define mHeaderTrigger( notifier, idx, vertical ) \
    mHeaderTriggerBody( notifier, idx, vertical, \
			receiver_->notifier.trigger(idx, *receiver_ ) )


void valueChanged( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    mTrigger( valueChanged, row, col );
}


void clicked( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    if ( receiver_->buttonstate_ == OD::RightButton )
	mTrigger( rightClicked, row, col )
    else if ( receiver_->buttonstate_ == OD::LeftButton )
	mTrigger( leftClicked, row, col );
}


void doubleClicked( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    mTrigger( doubleClicked, row, col );
}


void itemSelectionChanged()
{
    mTrigger( selectionChanged, -1, -1 );
}


void cellPressed( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    mNoTrigger( cellPressed, row, col );
}


void cellEntered( int row, int col )
{
    mNoTrigger( cellEntered, row, col );
}


void rowClicked( int idx )
{
    // Trigger is conditional to repair Qt inconsistency. Unlike tables and
    // and lists, the header signals a final click after mouse dragging.
    if ( idx == lastpressedheaderidx_ )
	mHeaderTrigger( rowClicked, idx, true );
}


void columnClicked( int idx )
{
    // Trigger is conditional to repair Qt inconsistency. Unlike tables and
    // and lists, the header signals a final click after mouse dragging.
    if ( idx == lastpressedheaderidx_ )
	mHeaderTrigger( columnClicked, idx, false );
}


void rowPressed( int idx )
{
    lastpressedheaderidx_ = idx;
    mNoHeaderTrigger( rowPressed, idx, true );
}


void columnPressed( int idx )
{
    lastpressedheaderidx_ = idx;
    mNoHeaderTrigger( columnPressed, idx, false );
}


void rowDoubleClicked( int idx )
{
    mNoHeaderTrigger( rowDoubleClicked, idx, true );
}


void columnDoubleClicked( int idx )
{
    mNoHeaderTrigger( columnDoubleClicked, idx, false );
}


#undef mNoTrigger
#undef mTrigger
#undef mTriggerBody
#undef mHeaderTriggerBody
#undef mNoHeaderTrigger
#undef mHeaderTrigger

};

QT_END_NAMESPACE
