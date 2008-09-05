#ifndef i_qlistview_h
#define i_qlistview_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/01/2002
 RCS:           $Id: i_qtreeview.h,v 1.11 2008-09-05 08:11:34 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uilistview.h"

#include <QObject>
#include <QTreeWidget>


//!brief Helper class for uilistview to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listVwMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListViewBody;

protected:

i_listVwMessenger( QTreeWidget& sender, uiListView& receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( &sender, SIGNAL(itemSelectionChanged()),
	     this, SLOT(itemSelectionChanged()) );

    connect( &sender, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
	     this, SLOT(itemChanged(QTreeWidgetItem*,int)) );

    connect( &sender,
	     SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), 
	     this,
	     SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );

    connect( &sender, SIGNAL(itemClicked(QTreeWidgetItem*,int)), 
	     this, SLOT(itemClicked(QTreeWidgetItem*,int)) );

    connect( &sender, SIGNAL(itemPressed(QTreeWidgetItem*,int)), 
	     this, SLOT(itemPressed(QTreeWidgetItem*,int)) );

    connect( &sender, SIGNAL(customContextMenuRequested(const QPoint &)), 
	     this, SLOT(customContextMenuRequested(const QPoint &)) );

    connect( &sender, SIGNAL(itemExpanded(QTreeWidgetItem*)), 
	     this, SLOT(itemExpanded(QTreeWidgetItem*)) );

    connect( &sender, SIGNAL(itemCollapsed(QTreeWidgetItem*)), 
	     this, SLOT(itemCollapsed(QTreeWidgetItem*)) );
}

private:

void setNotifiedItem( QTreeWidgetItem* item )
{ receiver_.setNotifiedItem(item); }

void setNotifiedColumn( int col )
{ receiver_.setNotifiedColumn( col ); }

uiListView&	receiver_;
QTreeWidget& 	sender_;

private slots:

void itemSelectionChanged()
{
    QList<QTreeWidgetItem*> items = sender_.selectedItems();
    if ( items.size() > 0 )
	setNotifiedItem( items[0] );
    receiver_.selectionChanged.trigger(receiver_);
}

void itemChanged( QTreeWidgetItem* item, int column )
{
    setNotifiedItem( item );
    receiver_.itemChanged.trigger(receiver_);
}

void currentItemChanged( QTreeWidgetItem* item, QTreeWidgetItem* )
{
    setNotifiedItem( item );
    receiver_.currentChanged.trigger(receiver_);
}

void itemClicked( QTreeWidgetItem* item, int col )
{
    setNotifiedItem( item );
    setNotifiedColumn( col );
    if ( receiver_.buttonstate_ == OD::RightButton )
	receiver_.rightButtonClicked.trigger( receiver_ );
    else if ( receiver_.buttonstate_ == OD::LeftButton )
	receiver_.leftButtonClicked.trigger( receiver_ );

    receiver_.mouseButtonClicked.trigger( receiver_ );
}

void itemPressed( QTreeWidgetItem* item, int col )
{
    setNotifiedItem( item );
    setNotifiedColumn( col );
    if ( receiver_.buttonstate_ == OD::RightButton )
	receiver_.rightButtonPressed.trigger( receiver_ );
    else if ( receiver_.buttonstate_ == OD::NoButton )
	receiver_.leftButtonPressed.trigger( receiver_ );

    receiver_.mouseButtonPressed.trigger( receiver_ );
}

void customContextMenuRequested( const QPoint& qpoint )
{
    setNotifiedItem( sender_.itemAt(qpoint) );
    receiver_.setNotifiedColumn( sender_.columnAt(qpoint.x()) );
    receiver_.contextMenuRequested.trigger(receiver_);
}

void itemExpanded( QTreeWidgetItem* item )
{
    setNotifiedItem( item );
    receiver_.expanded.trigger(receiver_);
}

void itemCollapsed( QTreeWidgetItem* item )
{
    setNotifiedItem( item );
    receiver_.collapsed.trigger(receiver_);
}

};

#endif
