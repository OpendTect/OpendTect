#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"
#include "visosg.h"

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

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override;

    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);

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

public:
    void			handleEvents(bool yn);
    bool			isHandlingEvents() const;
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

    void			setArrowColor(const OD::Color&);
    const OD::Color&		getArrowColor() const;

    void			setRotation(const Coord3&, const float);
    void			setDefaultRotation();
    bool			defaultRotation() const;

    void			setOwnShape(DataObject*,bool activeshape);
				/*!< Sets a shape on the dragger. */
    bool			selectable() const override;

    NotifierAccess*		rightClicked() override
				{ return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const override;
    const EventInfo*		rightClickedEventInfo() const;
    void			updateDragger( bool active );
    void			setDisplayTransformation(
						const mVisTrans*) override;

protected:
				~Dragger();
    void			triggerRightClick(
					const EventInfo* eventinfo) override;
    void			notifyStart() override;
    void			notifyStop() override;
    void			notifyMove() override;
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

    Coord3			rotation_;
    float			rotangle_;
    OD::Color			arrowcolor_;

public:
    bool			isMoving() const;
};

} // namespace visBase

