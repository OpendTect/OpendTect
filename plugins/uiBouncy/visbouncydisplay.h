#ifndef visbouncydisplay_h
#define visbouncydsiplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: visbouncydisplay.h,v 1.1 2009-09-14 22:50:45 cvskarthika Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class SoRotation;

namespace visBase { class BeachBall; class EventCatcher; }
namespace visBeachBall { class BallProperties; }

namespace uiBouncy
{

/*! \brief 
Display a bouncing beachball. 
Controls: 
ESC - quit game
Space - pause/resume
Mouse move - move paddle
+/- - increase/decrease speed of ball
*/
mClass BouncyDisplay : public visBase::VisualObjectImpl
{
public:

    static BouncyDisplay*	create()
    				mCreateDataObj(BouncyDisplay);

    void			setSceneID(const int);
    int				sceneid() const;

    visBase::BeachBall*		beachball() const;

    void                        addBouncy(visBeachBall::BallProperties);
    void			removeBouncy();
    void                        setBallScale();
    void			start();
    void			stop();
    bool			ispaused() const;
    bool			isstopped() const;

    Notifier<BouncyDisplay>	newEvent;

protected:
    				
    ~BouncyDisplay();

    void			zScaleCB(CallBacker*);
    void			eventCB(CallBacker*);
    void			keyPressCB(CallBacker*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    visBase::BeachBall*		bb_;
    SoRotation*			rotation_;
    int				sceneid_;
    visBase::EventCatcher*	eventcatcher_;
    bool			ispaused_;
    bool			isstopped_;

};
};

#endif
