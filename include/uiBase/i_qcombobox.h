#ifndef i_qcombobox_h
#define i_qcombobox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qcombobox.h,v 1.9 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicombobox.h"

#include <QComboBox> 

//! Helper class for uiComboBox to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_comboMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiComboBoxBody;

protected:
			i_comboMessenger( QComboBox*  sndr,
					 uiComboBox* receiver )
			: _sender( sndr )
			, _receiver( receiver )
			{ 
			    connect( sndr, SIGNAL( activated (int)),
				     this,   SLOT( activated (int)) );

			    connect( sndr,
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
