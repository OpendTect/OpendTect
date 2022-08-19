#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "pickset.h"

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

    MultiID			getMultiID() const override
				{ return storedmid_; }

    const char*			errMsg() const override
				{ return errmsg_.str(); }

    void			fullRedraw(CallBacker* =nullptr);
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }
    void			setOnlyAtSectionsDisplay(bool) override;
    bool			displayedOnlyAtSections() const override;
    void			allowDoubleClick( bool yn )
						    { allowdoubleclicks_ = yn; }

    BufferString		getManipulationString() const override;
    void			getObjectInfo(BufferString&) const override;
    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&,BufferString&,
						BufferString&) const override;
    bool			hasColor() const override	{ return true; }
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;

    bool			allowsPicks() const override	{ return true; }
    bool			isPicking() const override;
    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					VisID) override;
    NotifierAccess*		getManipulationNotifier() override
				{ return &manip_; }
    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;
    void			setRightHandSystem(bool yn) override;
    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    void			turnOnSelectionMode(bool) override	{}

    int				getPickIdx(visBase::DataObject*) const;

    const SurveyObject*		getPickedSurveyObject() const;
    SeedPainter*		getPainter()		{ return painter_; }
    bool			isPainting() const override;

    bool			canRemoveSelection() const override
				{ return true; }
    void			removeSelection(const Selector<Coord3>&,
						TaskRunner*);

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

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
    bool			removeSelections(TaskRunner*) override;

    virtual int			isDirMarkerClick(const TypeSet<VisID>&) const;
    void			triggerDeSel() override;

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
    VisID			mousepressid_;
    VisID			pickedsobjid_;
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
