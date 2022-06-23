#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
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


    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    virtual int			nrResolutions() const;
    virtual void		setResolution(int,TaskRunner*);

    SurveyObject::AttribFormat	getAttributeFormat(int) const
				{ return SurveyObject::RandomPos; }
    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			getRandomPosCache(int,DataPointSet&) const;
    void			setRandomPosData(int,const DataPointSet*,
						 TaskRunner*);

    bool			hasColor() const		{ return true; }
    bool			usesColor() const;
    OD::Color			getColor() const;
    void			setColor(OD::Color);
    bool			allowMaterialEdit() const	{ return true; }

    void			useTexture( bool yn, bool trigger );
    bool			usesTexture() const;
    bool			showsTexture() const;
    bool			canShowTexture() const;

    void			setDepthAsAttrib(int);
    void			enableAttrib(int attrib,bool yn);
    bool			hasSingleColorFallback() const	{ return true; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			triangulateAlg(mFltTriProj);
    mFltTriProj			triangulateAlg() const;

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;
    bool			arePanelsDisplayedInFull() const;

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*);

    bool			removeSelections(TaskRunner*);
    bool			canRemoveSelection() const	{ return true; }

    void			setOnlyAtSectionsDisplay(bool);
    bool			displayedOnlyAtSections() const;

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
    void			turnOnSelectionMode(bool);
    bool			isInStickSelectMode() const;

    const OD::LineStyle*	lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);
    const MarkerStyle3D*	markerStyle() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    bool			hasSpecificMarkerColor() const	{ return true; }

    virtual void		getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return MultiTextureSurveyObject
					::getMousePosInfo(ei,iop); }
   void				getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const;

    bool			allowsPicks() const		{ return true; }
    bool			isVerticalPlane() const		{return false;}
    bool			canBDispOn2DViewer() const	{return false;}
    int				addDataPack(const DataPointSet&) const ;
    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPackMgr::ID		getDataPackMgrID() const
				{ return DataPackMgr::SurfID(); }

    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    int whichobj)
				{ otherObjectsMoved( objs, whichobj ); }

    EM::Fault3D*		emFault();
    void			showSelectedSurfaceData();
    const BufferStringSet*	selectedSurfaceDataNames();
    const Array2D<float>*	getTextureData(int attrib);
    void			matChangeCB(CallBacker*);
    virtual void		setPixelDensity(float dpi);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    const visBase::GeomIndexedShape* getFaultDisplayedPlane() const;
    const visBase::GeomIndexedShape* getFaultDisplayedStickLines() const;
    const ObjectSet<visBase::MarkerSet>& getFaultDisplayedSticks() const;

    const MarkerStyle3D*	getPreferedMarkerStyle() const;
    void			setPreferedMarkerStyle(const MarkerStyle3D&);

protected:

    virtual			~FaultDisplay();
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj);
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

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    virtual void		addCache();
    virtual void		removeCache(int);
    virtual void		swapCache(int,int);
    virtual void		emptyCache(int);
    virtual bool		hasCache(int) const;

    bool			isPicking() const;
    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			dataTransformCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);
    bool			isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const;

    void			setActiveStick(const EM::PosID&);
    void			updateActiveStickMarker();
    void			updateHorizonIntersections( int whichobj,
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
    TypeSet<int>			horintersectids_;
    bool				displayintersections_;
    bool				displayhorintersections_;

    visBase::PolyLine3D*		activestickmarker_;
    int					activestick_;

    MPE::FaultEditor*			faulteditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    TypeSet<DataPack::ID>		datapackids_;

    bool				validtexture_;
    OD::Color				nontexturecol_;
    bool				usestexture_;

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
};

};


