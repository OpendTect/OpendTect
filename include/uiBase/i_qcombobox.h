#ifndef i_qComboBox_H
#define i_qComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qcombobox.h,v 1.3 2003-11-07 12:21:53 bert Exp $
________________________________________________________________________

-*/

#include <uicombobox.h>

#include <qobject.h>
#include <qwidget.h>
#include <qcombobox.h> 
#include <string.h>

//! Helper class for uiComboBox to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_comboMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiComboBoxBody;

protected:
			i_comboMessenger( QComboBox*  sender,
					 uiComboBox* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( activated (int)),
				     this,   SLOT( activated (int)) );
			}

    virtual		~i_comboMessenger() {}
   
private:

    uiComboBox* 	_receiver;
    QComboBox*  	_sender;

private slots:

/*!
    Handler for activated events.
    \sa QComboBox::activated
*/

    void 		activated( int index ) 
			{_receiver->selectionChanged.trigger(*_receiver);}

};

#endif
