#ifndef vislocationdisplay_h
#define vislocationdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		June 2006
 RCS:		$Id: vislocationdisplay.h,v 1.2 2006-06-19 21:34:22 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"

class Sphere;
namespace Pick { class Set; class Location; }

namespace visBase
{
    class DataObjectGroup;
    class EventCatcher;
    class Transformation;
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
    virtual NotifierAccess*	getManipulationNotifier() { return &visnotif_; }
    virtual void		setDisplayTransformation(
	    					visBase::Transformation*);
    virtual visBase::Transformation* getDisplayTransformation();
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:
					LocationDisplay();
    virtual visBase::VisualObject*	createLocation() const		    = 0;
    virtual void			setPosition(int loc,const Coord3& pos,
	    					    const Sphere& dir )	    = 0;
    virtual Coord3			getPosition(int loc) const 	    = 0;
    virtual ::Sphere			getDirection(int loc) const 	    = 0;

    Pick::Set*			set_;
    Notifier<LocationDisplay>	visnotif_;

    virtual			~LocationDisplay();

    Pick::Location&		addPick(const Coord3&,const Sphere&,bool);
    void			addDisplayPick(const Pick::Location&);

    void			pickCB(CallBacker* cb=0);
    void			locChg(CallBacker* cb=0);
    void			setChg(CallBacker* cb=0);
    virtual void		dispChg(CallBacker* cb=0);

    bool			showall_;
    int				mousepressid_;
    Coord3		        mousepressposition_;

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
