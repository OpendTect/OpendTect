#ifndef i_q4listbox_h
#define i_q4listbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.6 2007-05-10 05:43:45 cvsnanne Exp $
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
			: _sender( sender )
			, _receiver( receiver )
			{
			    connect( sender,
				SIGNAL(currentItemChanged(QListWidgetItem*,
							  QListWidgetItem*)),
				this,
				SLOT(currentItemChanged(QListWidgetItem*,
							QListWidgetItem*)) );

			    connect( sender,
				SIGNAL(itemDoubleClicked(QListWidgetItem*)),
				this,
				SLOT(doubleClicked(QListWidgetItem*)) );

			    connect( sender, SIGNAL(itemSelectionChanged()),
				     this, SLOT(itemSelectionChanged()) );

			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		_receiver;
    QListWidget*  	_sender;

private slots:


    void 		currentItemChanged( QListWidgetItem* cur,
					    QListWidgetItem* prev ) 
			{
			    if ( _receiver->lastclicked_ == -2 ) return;

			    if ( !cur ) cur = prev;
			    if ( !cur ) return;
			    cur->setSelected( true );
			    _receiver->lastclicked_ = _sender->row( cur );
			    _receiver->currentItemChanged.trigger( *_receiver );
			}
    
    void		doubleClicked( QListWidgetItem* cur )
			{
			    if ( _receiver->lastclicked_ == -2 ) return;

			    int idx = _sender->row( cur );
			    _receiver->lastclicked_ = idx;
			    _receiver->doubleClicked.trigger( *_receiver );
			}

    void		itemSelectionChanged()
			{
			    if ( _receiver->lastclicked_ == -2 ) return;

			    _receiver->lastclicked_ =
					_sender->row( _sender->currentItem() );
			    _receiver->selectionChanged.trigger( *_receiver );
			}

};

#endif
