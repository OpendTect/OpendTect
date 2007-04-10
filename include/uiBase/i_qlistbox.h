#ifndef i_q4listbox_h
#define i_q4listbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: i_qlistbox.h,v 1.3 2007-04-10 08:00:59 cvsbert Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>

#include <qobject.h>
#include <qwidget.h>
#include <string.h>

#include <QListWidget>

//! Helper class for uiListBox to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_listMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiListBoxBody;

protected:
			i_listMessenger( QListWidget*  sender,
					 uiListBox* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender,SIGNAL( itemSelectionChanged()),
				     this,SLOT( selectionChanged ()));

			    connect( sender,
				    SIGNAL(itemDoubleClicked(QListWidgetItem*)),
				    this,SLOT(doubleClicked(QListWidgetItem*)));

			}

    virtual		~i_listMessenger() {}
   
private:

    uiListBox* 		_receiver;
    QListWidget*  	_sender;

private slots:


    void 		selectionChanged() 
			{
			    _receiver->selectionChanged.trigger( *_receiver );
			}
    
    void		doubleClicked( QListWidgetItem * item )
			{
			    int idx = _sender->row(item);
			    _receiver->lastClicked_ = idx;
			    _receiver->doubleClicked.trigger(*_receiver);
			}


};

#endif
