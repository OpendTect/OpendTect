#ifndef i_qcombobox_h
#define i_qcombobox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qcombobox.h,v 1.7 2009-07-22 16:01:20 cvsbert Exp $
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

			    connect( sender,
				     SIGNAL( editTextChanged(const QString&)),
				     this,
				     SLOT( editTextChanged(const QString&)) );
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

void activated( int ) 
{ _receiver->notifyHandler( true ); }


void editTextChanged( const QString& )
{ _receiver->notifyHandler( false ); }


};

#endif
