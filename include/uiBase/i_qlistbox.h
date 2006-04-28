#ifndef i_q4listbox_h
#define i_q4listbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.2 2006-04-28 15:23:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>

#include <qobject.h>
#include <qwidget.h>
#include <string.h>

#include <q3listbox.h>
#define Q3ListBox	Q3ListBox
#define Q3ListBoxItem  Q3ListBoxItem

//! Helper class for uiListBox to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListBoxBody;

protected:
			i_listMessenger( Q3ListBox*  sender,
					 uiListBox* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( selectionChanged ()),
				     this,   SLOT( selectionChanged ()));

			    connect( sender, 
				     SIGNAL( doubleClicked(Q3ListBoxItem*) ),
				     this, SLOT( doubleClicked(Q3ListBoxItem*)));

			    connect( sender, SIGNAL( 
			      rightButtonClicked(Q3ListBoxItem*,const QPoint&)),
			      this, SLOT( 
			      rightButtonClicked(Q3ListBoxItem*,const QPoint&)));

			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		_receiver;
    Q3ListBox*  	_sender;

private slots:


/*!
    Handler for selectionChanged events.
    \sa QListBox::selectionChanged
*/
    void 		selectionChanged() 
			{_receiver->selectionChanged.trigger(*_receiver);}
    
/*!
    Handler for doubleClicked events.
    \sa QListBox::doubleClicked
*/
    void		doubleClicked( Q3ListBoxItem * item )
			{
			    int idx = _sender->index(item);
			    _receiver->lastClicked_ = idx;
			    _receiver->doubleClicked.trigger(*_receiver);
			}

/*!
    Handler for rightButtonClicked events.
    \sa QListBox::rightButtonClicked
*/
    void		rightButtonClicked( Q3ListBoxItem* item, const QPoint&)
			{
			    int idx = _sender->index(item);
			    _receiver->lastClicked_ = idx;
			    _receiver->rightButtonClicked.trigger(*_receiver);
			}

};

#endif
