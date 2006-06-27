#ifndef vislocationdisplay_h
#define vislocationdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		June 2006
 RCS:		$Id: vislocationdisplay.h,v 1.4 2006-06-27 21:21:47 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"

class Sphere;
namespace Pick { class Set; class Location; class SetMgr; }

namespace visBase
{
    class DataObjectGroup;
    class EventCatcher;
    class Transformation;
    class PickStyle;
};


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying picksets of varying types.
  The class is not intended for standalone usage, but is a common ground for
  picksets and other classes, where the inheriting classes knows about display
  shapes ++.
*/

class LocationDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    void			setSet(Pick::Set*); // once!
    void			setSetMgr(Pick::SetMgr*);
    				/*!<Only used for notifications. */
    Pick::Set*			getSet()		{ return set_; }

    const MultiID&		getStoredID() const 	{ return storedmid_; }

    void			fullRedraw();
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }

    virtual BufferString	getManipulationString() const;
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						const Coord3&,BufferString&,
						BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual Color		getColor() const;
    virtual bool		isPicking() const;
    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    virtual NotifierAccess*	getManipulationNotifier() { return &manip_; }
    virtual void		setDisplayTransformation(mVisTrans*);
    virtual mVisTrans*		getDisplayTransformation();
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:
					LocationDisplay();
    virtual visBase::VisualObject*	createLocation() const		    = 0;
    virtual void			setPosition(int loc,const Coord3& pos,
	    					    const Sphere& dir )	    = 0;
    virtual bool			hasDirection() const { return false; }

    virtual			~LocationDisplay();

    void			addPick(const Coord3&,const Sphere&,bool);
    void			addDisplayPick(const Pick::Location&);

    bool			getPickSurface(const visBase::EventInfo&,
	    				       Coord3&) const;
    Coord3			display2World(const Coord3&) const;
    Coord3			world2Display(const Coord3&) const;

    void			pickCB(CallBacker* cb);
    void			locChg(CallBacker* cb);
    void			setChg(CallBacker* cb);
    virtual void		dispChg(CallBacker* cb);

    Pick::Set*			set_;
    Pick::SetMgr*		picksetmgr_;
    Notifier<LocationDisplay>	manip_;
    int				waitsfordirectionid_;

    bool			showall_;
    int				mousepressid_;

    visBase::PickStyle*		pickstyle_;
    visBase::DataObjectGroup*	group_;
    visBase::EventCatcher*	eventcatcher_;
    visBase::Transformation*	transformation_;

    MultiID			storedmid_;

    static const char*		sKeyID();
    static const char*		sKeyShowAll();
    static const char*		sKeyMarkerType();
    static const char*		sKeyMarkerSize();
};

};


#endif
