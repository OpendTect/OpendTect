#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id$
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


mClass Dragger : public DataObject
{
public:
    static Dragger*		create()
    				mCreateDataObj(Dragger);

    enum Type			{ Translate1D, Translate2D, Translate3D,
    				  Scale3D };
    void			setDraggerType(Type);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    void			setSize(const Coord3&);
    Coord3			getSize() const;

    void			setRotation(const Coord3&,float);
    void			setDefaultRotation();

    void			turnOn(bool);
    bool			isOn() const;

    void			setDisplayTransformation( const mVisTrans* );
    const mVisTrans*		getDisplayTransformation() const;

    void			setOwnShape(DataObject*,
	    				    const char* partname );
    				/*!< Sets a shape on the dragger.
				    \note The object will not be reffed,
					  so it's up to the caller to make sure
					  it remains in memory */
    SoNode*			getShape( const char* name );
    bool			selectable() const;

    Notifier<Dragger>		started;
    Notifier<Dragger>		motion;
    Notifier<Dragger>		finished;
    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

protected:
    				~Dragger();
    void			triggerRightClick(const EventInfo* eventinfo);

    static void			startCB(void*,SoDragger*);
    static void			motionCB(void*,SoDragger*);
    static void			finishCB(void*,SoDragger*);
    
    Notifier<Dragger>		rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    SoSwitch*			onoff_;
    SoSeparator*		root_;
    Transformation*		positiontransform_;
    SoDragger*			dragger_;
    const mVisTrans*		displaytrans_;

    virtual SoNode*		gtInvntrNode();

};

} // namespace visBase

#endif
