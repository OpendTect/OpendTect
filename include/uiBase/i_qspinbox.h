#ifndef i_qspinbox_h
#define i_qspinbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qspinbox.h,v 1.7 2008-01-31 07:34:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"
#include <QDoubleSpinBox>

class QString;

//! Helper class for uiSpinBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/


class i_SpinBoxMessenger : public QObject 
{
    Q_OBJECT
    friend class uiSpinBoxBody;

protected:
			i_SpinBoxMessenger(QDoubleSpinBox*  sender,
					   uiSpinBox* receiver)
			: sender_(sender)
			, receiver_(receiver)
			{ 
			    connect( sender, SIGNAL(editingFinished()),
				     this, SLOT(editingFinished()) );
			    connect(sender, SIGNAL(valueChanged(double)),
				    this, SLOT(valueChanged(double)) );
			}

    virtual		~i_SpinBoxMessenger() {}
   
private:

    uiSpinBox*		receiver_;
    QDoubleSpinBox*  	sender_;

private slots:

    void		editingFinished()
			{ receiver_->valueChanged.trigger(*receiver_); }
    void 		valueChanged(double)
			{ receiver_->valueChanging.trigger(*receiver_); }
};

#endif
