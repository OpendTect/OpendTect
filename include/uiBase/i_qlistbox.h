#ifndef i_q4listbox_h
#define i_q4listbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.7 2007-05-14 06:48:27 cvsnanne Exp $
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

			    connect( sender, SIGNAL(itemSelectionChanged()),
				     this, SLOT(itemSelectionChanged()) );
			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		receiver_;
    QListWidget*  	sender_;

private slots:

    void		itemDoubleClicked( QListWidgetItem* cur )
			{ receiver_->doubleClicked.trigger( *receiver_ ); }

    void		itemSelectionChanged()
			{
			    QList<QListWidgetItem*> selitems =
						sender_->selectedItems();
			    if ( selitems.count() > 0 )
				sender_->setCurrentItem( selitems.first() );
			    else
				sender_->setCurrentItem( 0 );

			    receiver_->selectionChanged.trigger( *receiver_ );
			}

};

#endif
