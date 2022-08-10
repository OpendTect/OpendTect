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


    MultiID			getMultiID() const override;
    bool			isInlCrl() const override { return false; }

    bool			hasColor() const override	{ return true; }
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    bool			allowMaterialEdit() const override
				{ return true; }

    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;
    const MarkerStyle3D*	markerStyle() const override;
    void			setMarkerStyle(const MarkerStyle3D&) override;
    bool			hasSpecificMarkerColor() const override
				{ return true; }

    void			hideAllKnots(bool yn);
    bool			areAllKnotsHidden() const;

    void			showManipulator(bool) override;
    bool			isManipulatorShown() const override;
    void			enableEditor(bool yn) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*) override;

    const char*			errMsg() const override { return errmsg_.str();}

    void			updateSticks(bool activeonly=false);
    void			updateEditPids();
    void			updateKnotMarkers();
    void			updateAll();
    const visBase::Lines*	getStickSetLines() const { return sticks_; }

    Notifier<FaultStickSetDisplay> colorchange;
    Notifier<FaultStickSetDisplay> displaymodechange;

    bool			removeSelections(TaskRunner*) override;
    bool			canRemoveSelection() const override
				{ return true; }

    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    void			setStickSelectMode(bool yn);
    void			turnOnSelectionMode(bool) override;
    bool			isInStickSelectMode() const;

    bool			allowsPicks() const override	{ return true; }

    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const override;

    void			setPixelDensity(float dpi) override;
    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    const MarkerStyle3D*	getPreferedMarkerStyle() const;
    void			setPreferedMarkerStyle(const MarkerStyle3D&);

protected:
    virtual			~FaultStickSetDisplay();

    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					VisID whichobj) override;

    void			setActiveStick(const EM::PosID&);

    static const char*		sKeyEarthModelID();
    static const char*		sKeyDisplayOnlyAtSections();


    bool			isPicking() const override;
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

