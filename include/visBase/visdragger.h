#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id: visdragger.h,v 1.6 2004-05-13 09:12:40 kristofer Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class Color;

class SoDragger;
class SoSeparator;


namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;


class Dragger : public DataObject
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

    void			setTransformation( Transformation* );
    Transformation*		getTransformation();

    void			setOwnShape(visBase::DataObject*);
    bool			selectable() const;

    Notifier<Dragger>		started;
    Notifier<Dragger>		motion;
    Notifier<Dragger>		finished;
    NotifierAccess*		rightClicked() { return &rightclicknotifier; }

    SoNode*			getInventorNode();

protected:
    				~Dragger();
    void			triggerRightClick()
    				{ rightclicknotifier.trigger(); }

    static void			startCB(void*,SoDragger*);
    static void			motionCB(void*,SoDragger*);
    static void			finishCB(void*,SoDragger*);
    
    Notifier<Dragger>		rightclicknotifier;

    SoSwitch*			onoff;
    SoSeparator*		separator;
    Transformation*		positiontransform;
    SoDragger*			dragger;
    Transformation*		displaytrans;

    visBase::DataObject*	ownshape;
};

} // namespace visBase

#endif
