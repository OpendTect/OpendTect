#ifndef i_qtable_h
#define i_qtable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: i_qtable.h,v 1.7 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitable.h"

#include <QHeaderView>
#include <QObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>


//! Helper class for uiTable to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_tableMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTableBody;

protected:
i_tableMessenger( QTableWidget*  sndr, uiTable* receiver )
    : sender_(sndr)
    , receiver_(receiver)
    , lastpressedheaderidx_(-1)
{ 
    connect( sndr, SIGNAL(cellChanged(int,int)),
	     this, SLOT(valueChanged(int,int)) );

    connect( sndr, SIGNAL(cellClicked(int,int)),
	     this, SLOT(clicked(int,int)) );

    connect( sndr, SIGNAL(cellPressed(int,int)),
	     this, SLOT(cellPressed(int,int)) );

    connect( sndr, SIGNAL(cellDoubleClicked(int,int)),
	     this, SLOT(doubleClicked(int,int)) );

    connect( sndr, SIGNAL(itemSelectionChanged()),
	     this, SLOT(itemSelectionChanged()) );

    connect( sndr, SIGNAL(cellEntered(int,int)),
	     this, SLOT(cellEntered(int,int)) );

    connect( sndr->verticalHeader(), SIGNAL(sectionClicked(int)),
	     this, SLOT(rowClicked(int)) );
    connect( sndr->horizontalHeader(), SIGNAL(sectionClicked(int)),
	     this, SLOT(columnClicked(int)) );

    connect( sndr->verticalHeader(), SIGNAL(sectionPressed(int)),
	     this, SLOT(rowPressed(int)) );
    connect( sndr->horizontalHeader(), SIGNAL(sectionPressed(int)),
	     this, SLOT(columnPressed(int)) );

    connect( sndr->verticalHeader(), SIGNAL(sectionDoubleClicked(int)),
	     this, SLOT(rowDoubleClicked(int)) );
    connect( sndr->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)),
	     this, SLOT(columnDoubleClicked(int)) );
}

    virtual		~i_tableMessenger() {}
   
private:

    uiTable* 		receiver_;
    QTableWidget*  	sender_;
    int			lastpressedheaderidx_;

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
{ mTrigger( selectionChanged, -1, -1 ); }


void cellPressed( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    mNoTrigger( cellPressed, row, col );
}


void cellEntered( int row, int col )
{ mNoTrigger( cellEntered, row, col ); }


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
{ mNoHeaderTrigger( rowDoubleClicked, idx, true ); }


void columnDoubleClicked( int idx )
{ mNoHeaderTrigger( columnDoubleClicked, idx, false ); }


};

#endif
