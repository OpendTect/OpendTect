#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitreeview.h"

#include <QTreeWidget>


//!brief Helper class for uiTreeView to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_treeVwMessenger : public QObject
{
    Q_OBJECT
    friend class	uiTreeViewBody;

protected:

i_treeVwMessenger( QTreeWidget& sndr, uiTreeView& receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( &sndr, SIGNAL(itemSelectionChanged()),
	     this, SLOT(itemSelectionChanged()) );

    connect( &sndr, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
	     this, SLOT(itemChanged(QTreeWidgetItem*,int)) );

    connect( &sndr,
	     SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
	     this,
	     SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );

    connect( &sndr, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	     this, SLOT(itemClicked(QTreeWidgetItem*,int)) );

    connect( &sndr, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
	     this, SLOT(itemPressed(QTreeWidgetItem*,int)) );

    connect( &sndr, SIGNAL(customContextMenuRequested(const QPoint &)),
	     this, SLOT(customContextMenuRequested(const QPoint &)) );

    connect( &sndr, SIGNAL(itemExpanded(QTreeWidgetItem*)),
	     this, SLOT(itemExpanded(QTreeWidgetItem*)) );

    connect( &sndr, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
	     this, SLOT(itemCollapsed(QTreeWidgetItem*)) );

    connect( &sndr, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	     this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)) );

    connect( &sndr, SIGNAL(itemEntered(QTreeWidgetItem*,int)),
	     this, SLOT(itemEntered(QTreeWidgetItem*,int)) );
}

private:

void setNotifiedItem( QTreeWidgetItem* item )
{ receiver_.setNotifiedItem(item); }

void setNotifiedColumn( int col )
{ receiver_.setNotifiedColumn( col ); }

uiTreeView&	receiver_;
QTreeWidget&	sender_;


#define mTriggerBody( notifier, triggerstatement1, triggerstatement2 ) \
{ \
    BufferString msg = #notifier; \
    const int refnr = receiver_.beginCmdRecEvent( msg ); \
    triggerstatement1; \
    triggerstatement2; \
    receiver_.endCmdRecEvent( refnr, msg ); \
}

#define mNoTrigger( notifier ) \
    mTriggerBody( notifier, , )

#define mTrigger( notifier ) \
    mTriggerBody( notifier, receiver_.notifier.trigger(receiver_), )

#define mTriggerExtra( notifier, extranotifier ) \
    mTriggerBody( notifier, receiver_.notifier.trigger(receiver_), \
		  receiver_.extranotifier.trigger(receiver_) )

private slots:

void itemSelectionChanged()
{
    QList<QTreeWidgetItem*> items = sender_.selectedItems();
    setNotifiedItem( items.size()>0 ? items[0] : 0 );
    mTrigger( selectionChanged );
}

void itemChanged( QTreeWidgetItem* item, int column )
{
    setNotifiedItem( item );
    setNotifiedColumn( column );
    mTrigger( itemChanged );
}

void currentItemChanged( QTreeWidgetItem* item, QTreeWidgetItem* )
{
    setNotifiedItem( item );
    mTrigger( currentChanged );
}

void itemClicked( QTreeWidgetItem* item, int col )
{
    setNotifiedItem( item );
    setNotifiedColumn( col );
    if ( receiver_.buttonstate_ == OD::RightButton )
	mTriggerExtra( rightButtonClicked, mouseButtonClicked )
    else if ( receiver_.buttonstate_ == OD::LeftButton )
	mTriggerExtra( leftButtonClicked, mouseButtonClicked )
    else
	mTrigger( mouseButtonClicked );
}

void itemPressed( QTreeWidgetItem* item, int col )
{
    setNotifiedItem( item );
    setNotifiedColumn( col );
    if ( receiver_.buttonstate_ == OD::RightButton )
	mTriggerExtra( rightButtonPressed, mouseButtonPressed )
    else if ( receiver_.buttonstate_ == OD::LeftButton )
	mTriggerExtra( leftButtonPressed, mouseButtonPressed )
    else
	mTrigger( mouseButtonPressed );
}

void customContextMenuRequested( const QPoint& qpoint )
{
    setNotifiedItem( sender_.itemAt(qpoint) );
    receiver_.setNotifiedColumn( sender_.columnAt(qpoint.x()) );
    mTrigger( contextMenuRequested );
}

void itemExpanded( QTreeWidgetItem* item )
{
    setNotifiedItem( item );
    mTrigger( expanded );
}

void itemCollapsed( QTreeWidgetItem* item )
{
    setNotifiedItem( item );
    mTrigger( collapsed );
}

void itemDoubleClicked( QTreeWidgetItem* item, int col )
{
    if ( !receiver_.allowDoubleClick() )
	return;

    setNotifiedItem( item );
    setNotifiedColumn( col );
    mTrigger( doubleClicked );
}

void itemEntered( QTreeWidgetItem* item, int col )
{
    setNotifiedItem( item );
    setNotifiedColumn( col );
    mNoTrigger( itemEntered );
}

#undef mTriggerBody
#undef mNoTrigger
#undef mTrigger
#undef mTriggerExtra

};

QT_END_NAMESPACE
