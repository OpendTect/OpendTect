#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "objectset.h"
#include "position.h"
#include "ranges.h"


namespace osg { class Switch; }

namespace visBase
{

/*! \brief Class for panel strip draggers
*/

class Transformation;
class Dragger;
class MarkerSet;
class PlaneDragCBHandler;
class PolyLine;


mExpClass(visBase) RandomTrackDragger : public VisualObjectImpl
{
    friend class PlaneDragCBHandler;

public:
    static RandomTrackDragger*	create()
				mCreateDataObj(RandomTrackDragger);

    int				nrKnots() const;
    Coord			getKnot(int) const;
    void			setKnot(int,const Coord&);
    void			insertKnot(int,const Coord&);
    void			removeKnot(int);

    void			showAdjacentPanels(int knotidx,bool yn);
    bool			areAdjacentPanelsShown(int knotidx) const;
    void			showAllPanels(bool yn);
    bool			areAllPanelsShown() const;

    Interval<float>		getDepthRange() const;
    void			setDepthRange(const Interval<float>&);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setLimits(const Coord3& start,
					  const Coord3& stop,
					  const Coord3& step);

    void			updateZLimit( const Interval<float>& zborder );

    void			showPlaneDraggers(bool yn,int minsizeinsteps=0);

    void			setTransDragKeys(bool trans1d,int keys,
						 int groupidx=0);
    int				getTransDragKeys(bool trans1d,
						 int groupidx=0) const;

    CNotifier<RandomTrackDragger,int> motion;	//!<knotidx>=0, panelidx<0
    Notifier<RandomTrackDragger> movefinished;

    NotifierAccess*		rightClicked() override
				{ return &rightclicknotifier_; }
    const TypeSet<VisID>*	rightClickedPath() const override;
    const EventInfo*		rightClickedEventInfo() const;

protected:
				~RandomTrackDragger();

    void			startCB(CallBacker*);
    void			moveCB(CallBacker*);
    void			finishCB(CallBacker*);

    void			triggerRightClick(
					const EventInfo* eventinfo) override;

    void			doSetKnot(int,const Coord&);

    void			followActiveDragger(int activeidx);
    void			updatePanels();
    void			postponePanelUpdate(bool);
    void			turnPanelOn(int planeidx,bool yn);
    void			setPanelsPolygonOffset(bool);

    void			removePlaneDraggerCBHandler(int idx);
    void			addPlaneDraggerCBHandler();

    void			updatePlaneDraggers();
    void			updateKnotColor(int idx,bool horoverlap);
    bool			canShowPlaneDragger(int planeidx,
						    bool& horoverlap) const;
    void			snapToLimits(Coord3& pos) const;
    Coord3			getPlaneBoundingBoxInSteps(int planeidx) const;

    bool			doesKnotStickToBorder(int knotidx) const;
    unsigned char		getOnBorderFlags(int knotidx) const;

    void			showRotationAxis(bool yn,int planeidx=0,
					    Coord normpickedpos=Coord::udf());

    int				getDragControlIdx(bool trans1d,int groupidx,
						  bool docreate);
    int				getDragControlIdx(bool trans1d,
						  int groupidx) const;

    ObjectSet<Dragger>		draggers_;
				/* Contains four coupled draggers per knot:
				idx%4==0: 2D horizontal dragger at start depth
				idx%4==1: 1D vertical dragger at start depth
				idx%4==2: 2D horizontal dragger at stop depth
				idx%4==3: 1D vertical dragger at stop depth */
    ObjectSet<MarkerSet>	draggermarkers_;

    ObjectSet<PlaneDragCBHandler>	planedraghandlers_;

    osg::Switch*		panels_;
    osg::Switch*		planedraggers_;
    osg::Switch*		rotationaxis_;

    BoolTypeSet			showadjacents_;
    bool			showallpanels_;

    bool			showplanedraggers_;
    int				planedraggerminsizeinsteps_;

    bool			postponepanelupdate_;

    Interval<float>		zrange_;
    StepInterval<float>		limits_[3];

    Geom::Rectangle<double>	horborder_;
    Interval<float>		zborder_;

    Notifier<RandomTrackDragger> rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    const mVisTrans*		displaytrans_;

    struct DragControl
    {
				DragControl(bool trans1d,int groupidx);
				~DragControl()				{}

	bool			trans1d_;
	int			groupidx_;
	int			mousebutmask_;
	int			modkeymask_;
    };

    ObjectSet<DragControl>	dragcontrols_;

    static const char*		sKeyDraggerScale() { return "subDraggerScale"; }

public:
    int				getKnotIdx(const TypeSet<VisID>& pckpath) const;
    void			handleEvents(bool yn);
    bool			isHandlingEvents() const;
};

} // namespace visBase
