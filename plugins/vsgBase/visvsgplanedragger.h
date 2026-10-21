#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgobject.h"
#include "visvsgtransform.h"

#include "color.h"
#include "position.h"
#include "ranges.h"

namespace vsgGeo { class TabPlaneDragger; }
namespace dGBVis { class EventCatcher; class EventInfo; }

namespace dGBVis
{

/*!\brief Counterpart of visBase::DepthTabPlaneDragger.

  center() and size() are object coordinates (inline/crossline/Z when the
  scene adds the dragger with addInlCrlZObject). The display transformation
  is applied when the vertices are built. It is not baked into the value
  center() returns.
*/

mExpClass(vsgBase) DepthTabPlaneDragger : public VisualObjectImpl
{
public:
    static RefMan<DepthTabPlaneDragger> create();
				mCreateDGBVisObj(DepthTabPlaneDragger);

    void			removeScaleTabs();

    void			setCenter(const Coord3&,bool alldims=true);
    Coord3			center() const		{ return center_; }

    void			setSize(const Coord3&,bool alldims=true);
    Coord3			size() const		{ return size_; }

    void			setDim(int dim);
    int				getDim() const		{ return dim_; }

    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);
    void			getSpaceLimits(Interval<float>& x,
					       Interval<float>& y,
					       Interval<float>& z) const;

    void			setColor(const OD::Color&);

    void			setWidthLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);
    void			getWidthLimits(Interval<float>& x,
					       Interval<float>& y,
					       Interval<float>& z) const;

    void			setDisplayTransformation(
					const mDGBVisTrans*) override;
    const mDGBVisTrans*		getDisplayTransformation() const override
				{ return displaytrans_.get().ptr(); }

    void			showDraggerBorder(bool yn=true);
    bool			isDraggerBorderShown() const
				{ return bordershown_; }
    void			showPlane(bool yn=true);
    bool			isPlaneShown() const	{ return planeshown_; }

    void			setTransDragKeys(bool depth,int keys);
    int				getTransDragKeys(bool depth) const;
    void			setDragCtrlSpacing(const StepInterval<float>&,
						   const StepInterval<float>&,
						   const StepInterval<float>&);

    void			setEventCatcher(EventCatcher*);
    void			setNodeTransforms(
						const Transformation* zstretch,
						const Transformation* rotation);

    void			beginDrag(const Coord3& point,bool alongnormal);
    void			dragTo(const Coord3& point);
    void			endDrag();

    Notifier<DepthTabPlaneDragger>	started;
    Notifier<DepthTabPlaneDragger>	motion;
    Notifier<DepthTabPlaneDragger>	changed;
    Notifier<DepthTabPlaneDragger>	finished;

private:
				~DepthTabPlaneDragger();

    void			eventCB(CallBacker*);
    void			pushGeometry();
    void			clamp();
    bool			sceneToObject(const Coord3&,Coord3&) const;
    bool			objectToScene(const Coord3&,Coord3&) const;
    bool			objectPoint(const EventInfo&,
					    Coord3&) const;
    bool			rayInObject(const EventInfo&,
					    Coord3& start,Coord3& end) const;
    void			initNormalProjector(const EventInfo&);
    bool			normalDragPoint(const EventInfo&,
						Coord3&) const;

    vsg::ref_ptr<vsgGeo::TabPlaneDragger>	plane_;
    WeakPtr<mDGBVisTrans>	displaytrans_;
    WeakPtr<Transformation>	zstretch_;
    WeakPtr<Transformation>	rotation_;
				/*!<Weak, because the transformations hold this
				    dragger as a child. */
    EventCatcher*		events_		= nullptr;

    int				dim_ = 2;
    Coord3			center_;
    Coord3			size_ = Coord3(1.,1.,1.);
    Coord3			centers_[3];
    Coord3			sizes_[3];
    Interval<float>		space_[3];
    Interval<float>		width_[3];
    StepInterval<float>		spacing_[3];
    int				depthkeys_ = 0;
    int				planekeys_ = 0;
    bool			bordershown_ = true;
    bool			planeshown_ = true;
    bool			dragging_ = false;
    bool			dragmoved_ = false;
    bool			drag1d_ = false;
    bool			dragplaneok_ = false;
    Coord3			dragplanenml_;
    double			dragaxisref_ = 0.;
    Coord			dragmouse_;
    double			dragpixelsperunit_ = 1.;
    double			dragfrontsign_ = 1.;
    Coord3			dragstart_;
    Coord3			dragcenter_;
    Coord3			dragsize_;
};

} // namespace dGBVis
