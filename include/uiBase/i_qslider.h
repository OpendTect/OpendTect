#ifndef i_qslider_h
#define i_qslider_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qslider.h,v 1.1 2001-02-16 17:01:37 arend Exp $
________________________________________________________________________

-*/

#include <uislider.h>

#include <qobject.h>
#include <qwidget.h>
#include <qslider.h> 

class QString;

//! Helper class for uislider to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_SliderMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiSlider;

protected:
			i_SliderMessenger( QSlider*  sender,
					   uiSlider* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( sliderMoved(int)),
				     this,   SLOT( sliderMoved(int)) );
			    connect(sender,SIGNAL(valueChanged(int)),
				     this, SLOT(valueChanged(int)));
			}

    virtual		~i_SliderMessenger() {}
   
private:

    uiSlider* 	_receiver;
    QSlider*  	_sender;

private slots:

    void 		sliderMoved(int) 
			{ _receiver->sliderMoved.trigger(*_receiver); }
    void 		valueChanged(int)
			{ _receiver->valueChanged.trigger(*_receiver); }

};

#endif
