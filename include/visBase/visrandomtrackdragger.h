#ifndef visrandomtrackdragger_h
#define visrandomtrackdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2006
 RCS:           $Id: visrandomtrackdragger.h,v 1.6 2012-08-03 13:01:26 cvskris Exp $
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"
#include "ranges.h"

class Color;

class SoRandomTrackLineDragger;


namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;


mClass(visBase) RandomTrackDragger : public VisualObjectImpl
{
public:
    static RandomTrackDragger*	create()
    				mCreateDataObj(RandomTrackDragger);


    void			setSize(const Coord3&);
    Coord3			getSize() const;

    int				nrKnots() const;
    Coord			getKnot(int) const;
    void			setKnot(int,const Coord&);
    void			insertKnot(int,const Coord&);
    void			removeKnot(int);

    Interval<float>		getDepthRange() const;
    void			setDepthRange(const Interval<float>&);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setLimits(const Coord3& start,
	    				  const Coord3& stop,
					  const Coord3& step); 

    void			showFeedback(bool yn);

    CNotifier<RandomTrackDragger,int> motion;
    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

protected:
    				~RandomTrackDragger();
    void			triggerRightClick(const EventInfo* eventinfo);

    static void			motionCB(void*,SoRandomTrackLineDragger*);
    
    Notifier<RandomTrackDragger> rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    SoRandomTrackLineDragger*	dragger_;
    const mVisTrans*		displaytrans_;

    static const char*		sKeyDraggerScale() { return "subDraggerScale"; }
};

} // namespace visBase

#endif

