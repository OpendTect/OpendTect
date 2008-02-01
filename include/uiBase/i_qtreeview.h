#ifndef i_qlistview_h
#define i_qlistview_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/01/2002
 RCS:           $Id: i_qtreeview.h,v 1.7 2008-02-01 05:22:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uilistview.h"

#include <QObject>
#include <string.h>
#include <q3listview.h>



//! Helper class for uilistview to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listVwMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListViewBody;

protected:
			i_listVwMessenger( Q3ListView&  sender, 
					   uiListView& receiver )
			    : _sender( sender ) , _receiver( receiver )
    { 

	connect( &sender, SIGNAL( selectionChanged( Q3ListViewItem* )), 
		 this, SLOT( selectionChanged( Q3ListViewItem* )));

	connect( &sender, SIGNAL( currentChanged( Q3ListViewItem* )), 
		 this, SLOT( currentChanged( Q3ListViewItem* )));

	connect( &sender, SIGNAL( clicked( Q3ListViewItem* )), 
		 this, SLOT( clicked( Q3ListViewItem* )));

	connect( &sender, SIGNAL( pressed( Q3ListViewItem* )), 
		 this, SLOT( pressed( Q3ListViewItem* )));

	connect( &sender, SIGNAL( doubleClicked( Q3ListViewItem* )), 
		 this, SLOT( doubleClicked( Q3ListViewItem* )));

	connect( &sender, SIGNAL( returnPressed( Q3ListViewItem* )), 
		 this, SLOT( returnPressed( Q3ListViewItem* )));

	connect( &sender, SIGNAL( spacePressed( Q3ListViewItem* )), 
		 this, SLOT( spacePressed( Q3ListViewItem* )));

	connect( &sender, SIGNAL( 
		    rightButtonClicked( Q3ListViewItem* , const QPoint&, int )), 
		 this, SLOT( 
		    rightButtonClicked(Q3ListViewItem* , const QPoint&, int )));

	connect( &sender, SIGNAL( 
		    rightButtonPressed( Q3ListViewItem* , const QPoint&, int )), 
		 this, SLOT( 
		    rightButtonPressed(Q3ListViewItem* , const QPoint&, int )));

	connect( &sender, SIGNAL( 
		    mouseButtonPressed(int,Q3ListViewItem*,const QPoint&,int)),
		 this, SLOT( 
		    mouseButtonPressed(int,Q3ListViewItem*,const QPoint&,int)));

	connect( &sender, SIGNAL( 
		    mouseButtonClicked(int,Q3ListViewItem*,const QPoint&,int)), 
		 this, SLOT( 
		    mouseButtonClicked(int,Q3ListViewItem*,const QPoint&,int)));

	connect( &sender, SIGNAL( 
		    contextMenuRequested(Q3ListViewItem*,const QPoint &,int)), 
		 this, SLOT( 
		    contextMenuRequested(Q3ListViewItem*, const QPoint &,int)));

	connect( &sender, SIGNAL( onItem( Q3ListViewItem* )), 
		 this, SLOT( onItem( Q3ListViewItem* )));

	connect( &sender, SIGNAL( expanded( Q3ListViewItem* )), 
		 this, SLOT( expanded( Q3ListViewItem* )));

	connect( &sender, SIGNAL(collapsed(Q3ListViewItem*)), 
		 this, SLOT( collapsed( Q3ListViewItem* )));

	connect( &sender, SIGNAL( itemRenamed( Q3ListViewItem*, int )), 
		 this, SLOT( itemRenamed( Q3ListViewItem*, int )));
	
    }

    virtual		~i_listVwMessenger() {}

    void		setNotifiedItem( Q3ListViewItem* item )
			    { _receiver.setNotifiedItem(item); }
private:

    uiListView&		_receiver;
    Q3ListView& 	_sender;

private slots:

    void	selectionChanged( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.selectionChanged.trigger(_receiver);
		}

    void	currentChanged( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.currentChanged.trigger(_receiver);
		}

    void	clicked( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.clicked.trigger(_receiver);
		}

    void	pressed( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.pressed.trigger(_receiver);
		}

    void	doubleClicked( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.doubleClicked.trigger(_receiver);
		}

    void	returnPressed( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.returnPressed.trigger(_receiver);
		}

    void	spacePressed( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.spacePressed.trigger(_receiver);
		}

    void	rightButtonClicked( Q3ListViewItem* item, const QPoint&,
	    			    int col )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.rightButtonClicked.trigger(_receiver);
		}

    void	rightButtonPressed( Q3ListViewItem* item, const QPoint&,
	    			    int col )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.rightButtonPressed.trigger(_receiver);
		}

    void	mouseButtonPressed( int, Q3ListViewItem* item,const QPoint&,
	    			    int col )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.mouseButtonPressed.trigger(_receiver);
		}

    void	mouseButtonClicked( int, Q3ListViewItem* item, const QPoint&,
	    			    int col )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.mouseButtonClicked.trigger(_receiver);
		}

    void	contextMenuRequested( Q3ListViewItem* item, const QPoint&,
	    			      int col )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.contextMenuRequested.trigger(_receiver);
		}

    void	onItem( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.onItem.trigger(_receiver);
		}

    void	expanded( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.expanded.trigger(_receiver);
		}

    void	collapsed( Q3ListViewItem* item )
		{
		    setNotifiedItem( item );
		    _receiver.collapsed.trigger(_receiver);
		}

    void	itemRenamed( Q3ListViewItem* item, int col  )
		{
		    setNotifiedItem( item );
		    _receiver.setNotifiedColumn( col );
		    _receiver.itemRenamed.trigger(_receiver);
		}
};

#endif
