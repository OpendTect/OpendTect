#ifndef i_qthumbwhl_h
#define i_qthumbwhl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: i_qthumbwhl.h,v 1.9 2009-10-07 13:26:33 cvsjaap Exp $
________________________________________________________________________

-*/

#include <uithumbwheel.h>

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

private:

    uiThumbWheel* 	_receiver;
    SoQtThumbWheel*  	_sender;

#define mTrigger( notifier ) \
    const int refnr = _receiver->beginCmdRecEvent( #notifier ); \
    _receiver->notifier.trigger(*_receiver); \
    _receiver->endCmdRecEvent( refnr, #notifier );

private slots:

    void 		wheelPressed(void) 
			{ mTrigger( wheelPressed ); }
    void 		wheelMoved(float val)
			{ 
			    _receiver->lastmv = val;
			    mTrigger( wheelMoved ); 
			}
    void 		wheelReleased(void)
			{ mTrigger( wheelReleased ); }

};

#endif
