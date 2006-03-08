#ifndef vispicksetdisplay_h
#define vispicksetdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.47 2006-03-08 09:40:37 cvsnanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "bufstringset.h"

class Color;
class IOPar;
class Sphere;
class PickSet;

namespace visBase
{
    class DataObjectGroup;
    class EventCatcher;
    class Transformation;
};


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying picks.

  Picks are positions in 3d (x,y,z) selected by the user by mouseclicks,
  or generated at random. Picks have a constant size in pixels, and can be
  visualized by a number of shapes.
*/

class PickSetDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static PickSetDisplay*	create()
				mCreateDataObj(PickSetDisplay);

    bool			isPicking() const;

    void			copyFromPickSet(const PickSet&);
    void			copyToPickSet(PickSet&) const;

    void			addPick(const Coord3&,const Sphere&);
    void			addPick(const Coord3&);
    BufferString		getManipulationString() const;
    bool			hasChanged() const	{ return haschanged; }
    void			setChanged(bool yn)	{ haschanged = yn; }

    int				nrPicks() const;
    Coord3			getPick(int idx) const;
    Coord3			getDirection(int idx) const;
    void			removePick(const Coord3&);
    void			removeAll();

    float			getInitScreenSize() const { return initsz; }
    float			getPickScreenSize() const { return picksz; }
    void			setScreenSize(float);

    void			setColor(Color);
    Color			getColor() const;
    bool			hasColor() const	{ return true; }

    void			setType(int);
    int				getType() const;
    BufferStringSet&		getMarkerTypes()	{ return markertypes; }

    void			showAll(bool yn);
    bool			allShown() const	{ return showall; }
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&, int );

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    NotifierAccess*		getManipulationNotifier() { return &changed; }

    void			setDisplayTransformation(
	    					visBase::Transformation*);
    visBase::Transformation*	getDisplayTransformation();

    void			setSceneEventCatcher(visBase::EventCatcher*);

protected:
    virtual			~PickSetDisplay();

    void			pickCB(CallBacker* cb=0);

    BufferStringSet		markertypes;
    void			fillMarkerSet();

    int				picktype;
    float			picksz;
    const float			initsz;

    bool			showall;
    bool			haschanged;

    int				mousepressid;
    Coord3		        mousepressposition;

    visBase::DataObjectGroup*	group;
    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;

    Notifier<PickSetDisplay>	changed;

    static const char*		nopickstr;
    static const char*		pickprefixstr;
    static const char*		showallstr;
    static const char*		shapestr;
    static const char*		sizestr;
};

};


#endif
