#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		November 2008
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"
#include "visemsticksetdisplay.h"

#include "emposid.h"


namespace visBase
{
    class DrawStyle;
    class Lines;
    class MarkerSet;
    class Transformation;
}

namespace EM { class FaultStickSet; class FaultStickSetGeometry; }
namespace Geometry { class FaultStickSet; class IndexedPrimitiveSet; }
namespace MPE { class FaultStickSetEditor; }

namespace visSurvey
{

class MPEEditor;
class Seis2DDisplay;

/*!\brief Display class for FaultStickSets
*/

mExpClass(visSurvey) FaultStickSetDisplay : public visBase::VisualObjectImpl
					  , public SurveyObject
					  , public StickSetDisplay
{ mODTextTranslationClass(FaultStickSetDisplay);
public:
				FaultStickSetDisplay();

				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,
				    FaultStickSetDisplay,
				    "FaultStickSetDisplay",
				    toUiString(sFactoryKeyword()))


    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const		{ return true; }
    OD::Color			getColor() const;
    void			setColor(OD::Color);
    bool			allowMaterialEdit() const	{ return true; }

    const OD::LineStyle*	lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);
    const MarkerStyle3D*	markerStyle() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    bool			hasSpecificMarkerColor() const	{ return true; }

    void			hideAllKnots(bool yn);
    bool			areAllKnotsHidden() const;

    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    virtual void		enableEditor(bool yn);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*);

    const char*			errMsg() const { return errmsg_.str(); }

    void			updateSticks(bool activeonly=false);
    void			updateEditPids();
    void			updateKnotMarkers();
    void			updateAll();
    const visBase::Lines*	getStickSetLines() const { return sticks_; }

    Notifier<FaultStickSetDisplay> colorchange;
    Notifier<FaultStickSetDisplay> displaymodechange;

    bool			removeSelections(TaskRunner*);
    bool			canRemoveSelection() const	{ return true; }

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			displayedOnlyAtSections() const;

    void			setStickSelectMode(bool yn);
    void			turnOnSelectionMode(bool);
    bool			isInStickSelectMode() const;

    bool			allowsPicks() const		{ return true; }

    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const;

    virtual void		setPixelDensity(float dpi);
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    const MarkerStyle3D*	getPreferedMarkerStyle() const;
    void			setPreferedMarkerStyle(const MarkerStyle3D&);

protected:
    virtual			~FaultStickSetDisplay();

    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					int whichobj);

    void			setActiveStick(const EM::PosID&);

    static const char*		sKeyEarthModelID();
    static const char*		sKeyDisplayOnlyAtSections();


    bool			isPicking() const;
    void			mouseCB(CallBacker*);
    void			draggingStartedCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);
    bool			isSelectableMarkerInPolySel(
					const Coord3& markerworldpos) const;

    Coord3			disp2world(const Coord3& displaypos) const;

    void			displayOnlyAtSectionsUpdate();
    bool			coincidesWith2DLine(
					Geometry::FaultStickSet&,
					int sticknr,Pos::GeomID);
    bool			coincidesWithPlane(
					Geometry::FaultStickSet&,
					int sticknr,
					TypeSet<Coord3>& intersectpoints);
    EM::FaultStickSet*		emFaultStickSet();
    void			sowingFinishedCB(CallBacker*);
    void			updateManipulator();


    MPE::FaultStickSetEditor*	fsseditor_;
    visSurvey::MPEEditor*	viseditor_;

    Coord3			mousepos_;

    int				activesticknr_;

    TypeSet<EM::PosID>		editpids_;

    visBase::Lines*		sticks_;
    visBase::Lines*		activestick_;
    visBase::DrawStyle*		stickdrawstyle_;
    visBase::DrawStyle*		activestickdrawstyle_;

    bool			displayonlyatsections_;
    bool			makenewstick_;
    EM::PosID			activestickid_;

public:
    void			setDisplayOnlyAtSections(bool yn)
				{ setOnlyAtSectionsDisplay(yn); }
};

} // namespace VisSurvey

