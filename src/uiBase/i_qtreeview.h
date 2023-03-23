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
friend class uiTreeViewBody;

protected:

i_treeVwMessenger( QTreeWidget& sndr, uiTreeView& rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( &sndr, &QTreeWidget::itemSelectionChanged,
	     this, &i_treeVwMessenger::itemSelectionChanged );

    connect( &sndr, &QTreeWidget::itemChanged,
	     this, &i_treeVwMessenger::itemChanged );

    connect( &sndr, &QTreeWidget::currentItemChanged,
	     this, &i_treeVwMessenger::currentItemChanged );

    connect( &sndr, &QTreeWidget::itemClicked,
	     this, &i_treeVwMessenger::itemClicked );

    connect( &sndr, &QTreeWidget::itemPressed,
	     this, &i_treeVwMessenger::itemPressed );

    connect( &sndr, &QWidget::customContextMenuRequested,
	     this, &i_treeVwMessenger::customContextMenuRequested );

    connect( &sndr, &QTreeWidget::itemExpanded,
	     this, &i_treeVwMessenger::itemExpanded );

    connect( &sndr, &QTreeWidget::itemCollapsed,
	     this, &i_treeVwMessenger::itemCollapsed );

    connect( &sndr, &QTreeWidget::itemDoubleClicked,
	     this, &i_treeVwMessenger::itemDoubleClicked );

    connect( &sndr, &QTreeWidget::itemEntered,
	     this, &i_treeVwMessenger::itemEntered );
}


~i_treeVwMessenger()
{}

private:

void setNotifiedItem( QTreeWidgetItem* item )
{
    receiver_.setNotifiedItem(item);
}

void setNotifiedColumn( int col )
{
    receiver_.setNotifiedColumn( col );
}

    QTreeWidget&	sender_;
    uiTreeView&		receiver_;


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
