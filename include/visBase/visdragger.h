#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id: visdragger.h,v 1.3 2003-12-04 15:59:45 nanne Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class Color;

class SoDragger;
class SoGroup;


/*! \brief Class for simple draggers
*/

namespace visBase
{

class Transformation;


class Dragger : public SceneObject
{
public:
    static Dragger*		create()
    				mCreateDataObj(Dragger);

    enum Type			{ Translate, DragPoint };
    void			setDraggerType(Type);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    void			setSize(const Coord3&);
    Coord3			getSize() const;

    void			setRotation(const Coord3&,float);
    void			setColor(const Color&);

    void			turnOn(bool);
    bool			isOn() const;

    Notifier<Dragger>		started;
    Notifier<Dragger>		motion;
    Notifier<Dragger>		finished;

    SoNode*			getData();

protected:
    				~Dragger();

    static void			startCB(void*,SoDragger*);
    static void			motionCB(void*,SoDragger*);
    static void			finishCB(void*,SoDragger*);

    SoSwitch*			onoff;
    SoGroup*			group;
    Transformation*		transform;
    SoDragger*			dragger;
};

} // namespace visBase

#endif
