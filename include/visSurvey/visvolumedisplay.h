#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "mousecursor.h"
#include "ranges.h"
#include "seisdatapack.h"
#include "visboxdragger.h"
#include "visevent.h"
#include "vismarchingcubessurface.h"
#include "visobject.h"
#include "visvolorthoslice.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvobj.h"
#include "vistransform.h"
#include "visvolrenscalarfield.h"
#include "zaxistransform.h"

class MarchingCubesSurface;
class TaskRunner;
class TrcKeyZSampling;
class ZAxisTransformer;
template <class T> class Array3D;

namespace Attrib { class SelSpec; }

namespace visBase
{
    class Material;
}


namespace visSurvey
{

class Scene;

mExpClass(visSurvey) VolumeDisplay : public visBase::VisualObjectImpl
				   , public SurveyObject
{
public:
				VolumeDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, VolumeDisplay,
				    "VolumeDisplay",
				    ::toUiString(sFactoryKeyword()) )

    bool			isInlCrl() const override { return true; }

    static int			cInLine()		{ return 2; }
    static int			cCrossLine()		{ return 1; }
    static int			cTimeSlice()		{ return 0; }

    VisID			addSlice(int dim);
				/*!\note return with removeChild(displayid). */
    void			showVolRen(bool yn);
    bool			isVolRenShown() const;
    VisID			volRenID() const;

    VisID			addIsoSurface(TaskRunner* =nullptr,
					      bool updateisosurface=true);
				/*!\note return with removeChild(displayid). */
    void			removeChild(const VisID&);

    visBase::MarchingCubesSurface* getIsoSurface(int idx);
    void			updateIsoSurface(int,TaskRunner* =nullptr);
    int				getNrIsoSurfaces();
    int				getIsoSurfaceIdx(
				    const visBase::MarchingCubesSurface*) const;
    float			defaultIsoValue() const;
    float			isoValue(
				    const visBase::MarchingCubesSurface*) const;
				/*<Set isovalue and do update. */
    void			setIsoValue(
				    const visBase::MarchingCubesSurface*,
				    float,TaskRunner* =nullptr);

				/*<Seed based settings. set only, no update. */
    char			isFullMode(
				    const visBase::MarchingCubesSurface*)const;
				/*<Return -1 if undefined, 1 if full,
				   0 if seed based. */
    void			setFullMode(
				    const visBase::MarchingCubesSurface*,
				    bool full=1);
				/*<If 0, it is seed based. */
    char			seedAboveIsovalue(
				    const visBase::MarchingCubesSurface*) const;
				/*<-1 undefined, 1 above, 0 below. */
    void			setSeedAboveIsovalue(
				    const visBase::MarchingCubesSurface*,bool);
    MultiID			getSeedsID(
				    const visBase::MarchingCubesSurface*) const;
    void			setSeedsID(const visBase::MarchingCubesSurface*,
					   MultiID);

    bool			turnOn(bool yn) override;
    bool			isOn() const override;
    bool			hasPosModeManipulator() const override
				{ return true; }
    void			showManipulator(bool yn) override;
    bool			isManipulatorShown() const override;
    bool			isManipulated() const override;
    bool			canResetManipulation() const override;
    void			resetManipulation() override;
    void			acceptManipulation() override;
    NotifierAccess*		getMovementNotifier()  override
				{ return &boxMoving; }
    NotifierAccess*		getManipulationNotifier()  override
				{ return &boxMoving; }
    uiString			getManipulationString() const override;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const override;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const override;
    const Attrib::SelSpec*	    getSelSpec(int attrib,
					       int version=-1) const override;
    const TypeSet<float>*	getHistogram(int attrib) const override;
    void			setSelSpec(int attrib,
					   const Attrib::SelSpec&) override;
    void			setSelSpecs(int attrib,
					  const TypeSet<Attrib::SelSpec>&)
								override;

    bool			canHaveMultipleAttribs() const override
				{ return true; }
    int				nrAttribs() const override;
    bool			canAddAttrib(
					int nrattribstoadd=1) const override;
    bool			addAttrib() override;
    bool			canRemoveAttrib() const override;
    bool			removeAttrib(int attrib) override;
    bool			swapAttribs(int attrib0,int attrib1) override;
    void			enableAttrib(int attrib,bool yn) override;
    bool			isAttribEnabled(int attrib) const override;
    void			setAttribTransparency(int attrib,
						      unsigned char) override;
    unsigned char		getAttribTransparency(
						int attrib) const override;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*)
								override;

    visBase::TextureChannel2RGBA*	getChannels2RGBA() override;
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const override;

    float			slicePosition(visBase::OrthogonalSlice*) const;
    void			setSlicePosition(visBase::OrthogonalSlice*,
						    const TrcKeyZSampling&);
    TrcKeyZSampling		sliceSampling(visBase::OrthogonalSlice*) const;
    visBase::OrthogonalSlice*	getSelectedSlice() const;

    float			getValue(int attrib,const Coord3&) const;

    TrcKeyZSampling		getTrcKeyZSampling(bool displayspace,
						    int attrib) const override;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						   bool displayspace,
						   int attrib) const;
    void			setTrcKeyZSampling(
					const TrcKeyZSampling&) override;
    void			setTrcKeyZSampling(const TrcKeyZSampling&,
						bool dragmode);

    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					    Coord3&,BufferString& val,
					    uiString& info) const override;
    void			getObjectInfo(uiString&) const override;
    void			getTreeObjectInfo(uiString&) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
						 int version=0) const override;
    void			setColTabMapperSetup(int attrib,
					const ColTab::MapperSetup&,
					TaskRunner*) override;
    const ColTab::Sequence*	getColTabSequence(int attrib) const override;
    void			setColTabSequence(int attrib,
					const ColTab::Sequence&,
					TaskRunner*) override;
    bool			canSetColTabSequence() const override;

    void			setMaterial(visBase::Material*) override;
    bool			allowMaterialEdit() const override
				{ return true; }
    bool			allowsPicks() const override;
    bool			canDuplicate() const override
				{ return usesShading();}
    SurveyObject*		duplicate(TaskRunner*) const override;

    static bool			canUseVolRenShading();
    void			allowShading(bool yn) override;
    bool			usesShading() const;

    void			getChildren(TypeSet<VisID>&) const override;

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			setRightHandSystem(bool yn) override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    const char*			errMsg() const  override
				{ return errmsg_.str(); }

    bool			writeVolume(int attrib,const char* fnm) const;

    void			setDisplayTransformation(
						const mVisTrans*) override;

    bool			canEnableTextureInterpolation() const override;
    bool			textureInterpolationEnabled() const override;
    void			enableTextureInterpolation(bool) override;

    static const char*		sKeyVolDepthKey()	{ return "VolDepthKey";}
    static const char*		sKeyVolPlaneKey()	{ return "VolPlaneKey";}
    static const char*		sKeyInDepthVolResize()
						{ return "InDepthVolResize"; }

protected:
				~VolumeDisplay();

    bool			usesDataPacks() const override	{ return true; }
    bool			setVolumeDataPack(int attrib,VolumeDataPack*,
						TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<VolumeDataPack> getVolumeDataPack(int attrib) const override;

    bool			updateSeedBasedSurface(int,
						       TaskRunner* =nullptr);
    void			materialChange(CallBacker*);
    void			updateIsoSurfColor();
    bool			pickable() const { return true; }
    bool			rightClickable() const override { return true; }
    bool			selectable() const override { return true; }
    bool			isSelected() const override;
    const MouseCursor*		getMouseCursor() const override
				{ return &mousecursor_; }
    void			setScene(Scene*) override;
    void			updateAttribEnabling();
    void			getObjectInfoText(uiString& info,
						  bool compact) const;

    Notifier<VolumeDisplay>	boxMoving;

    RefMan<visBase::BoxDragger>			boxdragger_;
    RefMan<visBase::VolumeRenderScalarField>	scalarfield_;
    RefMan<visBase::TextureChannel2RGBA>	texchannel2rgba_;
/* OSG_TODO: Replace VolrenDisplay with OSG equivalent
    RefMan<visBase::VolrenDisplay>		volren_;
*/
    RefObjectSet<visBase::OrthogonalSlice>	slices_;
    RefObjectSet<visBase::MarchingCubesSurface> isosurfaces_;
    struct IsosurfaceSetting
    {
				IsosurfaceSetting();
	bool			operator==(const IsosurfaceSetting&) const;
	IsosurfaceSetting&	operator=(const IsosurfaceSetting&);

	float			isovalue_;
	char			mode_;
	char			seedsaboveisoval_;
	MultiID			seedsid_;
    };

    TypeSet<IsosurfaceSetting>	isosurfsettings_;
    TypeSet<char>		sections_;

    void			draggerStartCB(CallBacker*);
    void			draggerMoveCB(CallBacker*);
    void			draggerFinishCB(CallBacker*);
    void			updateDraggerLimits(bool dragmode=false);
    bool			keepdraggerinsidetexture_;
    TrcKeyZSampling		draggerstartcs_;
    TrcKeyZSampling		texturecs_;

    void			sliceMoving(CallBacker*);
    void			setData(const RegularSeisDataPack*,
					int datatype=0);

    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool updateic,bool updatez);
    void			updateMouseCursorCB(CallBacker*) override;
    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;

    RefMan<ZAxisTransform>	datatransform_;
    ZAxisTransformer*		datatransformer_	= nullptr;

    struct AttribData
    {
					AttribData();
					~AttribData();

	TypeSet<Attrib::SelSpec>*		selspec_;
	Attrib::SelSpec&			as_;
	ConstRefMan<RegularSeisDataPack>	cache_;
    };

    ObjectSet<AttribData>	attribs_;

    BufferString		sliceposition_;
    BufferString		slicename_;
    TrcKeyZSampling		csfromsession_;

    MouseCursor			mousecursor_;
    RefMan<visBase::EventCatcher> eventcatcher_;

    bool			isinited_	= false;
    bool			ismanip_	= false;
    bool			onoffstatus_	= true;

    ConstRefMan<mVisTrans>	displaytrans_;

    static const char*		sKeyVolumeID();
    static const char*		sKeyInline();
    static const char*		sKeyCrossLine();
    static const char*		sKeyTime();
    static const char*		sKeyVolRen();
    static const char*		sKeyNrSlices();
    static const char*		sKeySlice();
    static const char*		sKeyTexture();

    static const char*		sKeyNrIsoSurfaces();
    static const char*		sKeyIsoValueStart();
    static const char*		sKeyIsoOnStart();
    static const char*		sKeySurfMode();
    static const char*		sKeySeedsMid();
    static const char*		sKeySeedsAboveIsov();
};

} // namespace visSurvey
