#ifndef i_qspinbox_h
#define i_qspinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qspinbox.h,v 1.1 2001-02-16 17:01:37 arend Exp $
________________________________________________________________________

-*/

#include <uispinbox.h>

#include <qobject.h>
#include <qwidget.h>
#include <qspinbox.h> 

class QString;

//! Helper class for uiSpinBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_SpinBoxMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiSpinBox;

protected:
			i_SpinBoxMessenger( QSpinBox*  sender,
					   uiSpinBox* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect(sender,SIGNAL(valueChanged(int)),
				     this, SLOT(valueChanged(int)));
			}

    virtual		~i_SpinBoxMessenger() {}
   
private:

    uiSpinBox* 	_receiver;
    QSpinBox*  	_sender;

private slots:

    void 		valueChanged(int)
			{ _receiver->valueChanged.trigger(*_receiver); }

};

#endif
