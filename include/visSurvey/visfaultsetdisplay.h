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

#include "emposid.h"
#include "explfaultsticksurface.h"
#include "ranges.h"

class DataPointSet;
class ZAxisTransform;

namespace visBase
{
    class EventCatcher;
    class GeomIndexedShape;
    class Transformation;
    class PolyLine3D;
    class DrawStyle;
}

namespace EM { class FaultSet3D; }
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
				visSurvey::SurveyObject,FaultSetDisplay,
				"FaultSetDisplay",
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
    bool			canShowTexture() const;

    void			setDepthAsAttrib(int);
    void			enableAttrib(int attrib,bool yn);
    bool			hasSingleColorFallback() const	{ return true; }

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			triangulateAlg(mFltTriProj);
    mFltTriProj			triangulateAlg() const;

    void			displayPanels(bool);
    bool			arePanelsDisplayed() const;
    bool			arePanelsDisplayedInFull() const;

    bool			setEMObjectID(const EM::ObjectID&);
    EM::ObjectID		getEMObjectID() const;

    void			setScene(Scene*);

    bool			canRemoveSelection() const	{return false;}

    void			setOnlyAtSectionsDisplay(bool);
    bool			displayedOnlyAtSections() const;

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;
    bool			canDisplayIntersections() const;

    void			displayHorizonIntersections(bool yn);
    bool			areHorizonIntersectionsDisplayed() const;
    bool			canDisplayHorizonIntersections() const;

    Notifier<FaultSetDisplay>	colorchange;
    Notifier<FaultSetDisplay>	displaymodechange;

    const OD::LineStyle*	lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);
    virtual void		getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return MultiTextureSurveyObject
					::getMousePosInfo(ei,iop); }
   void				getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const;

    bool			allowsPicks() const		{return false;}
    bool			isVerticalPlane() const		{return false;}
    bool			canBDispOn2DViewer() const	{return false;}
    int				addDataPack(const DataPointSet&) const ;
    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPackMgr::MgrID		getDataPackMgrID() const
				{ return DataPackMgr::PointID(); }

    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    int whichobj)
				{ otherObjectsMoved( objs, whichobj ); }

    EM::FaultSet3D*		emFaultSet();
    void			matChangeCB(CallBacker*);
    virtual void		setPixelDensity(float dpi);
    virtual void		setAttribTransparency(int,unsigned char);

    virtual bool		addAttrib();
    virtual bool		removeAttrib(int attrib);
    virtual bool		swapAttribs(int a0,int a1);

    const TypeSet<float>*	getHistogram(int) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
						     int version) const;
    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,TaskRunner*);

    bool			canSetColTabSequence() const;
    void			setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    const visBase::GeomIndexedShape* getFaultDisplayedPlane(int) const;

protected:

    virtual			~FaultSetDisplay();
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj);
    int				getFaultID(const visBase::EventInfo&) const;
    void			setRandomPosDataInternal(int attrib,
							 const DataPointSet*,
							 int column,
							 TaskRunner*);
    void			updatePanelDisplay();
    void			updateIntersectionDisplay();
    void			updateHorizonIntersectionDisplay();
    void			updateDisplay();

    void			updateSingleColor();

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    virtual void		addCache();
    virtual void		removeCache(int);
    virtual void		swapCache(int,int);
    virtual void		emptyCache(int);
    virtual bool		hasCache(int) const;

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			dataTransformCB(CallBacker*);

    void			updateHorizonIntersections( int whichobj,
					const ObjectSet<const SurveyObject>&);

    Coord3			disp2world(const Coord3& displaypos) const;

    void			setLineRadius(visBase::GeomIndexedShape*);

    ZAxisTransform*			zaxistransform_;
    int					voiid_;

    ObjectSet<visBase::GeomIndexedShape>	paneldisplays_;
    ObjectSet<Geometry::ExplFaultStickSurface>	explicitpanels_;

    ObjectSet<visBase::GeomIndexedShape>	intersectiondisplays_;
    ObjectSet<Geometry::ExplPlaneIntersection>	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    ObjectSet<visBase::GeomIndexedShape> horintersections_;
    ObjectSet<Geometry::ExplFaultStickSurface>	horshapes_;

    ObjectSet<visBase::TextureChannels> channelset_;

    TypeSet<int>			horintersectids_;
    bool				displayintersections_;
    bool				displayhorintersections_;

    Coord3				mousepos_;

    TypeSet<DataPack::ID>		datapackids_;

    OD::Color				nontexturecol_;

    bool				displaypanels_;

    EM::FaultSet3D*			faultset_;

    ObjectSet<ObjectSet<Array2D<float> > >  texturedataset_;

    visBase::DrawStyle*			drawstyle_;
    bool				otherobjects_;

    const mVisTrans*			displaytransform_;
    visBase::EventCatcher*		eventcatcher_;

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
