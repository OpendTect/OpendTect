#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visboxdragger.h,v 1.15 2011/04/28 07:00:12 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTabBoxDragger;
class SoMaterial;
class SoDragger;
class SoSwitch;

template <class T> class Interval;

namespace visBase
{

mClass BoxDragger : public DataObject
{
public:
    static BoxDragger*		create()
				mCreateDataObj(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;
    
    void			setWidth(const Coord3&);
    Coord3			width() const;

    void			setBoxTransparency(float);
    				//!<Between 0 and 1

    void			setSpaceLimits( const Interval<float>&,
	    					const Interval<float>&,
						const Interval<float>&);

    void			turnOn(bool yn);
    bool			isOn() const;


    bool			selectable() const { return selectable_; }
    void			setSelectable(bool yn) { selectable_ = yn; }

    Notifier<BoxDragger>	started;
    Notifier<BoxDragger>	motion;
    Notifier<BoxDragger>	changed;
    Notifier<BoxDragger>	finished;

protected:
				~BoxDragger();
    void			setOwnShapeHints();

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSwitch*			onoff_;
    SoTabBoxDragger*		boxdragger_;
    SoMaterial*			boxmaterial_;

    Interval<float>*		xinterval_;
    Interval<float>*		yinterval_;
    Interval<float>*		zinterval_;

    Coord3			prevwidth_;
    Coord3			prevcenter_;
    bool			selectable_;

    virtual SoNode*		gtInvntrNode();

};

};
	
#endif
