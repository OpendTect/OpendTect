#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "datapointset.h"
#include "explfaultsticksurface.h"
#include "faulteditor.h"
#include "ranges.h"
#include "visdrawstyle.h"
#include "visgeomindexedshape.h"
#include "visemsticksetdisplay.h"
#include "vismpeeditor.h"
#include "vismultiattribsurvobj.h"
#include "vispolyline.h"
#include "zaxistransform.h"

namespace visBase
{
    class Transformation;
}

namespace EM { class Fault3D; }
namespace Geometry
{
    class ExplPlaneIntersection;
    class FaultStickSurface;
}

template <class T > class Array2D;

namespace visSurvey
{
class HorizonDisplay;

/*!\brief


*/

mExpClass(visSurvey) FaultDisplay : public MultiTextureSurveyObject
				  , public StickSetDisplay
{ mODTextTranslationClass(FaultDisplay);
public:
				FaultDisplay();

				mDefaultFactoryInstantiation(
				SurveyObject, FaultDisplay,
				"FaultDisplay",
				::toUiString(sFactoryKeyword()) )

    MultiID			getMultiID() const override;
    bool			isInlCrl() const override { return false; }

    int				nrResolutions() const override;
    void			setResolution(int,TaskRunner*) override;

    SurveyObject::AttribFormat	getAttributeFormat(int) const override
				{ return SurveyObject::RandomPos; }
    bool			getRandomPos(DataPointSet&,
					     TaskRunner*) const override;
    bool			getRandomPosCache(int,
						DataPointSet&) const override;
    bool			setRandomPosData(int,const DataPointSet*,
						 TaskRunner*) override;

    bool			hasColor() const override	{ return true; }
    bool			usesColor() const override;
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    bool			allowMaterialEdit() const override
				{ return true; }

    void			useTexture(bool yn,bool trigger) override;
    bool			canShowTexture() const override;

    void			setDepthAsAttrib(int);
    void			enableAttrib(int attrib,bool yn) override;
    bool			hasSingleColorFallback() const override
				{ return true; }

    void			showManipulator(bool) override;
    bool			isManipulatorShown() const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;

    void			triangulateAlg(mFltTriProj);
    mFltTriProj			triangulateAlg() const;

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;
    bool			arePanelsDisplayedInFull() const;

    void			hideAllKnots(bool yn) override;

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*) override;

    bool			removeSelections(TaskRunner*) override;
    bool			canRemoveSelection() const override
				{ return true; }

    void			setOnlyAtSectionsDisplay(bool) override;
    bool			displayedOnlyAtSections() const override;

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;
    bool			canDisplayIntersections() const;

    void			displayHorizonIntersections(bool yn);
    bool			areHorizonIntersectionsDisplayed() const;
    bool			canDisplayHorizonIntersections() const;

    Notifier<FaultDisplay>	colorchange;
    Notifier<FaultDisplay>	displaymodechange;

    void			updateKnotMarkers();

    void			setStickSelectMode(bool yn);
    void			turnOnSelectionMode(bool) override;
    bool			isInStickSelectMode() const;

    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;
    const MarkerStyle3D*	markerStyle() const override;
    void			setMarkerStyle(const MarkerStyle3D&) override;
    bool			hasSpecificMarkerColor() const override
				{ return true; }

    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return MultiTextureSurveyObject
					::getMousePosInfo(ei,iop); }
   void				getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					uiString& info) const override;

    bool			allowsPicks() const override	{ return true; }
    bool			isVerticalPlane() const override {return false;}
    bool			canBDispOn2DViewer() const override
				{ return false; }

    bool			usesDataPacks() const override	{ return true; }
    bool			setPointDataPack(int attrib,PointDataPack*,
						 TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<PointDataPack>	getPointDataPack(int attrib) const override;

    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    const VisID& whichobj)
				{ otherObjectsMoved( objs, whichobj ); }

    const EM::Fault3D*		emFault() const;
    EM::Fault3D*		emFault();
    void			showSelectedSurfaceData();
    const BufferStringSet*	selectedSurfaceDataNames();
    const Array2D<float>*	getTextureData(int attrib);
    void			matChangeCB(CallBacker*);
    void			setPixelDensity(float dpi) override;

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    const visBase::GeomIndexedShape* getFaultDisplayedPlane() const;
    const visBase::GeomIndexedShape* getFaultDisplayedStickLines() const;
    const ObjectSet<visBase::MarkerSet>* getFaultDisplayedSticks() const;

    const MarkerStyle3D*	getPreferedMarkerStyle() const;
    void			setPreferedMarkerStyle(const MarkerStyle3D&);

protected:

    virtual			~FaultDisplay();

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj) override;
    bool			setRandomPosDataInternal(int attrib,
							 const DataPointSet*,
							 int column,
							 TaskRunner*);
    void			updatePanelDisplay();
    void			updateStickDisplay();
    void			updateIntersectionDisplay();
    void			updateHorizonIntersectionDisplay();
    void			updateDisplay();

    void			updateSingleColor();
    void			updateManipulator();

    bool			isDisplayingSticksUseful() const;
    bool			isDisplayedPlanesUseful() const;

    bool			getCacheValue(int attrib,int version,
					  const Coord3&,float&) const override;
    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;
    RefMan<MPE::ObjectEditor>	getMPEEditor(bool create) override;

    bool			isPicking() const override;
    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			dataTransformCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);
    bool			isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const;

    void			setActiveStick(const EM::PosID&);
    void			updateActiveStickMarker();
    void			updateHorizonIntersections(const VisID& which,
					const ObjectSet<const SurveyObject>&);
    void			removeIntersectionObject(const SurveyObject*,
	    						 bool);
    void			updateEditorMarkers();

    Coord3			disp2world(const Coord3& displaypos) const;

    bool			coincidesWith2DLine(
					const Geometry::FaultStickSurface&,
					int sticknr) const;
    bool			coincidesWithPlane(
					const Geometry::FaultStickSurface&,
					int sticknr,
					TypeSet<Coord3>& intersectpoints) const;
    void			updateStickHiding();
    void			setLineRadius(visBase::GeomIndexedShape*);
    void			sowingFinishedCB(CallBacker*);
    bool			onSection(int sticknr);
    void			showActiveStickMarker();

    int					voiid_ = -1;

    RefMan<visBase::GeomIndexedShape>	paneldisplay_;
    Geometry::ExplFaultStickSurface*	explicitpanels_		= nullptr;

    RefMan<visBase::GeomIndexedShape>	stickdisplay_;
    Geometry::ExplFaultStickSurface*	explicitsticks_		= nullptr;

    RefMan<visBase::GeomIndexedShape>	intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_	= nullptr;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    RefObjectSet<visBase::GeomIndexedShape> horintersections_;
    ObjectSet<Geometry::ExplFaultStickSurface>	horshapes_;
    TypeSet<VisID>			horintersectids_;
    bool				displayintersections_	    = false;
    bool				displayhorintersections_    = false;

    RefMan<visBase::PolyLine3D>		activestickmarker_;
    int					activestick_	= mUdf(int);

    RefMan<MPE::FaultEditor>		faulteditor_;
    RefMan<MPEEditor>			viseditor_;

    Coord3				mousepos_;

    RefObjectSet<DataPointSet>		datapacks_;

    OD::Color				nontexturecol_;

    bool				displaypanels_ = true;

    ObjectSet<Array2D<float> >		texuredatas_;

    RefMan<visBase::DrawStyle>		drawstyle_;
    bool				otherobjects_		= false;
    bool				endstick_		= false;
    EM::PosID				activestickid_ = EM::PosID::udf();

    static const char*			sKeyTriProjection();
    static const char*			sKeyEarthModelID();
    static const char*			sKeyDisplayPanels();
    static const char*			sKeyDisplaySticks();
    static const char*			sKeyDisplayIntersections();
    static const char*			sKeyDisplayHorIntersections();
    static const char*			sKeyUseTexture();
    static const char*			sKeyLineStyle();
    static const char*			sKeyZValues();
};

};
