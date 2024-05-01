#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "position.h"
#include "visobject.h"
#include "visosg.h"
#include "vistransform.h"

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
				DraggerBase();
				~DraggerBase();

    friend class DraggerCallbackHandler;

    virtual  void		notifyStart() = 0;
    virtual  void		notifyStop() = 0;
    virtual  void		notifyMove() = 0;

    ConstRefMan<mVisTrans>	displaytrans_;
    osgManipulator::Dragger*	osgdragger_ = nullptr;
    osg::Group*			osgroot_;

    void			initDragger(osgManipulator::Dragger*);

    Interval<float>		spaceranges_[3];

private:
    DraggerCallbackHandler*	cbhandler_ = nullptr;

public:
    void			handleEvents(bool yn);
    bool			isHandlingEvents() const;
};


mExpClass(visBase) Dragger : public DraggerBase
{
public:
    static RefMan<Dragger>	create();
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
    const TypeSet<VisID>*	rightClickedPath() const override;
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
    const EventInfo*		rightclickeventinfo_ = nullptr;

    RefMan<DataObject>		inactiveshape_;
    bool			ismarkershape_	= true;
    bool			is2dtranslate_;
    Coord3			markerpos_;
    float			draggersizescale_	= 100.f;
    float			defaultdraggergeomsize_ = 0.025f;

    Coord3			rotation_;
    float			rotangle_	= 0.f;
    OD::Color			arrowcolor_;

public:
    bool			isMoving() const;
};

} // namespace visBase
