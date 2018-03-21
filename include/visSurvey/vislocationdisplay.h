#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		June 2006
________________________________________________________________________


-*/

#include "vissurveycommon.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "pickset.h"

class Sphere;
template <class T> class Selector;


namespace visSurvey
{

class Sower;

/*!\brief Used for displaying picksets of varying types.
  The class is not intended for standalone usage, but is a common ground for
  picksets and other classes, where the inheriting classes know about display
  shapes ++.
*/

mExpClass(visSurvey) LocationDisplay : public visBase::VisualObjectImpl
				     , public visSurvey::SurveyObject
{
    friend class Sower;

public:

    typedef Pick::Set::LocID	LocID;
    typedef visBase::EventInfo	EventInfo;

    virtual void		setSet(Pick::Set*); // once!

    Pick::Set*			getSet()		{ return set_; }
    const Pick::Set*		getSet() const		{ return set_; }

    DBKey			getDBKey() const;

    const uiString&		errMsg() const { return errmsg_; }

    void			fullRedraw(CallBacker* =0);
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }
    virtual void		setOnlyAtSectionsDisplay(bool);
    virtual bool		displayedOnlyAtSections() const;
    void			allowDoubleClick(bool yn);

    virtual BufferString	getManipulationString() const;
    void			getObjectInfo(BufferString&) const;
    void			getMousePosInfo(const EventInfo&,IOPar&) const;
    virtual void		getMousePosInfo(const EventInfo&,Coord3&,
					    BufferString&,BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual Color		getColor() const;
    virtual void		setColor(Color);

    virtual bool		allowsPicks() const	{ return true; }
    virtual bool		isPicking() const;
    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    virtual NotifierAccess*	getManipulationNotifier() { return &manip_; }
    virtual void		setDisplayTransformation(const mVisTrans*);
    virtual const mVisTrans*	getDisplayTransformation() const;
    void			setRightHandSystem(bool yn);
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void		turnOnSelectionMode(bool)		{}
    int				getPickID(visBase::DataObject*) const;

    const SurveyObject*		getPickedSurveyObject() const;

    bool			canRemoveSelection() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
						TaskRunner*);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
				LocationDisplay();
    virtual void		setPosition(int,const Pick::Location&)	{}
    virtual void		setPosition(int,const Pick::Location&,
					    bool add)			{}
    virtual void		removePosition(int)			{}
    virtual void		removeAll();

    virtual bool		hasDirection() const { return false; }
    virtual bool		hasText() const { return false; }
    virtual int			clickedMarkerIndex(const EventInfo&) const;
    virtual bool		isMarkerClick(const EventInfo&) const;
    virtual void		updateDragger() {}
    virtual void		setDraggerNormal(const Coord3&) {}
    virtual bool		draggerNormal() const { return true; }
    virtual bool		removeSelections(TaskRunner*);

    virtual int			isDirMarkerClick(const TypeSet<int>&) const;
    void			triggerDeSel();

    virtual			~LocationDisplay();

    LocID			addPick(const Coord3&,const Sphere&);

    bool			getPickSurface(const EventInfo&,Coord3&,
						Coord3&) const;
    Coord3			display2World(const Coord3&) const;
    Coord3			world2Display(const Coord3&) const;
    bool			transformPos(Pick::Location&) const;
    const Coord3		getActivePlaneNormal(const EventInfo&) const;
    void			handleDraggingEvent(const EventInfo&);
    void			handleDirectionEvent(const EventInfo&);
    int				getEventID(const EventInfo&);
    void			handleMouseDown(const EventInfo&,int,bool);
    void			handleMouseUp(const EventInfo&,int);

    void			pickCB(CallBacker*);
    void			setChgCB(CallBacker*);

    virtual void		locChg(const Monitorable::ChangeData&);
    virtual void		dispChg();

    RefMan<Pick::Set>		set_;
    Notifier<LocationDisplay>	manip_;

    LocID			directionlocationid_;
    LocID			movinglocationid_;

    bool			allowdoubleclick_;
    TypeSet<int>		invalidpicks_;
    bool			showall_;
    int				mousepressid_;
    int				pickedsurvobjid_;
    int				voiidx_;
    bool			ctrldown_;
    ObjectSet< Selector<Coord3> > selectors_;

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		transformation_;
    ZAxisTransform*		datatransform_;

    static const char*		sKeyID();
    static const char*		sKeyMgrName();
    static const char*		sKeyShowAll();
    static const char*		sKeyMarkerType();
    static const char*		sKeyMarkerSize();

    Sower*			sower_;
    bool			selectionmodel_;

};

} // namespace visSurvey
