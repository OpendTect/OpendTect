#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"


namespace osgManipulator { class TabBoxDragger; }
namespace osg { class Switch; }

class SoTabBoxDragger;
class SoMaterial;
class SoDragger;
class SoSwitch;

template <class T> class Interval;

namespace visBase
{

class BoxDraggerCallbackHandler;

mClass(visBase) BoxDragger : public DataObject
{
public:
    friend class BoxDraggerCallbackHandler;

//public:
    static BoxDragger*		create()
				mCreateDataObj(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;
    
    void			setWidth(const Coord3&);
    Coord3			width() const;

    void			setBoxTransparency(float);
    				//!<Between 0 and 1

    void			setSpaceLimits(const Interval<float>&,
					       const Interval<float>&,
					       const Interval<float>&);

    void			setWidthLimits(const Interval<float>& x,
	    				       const Interval<float>& y,
					       const Interval<float>& z );

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

    void			initOsgDragger();

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSwitch*			onoff_;
    SoTabBoxDragger*		boxdragger_;
    SoMaterial*			boxmaterial_;

    osgManipulator::TabBoxDragger*	osgboxdragger_;
    osg::Switch*			osgdraggerroot_;
    BoxDraggerCallbackHandler*		osgcallbackhandler_;

    Interval<float>*		xinterval_;
    Interval<float>*		yinterval_;
    Interval<float>*		zinterval_;

    Interval<float>		widthranges_[3];
    Interval<float>		spaceranges_[3];

    Coord3			prevwidth_;
    Coord3			prevcenter_;
    bool			selectable_;

    virtual SoNode*		gtInvntrNode();
    virtual osg::Node*		gtOsgNode();
};

};
	
#endif

