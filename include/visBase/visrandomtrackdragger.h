#ifndef visrandomtrackdragger_h
#define visrandomtrackdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2006
 RCS:           $Id$
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


mExpClass(visBase) RandomTrackDragger : public VisualObjectImpl
{
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

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setLimits(const Coord3& start,
	    				  const Coord3& stop,
					  const Coord3& step); 

    CNotifier<RandomTrackDragger,int> motion;
    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

protected:
    				~RandomTrackDragger();
    void			startCB(CallBacker*);
    void			moveCB(CallBacker*);
    void			finishCB(CallBacker*);
    void			triggerRightClick(const EventInfo* eventinfo);
    void			followActiveDragger(int activeidx);
    void			updatePanels();

    ObjectSet<Dragger>		draggers_;
				/* Contains four coupled draggers per knot:
				idx%4==0: 2D horizontal dragger at start depth
				idx%4==1: 1D vertical dragger at start depth
				idx%4==2: 2D horizontal dragger at stop depth
				idx%4==3: 1D vertical dragger at stop depth */
    osg::Switch*		panels_;
    BoolTypeSet			showadjacents_;
    bool			showallpanels_;

    Interval<float>		zrange_;
    StepInterval<float>		limits_[3];

    Notifier<RandomTrackDragger> rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    const mVisTrans*		displaytrans_;

    static const char*		sKeyDraggerScale() { return "subDraggerScale"; }
};

} // namespace visBase

#endif

