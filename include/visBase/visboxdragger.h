#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visboxdragger.h,v 1.7 2003-10-01 14:20:38 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTabBoxDragger;
class SoDragger;
class SoSwitch;

template <class T> class Interval;

namespace visBase
{

class BoxDragger : public SceneObject
{
public:
    static BoxDragger*		create()
				mCreateDataObj(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;
    
    void			setWidth(const Coord3&);
    Coord3			width() const;

    void			setSpaceLimits( const Interval<float>&,
	    					const Interval<float>&,
						const Interval<float>&);

    void			turnOn(bool yn);
    bool			isOn() const;

    Notifier<BoxDragger>	started;
    Notifier<BoxDragger>	motion;
    Notifier<BoxDragger>	changed;
    Notifier<BoxDragger>	finished;

    SoNode*			getData();

protected:
				~BoxDragger();

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSwitch*			onoff;
    SoTabBoxDragger*		boxdragger;

    Interval<float>*		xinterval;
    Interval<float>*		yinterval;
    Interval<float>*		zinterval;

    Coord3			prevwidth;
    Coord3			prevcenter;
};

};
	
#endif
