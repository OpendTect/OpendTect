#ifndef i_q4table_h
#define i_q4table_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: i_qtable.h,v 1.1 2008-02-01 05:02:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitable.h"

#include <QObject>
#include <QTableWidget>
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
			i_tableMessenger( QTableWidget*  sender,
					  uiTable* receiver )
			: sender_( sender )
			, receiver_( receiver )
			{ 
			    connect( sender, SIGNAL(cellChanged(int,int)),
				     this,   SLOT(valueChanged(int,int)) );

			    connect( sender, 
				     SIGNAL(cellClicked(int,int)),
				     this, SLOT(clicked(int,int)) );

			    connect( sender, 
				     SIGNAL(cellDoubleClicked(int,int)),
				     this, SLOT(doubleClicked(int,int)) );
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
    receiver_->doubleClicked.trigger(*receiver_);
}

};

#endif
