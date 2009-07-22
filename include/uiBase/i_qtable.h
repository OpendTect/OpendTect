#ifndef i_qtable_h
#define i_qtable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: i_qtable.h,v 1.4 2009-07-22 16:01:20 cvsbert Exp $
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
i_tableMessenger( QTableWidget*  sender, uiTable* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(cellChanged(int,int)),
	     this, SLOT(valueChanged(int,int)) );

    connect( sender, SIGNAL(cellClicked(int,int)),
	     this, SLOT(clicked(int,int)) );

    connect( sender, SIGNAL(cellDoubleClicked(int,int)),
	     this, SLOT(doubleClicked(int,int)) );

    connect( sender, SIGNAL(itemSelectionChanged()),
	     this, SLOT(itemSelectionChanged()) );

    connect( sender->verticalHeader(), SIGNAL(sectionClicked(int)),
	     this, SLOT(rowClicked(int)) );
    connect( sender->horizontalHeader(), SIGNAL(sectionClicked(int)),
	     this, SLOT(columnClicked(int)) );
}

    virtual		~i_tableMessenger() {}
   
private:

    uiTable* 		receiver_;
    QTableWidget*  	sender_;

private slots:

void valueChanged( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    receiver_->valueChanged.trigger(*receiver_);
}

void clicked( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    if ( receiver_->buttonstate_ == OD::RightButton )
	receiver_->rightClicked.trigger( *receiver_ );
    else if ( receiver_->buttonstate_ == OD::LeftButton )
	receiver_->leftClicked.trigger( *receiver_ );
}

void doubleClicked( int row, int col )
{
    receiver_->notifcell_ = RowCol(row,col);
    receiver_->doubleClicked.trigger( *receiver_ );
}

void itemSelectionChanged()
{ receiver_->selectionChanged.trigger( *receiver_ ); }

void rowClicked( int idx )
{ receiver_->rowClicked.trigger( idx, *receiver_ ); }

void columnClicked( int idx )
{ receiver_->columnClicked.trigger( idx, *receiver_ ); }

};

#endif
