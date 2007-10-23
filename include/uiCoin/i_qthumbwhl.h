#ifndef i_qthumbwhl_h
#define i_qthumbwhl_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: i_qthumbwhl.h,v 1.4 2007-10-23 11:24:25 cvsjaap Exp $
________________________________________________________________________

-*/

#include <uithumbwheel.h>

#include <qevent.h>
#include <qobject.h>
#include <Inventor/Qt/widgets/SoQtThumbWheel.h>

class QString;

//! Helper class for uiThumbWheel to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_ThumbWheelMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiThumbWheelBody;

protected:
			i_ThumbWheelMessenger( SoQtThumbWheel*  sender,
					   uiThumbWheel* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( wheelPressed(void)),
				     this,   SLOT( wheelPressed(void)) );
			    connect( sender, SIGNAL( wheelMoved(float)),
				     this,   SLOT( wheelMoved(float)) );
			    connect( sender, SIGNAL( wheelReleased(void)),
				     this,   SLOT( wheelReleased(void)) );
			}

    virtual		~i_ThumbWheelMessenger() {}

    bool		event( QEvent* ev )
			{ return _receiver->handleEvent(ev)
			    			? true : QObject::event(ev); }

private:

    uiThumbWheel* 	_receiver;
    SoQtThumbWheel*  	_sender;

private slots:

    void 		wheelPressed(void) 
			{ _receiver->wheelPressed.trigger(*_receiver); }
    void 		wheelMoved(float val)
			{ 
			    _receiver->lastmv = val;
			    _receiver->wheelMoved.trigger(*_receiver); 
			}
    void 		wheelReleased(void)
			{ _receiver->wheelReleased.trigger(*_receiver); }

};

#endif
