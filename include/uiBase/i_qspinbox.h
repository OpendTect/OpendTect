#ifndef i_qspinbox_h
#define i_qspinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qspinbox.h,v 1.5 2007-03-02 16:25:31 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"

#include <qobject.h>
#include <qwidget.h>
#include <qspinbox.h> 

class QString;

//! Helper class for uiSpinBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/


#ifdef USEQT3
# define mQSpinBox      QSpinBox
#else
# define mQSpinBox      QDoubleSpinBox
#endif

class i_SpinBoxMessenger : public QObject 
{
    Q_OBJECT
    friend class uiSpinBoxBody;

protected:
			i_SpinBoxMessenger(mQSpinBox*  sender,
					   uiSpinBox* receiver)
			: _sender(sender)
			, _receiver(receiver)
			{ 
#ifdef USEQT3
			    connect(sender,SIGNAL(valueChanged(int)),
				    this, SLOT(valueChanged(int)));
#else
			    connect(sender,SIGNAL(editingFinished()),
				    this, SLOT(editingFinished()));
#endif
			}

    virtual		~i_SpinBoxMessenger() {}
   
private:

    uiSpinBox*		_receiver;
    mQSpinBox*  	_sender;

private slots:

#ifdef USEQT3
    void 		valueChanged(int)
			{ _receiver->valueChanged.trigger(*_receiver); }
#else
    void		editingFinished()
			{ _receiver->valueChanged.trigger(*_receiver); }
#endif
};

#endif
