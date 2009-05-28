#ifndef i_qlistbox_h
#define i_qlistbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.12 2009-05-28 09:08:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

#include <QListWidget>
#include <QObject>

//! Helper class for uiListBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListBoxBody;

protected:
			i_listMessenger( QListWidget* sender,
					 uiListBox* receiver )
			: sender_( sender )
			, receiver_( receiver )
			{
			    connect( sender,
				SIGNAL(itemDoubleClicked(QListWidgetItem*)),
				this,
				SLOT(itemDoubleClicked(QListWidgetItem*)) );

			    connect( sender,
				SIGNAL(itemClicked(QListWidgetItem*)),
				this,
				SLOT(itemClicked(QListWidgetItem*)) );

			    connect( sender, SIGNAL(itemSelectionChanged()),
				     this, SLOT(itemSelectionChanged()) );
			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		receiver_;
    QListWidget*  	sender_;

private slots:

void itemDoubleClicked( QListWidgetItem* cur )
{ receiver_->notifyHandler( "doubleClicked" ); }


void itemClicked( QListWidgetItem* )
{
    if ( receiver_->buttonstate_ == OD::RightButton )
	receiver_->notifyHandler( "rightButtonClicked" );
    else if ( receiver_->buttonstate_ == OD::LeftButton )
	receiver_->notifyHandler( "leftButtonClicked" );
}

void itemSelectionChanged()
{
// TODO: Remove this hack when using Qt 4.3
    QList<QListWidgetItem*> selitems = sender_->selectedItems();
    if ( selitems.count() == 0 )
	sender_->setCurrentItem( 0 );
    else if ( selitems.count() == 1 )
	sender_->setCurrentItem( selitems.first() );

    receiver_->notifyHandler( "selectionChanged" );
}

};

#endif
