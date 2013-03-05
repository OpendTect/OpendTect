#ifndef visfaultsticksetdisplay_h
#define visfaultsticksetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		November 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"

#include "emposid.h"


namespace visBase 
{ 
    class Transformation;
    class IndexedPolyLine3D;
    class PickStyle;
}

namespace Geometry { class FaultStickSet; } 
namespace EM { class FaultStickSet; }
namespace MPE { class FaultStickSetEditor; }

namespace visSurvey
{
class MPEEditor;
class Seis2DDisplay;

/*!\brief Display class for FaultStickSets
*/

mExpClass(visSurvey) FaultStickSetDisplay : public visBase::VisualObjectImpl,
			     public SurveyObject
{
public:
    static FaultStickSetDisplay* create()
				mCreateDataObj(FaultStickSetDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    const char*			errMsg() const { return errmsg_.str(); }

    void			updateSticks(bool activeonly=false);
    void			updateEditPids();
    void			updateKnotMarkers();
    void			updateAll();

    Notifier<FaultStickSetDisplay> colorchange;
    Notifier<FaultStickSetDisplay> displaymodechange;

    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);
    bool			canRemoveSelection() const	{ return true; }

    void			setDisplayOnlyAtSections(bool yn);
    bool			displayedOnlyAtSections() const;

    void			setStickSelectMode(bool yn);
    bool			isInStickSelectMode() const;

    bool			allowsPicks() const		{ return true; }

    void			getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:

    virtual			~FaultStickSetDisplay();

    void   			otherObjectsMoved(
	    				const ObjectSet<const SurveyObject>&,
					int whichobj);

    void			setActiveStick(const EM::PosID&);

    void			mouseCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);
    bool			isSelectableMarkerInPolySel(
					const Coord3& markerworldpos) const;

    Coord3			disp2world(const Coord3& displaypos) const;

    void			displayOnlyAtSectionsUpdate();
    bool			coincidesWith2DLine(
					const Geometry::FaultStickSet&,
					int sticknr,const MultiID& lineset,
					const char* linenm) const;
    bool			coincidesWithPlane(
					const Geometry::FaultStickSet&,
					int sticknr,
					TypeSet<Coord3>& intersectpoints) const;

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		displaytransform_;

    EM::FaultStickSet*		emfss_;
    MPE::FaultStickSetEditor*	fsseditor_;
    visSurvey::MPEEditor*	viseditor_;

    Coord3			mousepos_;
    bool			showmanipulator_;

    int				activesticknr_;

    TypeSet<EM::PosID>		editpids_;

    visBase::IndexedPolyLine3D* sticks_;
    visBase::IndexedPolyLine3D* activestick_;

    visBase::PickStyle*		stickspickstyle_;
    visBase::PickStyle*		activestickpickstyle_;

    bool			displayonlyatsections_;
    bool			stickselectmode_;

    bool			ctrldown_;

    ObjectSet<visBase::DataObjectGroup>	knotmarkers_;

    struct StickIntersectPoint
    {
	Coord3			pos_;
	int 			sid_;
	int			sticknr_;
    };

    ObjectSet<StickIntersectPoint> stickintersectpoints_;

    static const char*		sKeyEarthModelID();
    static const char*		sKeyDisplayOnlyAtSections();
};

} // namespace VisSurvey


#endif

