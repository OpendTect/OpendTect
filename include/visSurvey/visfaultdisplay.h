#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "vismultiattribsurvobj.h"
#include "visemsticksetdisplay.h"

#include "emposid.h"
#include "explfaultsticksurface.h"
#include "ranges.h"

class DataPointSet;
class ZAxisTransform;

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
    class PolyLine3D;
    class DrawStyle;
}

namespace EM { class Fault3D; }
namespace MPE { class FaultEditor; }
namespace Geometry
{
    class ExplPlaneIntersection;
    class FaultStickSurface;
}

template <class T > class Array2D;

namespace visSurvey
{
class MPEEditor;
class HorizonDisplay;

/*!\brief


*/

mExpClass(visSurvey) FaultDisplay : public MultiTextureSurveyObject
				  , public StickSetDisplay
{ mODTextTranslationClass(FaultDisplay);
public:
				FaultDisplay();

				mDefaultFactoryInstantiation(
				visSurvey::SurveyObject,FaultDisplay,
				"FaultDisplay",
				toUiString(sFactoryKeyword()));

    MultiID			getMultiID() const override;
    bool			isInlCrl() const override { return false; }

    int				nrResolutions() const override;
    void			setResolution(int,TaskRunner*) override;

    SurveyObject::AttribFormat	getAttributeFormat(int) const override
				{ return SurveyObject::RandomPos; }
    void			getRandomPos(DataPointSet&,
					     TaskRunner*) const override;
    void			getRandomPosCache(int,
						DataPointSet&) const override;
    void			setRandomPosData(int,const DataPointSet*,
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
					BufferString& info) const override;

    bool			allowsPicks() const override	{ return true; }
    bool			isVerticalPlane() const override {return false;}
    bool			canBDispOn2DViewer() const override
				{ return false; }
    DataPackID			addDataPack(const DataPointSet&) const;
    bool			setDataPackID(int attrib,DataPackID,
					      TaskRunner*) override;
    DataPackID			getDataPackID(int attrib) const override;
    DataPackID			getDisplayedDataPackID(int attr) const override;
    DataPackMgr::MgrID		getDataPackMgrID() const override
				{ return DataPackMgr::SurfID(); }

    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    VisID whichobj)
				{ otherObjectsMoved( objs, whichobj ); }

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
    mDeprecated("getFaultDisplayedSticks_")
    const ObjectSet<visBase::MarkerSet>& getFaultDisplayedSticks() const;

    const MarkerStyle3D*	getPreferedMarkerStyle() const;
    void			setPreferedMarkerStyle(const MarkerStyle3D&);
    bool			isAlreadyTransformed() const;

protected:

    virtual			~FaultDisplay();

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj) override;
    void			setRandomPosDataInternal(int attrib,
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

    bool			getCacheValue(int attrib,int version,
					  const Coord3&,float&) const override;
    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;

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
    void			updateHorizonIntersections(VisID whichobj,
					const ObjectSet<const SurveyObject>&);
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

    ZAxisTransform*			zaxistransform_;
    int					voiid_;

    visBase::GeomIndexedShape*		paneldisplay_;
    Geometry::ExplFaultStickSurface*	explicitpanels_;

    visBase::GeomIndexedShape*		stickdisplay_;
    Geometry::ExplFaultStickSurface*	explicitsticks_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    ObjectSet<visBase::GeomIndexedShape> horintersections_;
    ObjectSet<Geometry::ExplFaultStickSurface>	horshapes_;
    TypeSet<VisID>			horintersectids_;
    bool				displayintersections_;
    bool				displayhorintersections_;

    visBase::PolyLine3D*		activestickmarker_;
    int					activestick_;

    MPE::FaultEditor*			faulteditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    TypeSet<DataPackID>		datapackids_;

    OD::Color				nontexturecol_;

    bool				displaypanels_;

    ObjectSet<Array2D<float> >		texuredatas_;

    visBase::DrawStyle*			drawstyle_;
    bool				otherobjects_;
    bool				endstick_;
    EM::PosID				activestickid_;

    static const char*			sKeyTriProjection();
    static const char*			sKeyEarthModelID();
    static const char*			sKeyDisplayPanels();
    static const char*			sKeyDisplaySticks();
    static const char*			sKeyDisplayIntersections();
    static const char*			sKeyDisplayHorIntersections();
    static const char*			sKeyUseTexture();
    static const char*			sKeyLineStyle();
    static const char*			sKeyZValues();

    bool				isDisplayingSticksUseful() const;
public:
    const ObjectSet<visBase::MarkerSet>* getFaultDisplayedSticks_() const;

};

};
