#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		June 2006
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "refcount.h"

class Sphere;
namespace Pick { class Set; class Location; class SetMgr; }
template <class T> class Selector;


namespace visSurvey
{

class Sower;
class SeedPainter;

/*!\brief Used for displaying picksets of varying types.
  The class is not intended for standalone usage, but is a common ground for
  picksets and other classes, where the inheriting classes knows about display
  shapes ++.
*/

mExpClass(visSurvey) LocationDisplay : public visBase::VisualObjectImpl
				     , public visSurvey::SurveyObject
{
    friend class Sower;

public:
    virtual void		setSet(Pick::Set*); // once!
    void			setSetMgr(Pick::SetMgr*);
    				/*!<Only used for notifications. */
    RefMan<Pick::Set>		getSet();
    ConstRefMan<Pick::Set>	getSet() const;

    MultiID			getMultiID() const	{ return storedmid_; }

    const char*			errMsg() const { return errmsg_.str(); }

    void			fullRedraw(CallBacker* =0);
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }
    virtual void		setOnlyAtSectionsDisplay(bool);
    virtual bool		displayedOnlyAtSections() const;
    void			allowDoubleClick( bool yn )
						    { allowdoubleclicks_ = yn; }

    virtual BufferString	getManipulationString() const;
    void			getObjectInfo(BufferString&) const;
    void			getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,BufferString&,
						BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual OD::Color		getColor() const;
    virtual void		setColor(OD::Color);

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

    int				getPickIdx(visBase::DataObject*) const;

    const SurveyObject*		getPickedSurveyObject() const;
    SeedPainter*		getPainter()		{ return painter_; }
    bool			isPainting() const;

    bool			canRemoveSelection() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    Coord3			convertCoords(const Coord3&,bool disptoworld);

protected:
				LocationDisplay();
    virtual void		setPosition(int idx,const Pick::Location&,
					    bool add=false);
    virtual void		removePosition(int);
    virtual void		removeAll();
    virtual void		redrawMultiSets() {}

    virtual bool		hasDirection() const { return false; }
    virtual bool		hasText() const { return false; }
    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;
    virtual bool		isMarkerClick(
					const visBase::EventInfo& evi)const;
    virtual void		updateDragger() {}
    virtual void		setDraggerNormal(const Coord3&) {}
    virtual bool		draggerNormal() const {return true;}
    virtual bool		removeSelections(TaskRunner*);

    virtual int			isDirMarkerClick(const TypeSet<int>&) const;
    void			triggerDeSel();

    virtual			~LocationDisplay();

    bool			addPick(const Coord3&,const Sphere&,bool);
    void			removePick(int,bool setundo=true);

    bool			getPickSurface(const visBase::EventInfo&,
					   Coord3& pos, Coord3& normal) const;
    Coord3			display2World(const Coord3&) const;
    Coord3			world2Display(const Coord3&) const;
    bool			transformPos(Pick::Location&) const;
    const Coord3		getActivePlaneNormal(
					     const visBase::EventInfo&) const;

    void			pickCB(CallBacker* cb);
    virtual void		locChg(CallBacker* cb);
    virtual void		setChg(CallBacker* cb);
    virtual void		dispChg(CallBacker* cb);
    void			bulkLocChg(CallBacker* cb);

    RefMan<Pick::Set>		set_;
    Pick::SetMgr*		picksetmgr_		= nullptr;
    Notifier<LocationDisplay>	manip_;
    int				waitsfordirectionid_	= -1;
    int				waitsforpositionid_	= -1;

    TypeSet<int>		invalidpicks_;

    bool			showall_		= true;
    int				mousepressid_		= -1;
    int				pickedsobjid_		= -1;
				//!< Picked SurveyObject ID
    int				voiidx_			= -1;
    bool			ctrldown_		= false;

    visBase::EventCatcher*	eventcatcher_		= nullptr;
    const mVisTrans*		transformation_		= nullptr;
    ZAxisTransform*		datatransform_		= nullptr;

    MultiID			storedmid_;

    static const char*		sKeyID();
    static const char*		sKeyMgrName();
    static const char*		sKeyShowAll();
    static const char*		sKeyMarkerType();
    static const char*		sKeyMarkerSize();

    Sower*			sower_;
    SeedPainter*		painter_;
    Coord3			undoloccoord_		= Coord3::udf();
    bool			undomove_		= false;
    bool			selectionmodel_		= false;
    bool			allowdoubleclicks_	= true;
};

} // namespace visSurvey
