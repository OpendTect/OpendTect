#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/

#include "visobject.h"
#include "visosg.h"
#include "position.h"

namespace osgManipulator { class Dragger; }
namespace osg
{
    class MatrixTransform;
    class Node;
}

namespace visBase
{

/*! \brief Class for simple dragger
*/

class DraggerCallbackHandler;
class Transformation;

mExpClass(visBase) DraggerBase : public DataObject
{
public:

    Notifier<DraggerBase>	started;
    Notifier<DraggerBase>	motion;
    Notifier<DraggerBase>	finished;
    Notifier<DraggerBase>	changed;

    virtual void		setDisplayTransformation( const mVisTrans* );
    const mVisTrans*		getDisplayTransformation() const;

    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);

    void			handleEvents(bool yn);
    bool			isHandlingEvents() const;

protected:
    friend			class DraggerCallbackHandler;
				DraggerBase();
				~DraggerBase();

    virtual  void		notifyStart() = 0;
    virtual  void		notifyStop() = 0;
    virtual  void		notifyMove() = 0;

    const mVisTrans*		displaytrans_;
    osgManipulator::Dragger*	osgdragger_;
    osg::Group*			osgroot_;

    void			initDragger(osgManipulator::Dragger*);

    Interval<float>		spaceranges_[3];

private:
    DraggerCallbackHandler*	cbhandler_;


};


mExpClass(visBase) Dragger : public DraggerBase
{
public:
    static Dragger*		create()
				mCreateDataObj(Dragger);

    enum Type			{ Translate1D, Translate2D, Translate3D,
				  Scale3D };
    void			setDraggerType(Type);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    void			setSize(const float);
    float			getSize() const;

    void			setArrowColor(const Color&);
    const Color&		getArrowColor() const;

    void			setRotation(const Coord3&, const float);
    void			setDefaultRotation();
    bool			defaultRotation() const;

    void			setOwnShape(DataObject*,bool activeshape);
				/*!< Sets a shape on the dragger. */
    bool			selectable() const;

    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;
    void			updateDragger( bool active );
    void			setDisplayTransformation( const mVisTrans* );

    bool			isMoving() const	{ return ismoving_; }

protected:
				~Dragger();
    void			triggerRightClick(const EventInfo* eventinfo);
    virtual  void		notifyStart();
    virtual  void		notifyStop();
    virtual  void		notifyMove();
    osg::MatrixTransform*	createDefaultDraggerGeometry();
    osg::MatrixTransform*	createTranslateDefaultGeometry();
    void			setScaleAndTranslation(bool move=false);


    Notifier<Dragger>		rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    DataObject*			inactiveshape_;
    bool			ismarkershape_;
    bool			is2dtranslate_;
    Coord3			markerpos_;
    float			draggersizescale_;
    float			defaultdraggergeomsize_;

    bool			ismoving_;

    Coord3			rotation_;
    float			rotangle_;
    Color			arrowcolor_;
};

} // namespace visBase
