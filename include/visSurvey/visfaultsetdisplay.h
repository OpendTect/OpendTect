#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "datapointset.h"
#include "emfaultset3d.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismultiattribsurvobj.h"
#include "vistexturechannels.h"
#include "zaxistransform.h"

#include "emposid.h"
#include "explfaultsticksurface.h"
#include "ranges.h"

namespace Geometry
{
    class ExplPlaneIntersection;
    class FaultStickSurface;
}

template <class T > class Array2D;

namespace visSurvey
{
mExpClass(visSurvey) FaultSetDisplay : public MultiTextureSurveyObject
{ mODTextTranslationClass(FaultSetDisplay);
public:
				FaultSetDisplay();

				mDefaultFactoryInstantiation(
				SurveyObject, FaultSetDisplay,
				"FaultSetDisplay",
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

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;

    void			triangulateAlg(mFltTriProj);
    mFltTriProj			triangulateAlg() const;

    void			displayPanels(bool);
    bool			arePanelsDisplayed() const;
    bool			arePanelsDisplayedInFull() const;

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*) override;

    bool			canRemoveSelection() const override
				{ return false; }

    void			setOnlyAtSectionsDisplay(bool) override;
    bool			displayedOnlyAtSections() const override;

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;
    bool			canDisplayIntersections() const;

    void			displayHorizonIntersections(bool yn);
    bool			areHorizonIntersectionsDisplayed() const;
    bool			canDisplayHorizonIntersections() const;

    Notifier<FaultSetDisplay>	colorchange;
    Notifier<FaultSetDisplay>	displaymodechange;

    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;
    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return MultiTextureSurveyObject
					::getMousePosInfo(ei,iop); }
   void				getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					uiString& info) const override;

    bool			allowsPicks() const override	 {return false;}
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

    EM::FaultSet3D*		emFaultSet();
    void			matChangeCB(CallBacker*);
    void			setPixelDensity(float dpi) override;
    virtual void		setAttribTransparency(int,
						      unsigned char) override;

    bool			addAttrib() override;
    bool			removeAttrib(int attrib) override;
    bool			swapAttribs(int a0,int a1) override;

    const TypeSet<float>*	getHistogram(int) const override;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
						 int version) const override;
    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,
					TaskRunner*) override;

    bool			canSetColTabSequence() const override;
    void			setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*) override;

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    const visBase::GeomIndexedShape* getFaultDisplayedPlane(int) const;

protected:
				~FaultSetDisplay();

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj) override;
    EM::FaultID			getFaultID(const visBase::EventInfo&) const;
    bool			setRandomPosDataInternal(int attrib,
							 const DataPointSet*,
							 int column,
							 TaskRunner*);
    void			updatePanelDisplay();
    void			updateIntersectionDisplay();
    void			updateHorizonIntersectionDisplay();
    void			updateDisplay();

    void			updateSingleColor();

    bool			getCacheValue(int attrib,int version,
					  const Coord3&,float&) const override;
    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			dataTransformCB(CallBacker*);

    void			updateHorizonIntersections(const VisID& which,
					const ObjectSet<const SurveyObject>&);

    Coord3			disp2world(const Coord3& displaypos) const;

    void			setLineRadius(visBase::GeomIndexedShape*);

    RefMan<ZAxisTransform>		zaxistransform_;
    int					voiid_ = -1;

    RefObjectSet<visBase::GeomIndexedShape>	paneldisplays_;
    ObjectSet<Geometry::ExplFaultStickSurface>	explicitpanels_;

    RefObjectSet<visBase::GeomIndexedShape>	intersectiondisplays_;
    ObjectSet<Geometry::ExplPlaneIntersection>	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    RefObjectSet<visBase::GeomIndexedShape> horintersections_;
    ObjectSet<Geometry::ExplFaultStickSurface>	horshapes_;

    RefObjectSet<visBase::TextureChannels> channelset_;

    TypeSet<int>			horintersectids_;
    bool				displayintersections_ = false;
    bool				displayhorintersections_ = false;

    Coord3				mousepos_;

    RefObjectSet<DataPointSet>		datapacks_;

    OD::Color				nontexturecol_;

    bool				displaypanels_ = true;

    RefMan<EM::FaultSet3D>		faultset_;

    ObjectSet<ObjectSet<Array2D<float> > >  texturedataset_;

    RefMan<visBase::DrawStyle>		drawstyle_;
    bool				otherobjects_ = false;

    ConstRefMan<mVisTrans>		displaytransform_;
    RefMan<visBase::EventCatcher>	eventcatcher_;

    static const char*			sKeyTriProjection();
    static const char*			sKeyEarthModelID();
    static const char*			sKeyDisplayPanels();
    static const char*			sKeyDisplayIntersections();
    static const char*			sKeyDisplayHorIntersections();
    static const char*			sKeyLineStyle();
    static const char*			sKeyUseTexture();
    static const char*			sKeyZValues();
};

};
