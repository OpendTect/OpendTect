#ifndef visbouncydisplay_h
#define visbouncydsiplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: visbouncydisplay.h,v 1.5 2012/04/19 13:56:05 cvskris Exp $
________________________________________________________________________

-*/


#include "vissurvobj.h"
#include "visobject.h"

class SoRotation;

namespace visBase { 
    class BeachBall; 
    class EventCatcher; 
    class Cube; 
    class Transformation;
}
namespace visBeachBall { class BallProperties; }

namespace uiBouncy
{

/*! \brief 
Display a bouncing beachball. 
Mouse control: 
X, Y axes of mouse correspond to the cross-line and in-line axes respectively.
Keyboard controls:
Left & right arrow keys - cross-line axis
Up & down keys - in-line axis
Q - quit game
Space - pause/resume
Mouse move - move paddle
+/- - increase/decrease speed of ball
*/
mClass BouncyDisplay : public visBase::VisualObjectImpl,
		       public visSurvey::SurveyObject
{
public:

    static BouncyDisplay*	create()
    				mCreateDataObj(BouncyDisplay);

    void			setSceneID(const int);
    int				sceneid() const;

    void                        addBouncy(visBeachBall::BallProperties);
    void			removeBouncy();
    void                        setScale();
    void			start();
    void			stop();
    bool			ispaused() const;
    bool			isstopped() const;

    Notifier<BouncyDisplay>	newEvent;    

    // beachball access methods
    void                        setBallProperties(const
	    				visBeachBall::BallProperties&);
    visBeachBall::BallProperties getBallProperties() const;

    void			setBallPosition(const Coord3&);
    Coord3                      getBallPosition() const;

    void			setPaddlePosition(const Coord3&);
    Coord3			getPaddlePosition() const;

    void                        setDisplayTransformation(
	    				const visBase::Transformation*);
    const visBase::Transformation*    getDisplayTransformation() const;

protected:
    				
				~BouncyDisplay();

    void			zScaleCB(CallBacker*);
    void			eventCB(CallBacker*);
    void			keyPressCB(CallBacker*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			movePaddleLeft();
    void			movePaddleRight();
    void			movePaddleUp();
    void			movePaddleDown();
    
    visBase::BeachBall*		bb_;
    SoRotation*			rotation_;
    visBase::Cube*		paddle_;
    int				sceneid_;
    visBase::EventCatcher*	eventcatcher_;
    bool			ispaused_;
    bool			isstopped_;

};
};

#endif
