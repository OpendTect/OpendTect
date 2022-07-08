#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"
#include "mousecursor.h"
#include "vissurvobj.h"
#include "ranges.h"


class MarchingCubesSurface;
class RegularSeisDataPack;
class TaskRunner;
class TrcKeyZSampling;
class ZAxisTransform;
class ZAxisTransformer;
template <class T> class Array3D;

namespace Attrib { class SelSpec; }

namespace visBase
{
    class MarchingCubesSurface;
    class Material;
    class BoxDragger;
    class VolumeRenderScalarField;
    class OrthogonalSlice;
    class TextureChannel2RGBA;
}


namespace visSurvey
{

class Scene;

mExpClass(visSurvey) VolumeDisplay : public visBase::VisualObjectImpl,
				     public SurveyObject
{
public:
				VolumeDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,VolumeDisplay,
				    "VolumeDisplay",
				    toUiString(sFactoryKeyword()));

    bool			isInlCrl() const { return true; }

    static int			cInLine()		{ return 2; }
    static int			cCrossLine()		{ return 1; }
    static int			cTimeSlice()		{ return 0; }

    int				addSlice(int dim);
				/*!\note return with removeChild(displayid). */
    void			showVolRen(bool yn);
    bool			isVolRenShown() const;
    int				volRenID() const;

    int				addIsoSurface(TaskRunner* = 0,
					      bool updateisosurface = true);
				/*!\note return with removeChild(displayid). */
    void			removeChild(int displayid);

    visBase::MarchingCubesSurface* getIsoSurface(int idx);
    void			updateIsoSurface(int,TaskRunner* = 0);
    int				getNrIsoSurfaces();
    int				getIsoSurfaceIdx(
				    const visBase::MarchingCubesSurface*) const;
    float			defaultIsoValue() const;
    float			isoValue(
				    const visBase::MarchingCubesSurface*) const;
				/*<Set isovalue and do update. */
    void			setIsoValue(
				    const visBase::MarchingCubesSurface*,
				    float,TaskRunner* =0);

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

    bool                        turnOn(bool yn);
    bool                        isOn() const;
    bool			hasPosModeManipulator() const	{ return true; }
    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const;
    void			resetManipulation();
    void			acceptManipulation();
    NotifierAccess*		getMovementNotifier() { return &boxMoving; }
    NotifierAccess*		getManipulationNotifier() { return &boxMoving; }
    BufferString		getManipulationString() const;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const;
    const Attrib::SelSpec*	    getSelSpec(int attrib,int version=0) const;
    const TypeSet<float>*	getHistogram(int attrib) const;
    void			setSelSpec(int attrib,const Attrib::SelSpec&);
    void			setSelSpecs(int attrib,
					  const TypeSet<Attrib::SelSpec>&);

    bool			canHaveMultipleAttribs() const { return true; }
    int				nrAttribs() const;
    bool			canAddAttrib(int nrattribstoadd=1) const;
    bool			addAttrib();
    bool			canRemoveAttrib() const;
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    void			setAttribTransparency(int attrib,unsigned char);
    unsigned char		getAttribTransparency(int attrib) const;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*);

    visBase::TextureChannel2RGBA*	getChannels2RGBA();
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const;

    float			slicePosition(visBase::OrthogonalSlice*) const;
    void			setSlicePosition(visBase::OrthogonalSlice*,
						    const TrcKeyZSampling&);
    TrcKeyZSampling		sliceSampling(visBase::OrthogonalSlice*) const;
    visBase::OrthogonalSlice*	getSelectedSlice() const;

    float			getValue(int attrib,const Coord3&) const;

    TrcKeyZSampling		getTrcKeyZSampling(int attrib) const;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						   bool displayspace,
						   int attrib) const;
    void			setTrcKeyZSampling(const TrcKeyZSampling&,
						bool dragmode=false);
    bool			setDataVolume(int attrib,
					      const RegularSeisDataPack*,
					      TaskRunner*);
    const RegularSeisDataPack*	getCacheVolume(int attrib) const;
    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::MgrID	   getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
				     		Coord3&,BufferString& val,
						BufferString& info) const;
    void			getObjectInfo(BufferString&) const;
    void			getTreeObjectInfo(uiString&) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
						     int version=0) const;
    void			setColTabMapperSetup(int attrib,
					const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int attrib) const;
    void			setColTabSequence(int attrib,
					const ColTab::Sequence&,TaskRunner*);
    bool			canSetColTabSequence() const;

    void			setMaterial(visBase::Material*);
    bool			allowMaterialEdit() const	{ return true; }
    virtual bool		allowsPicks() const;
    bool			canDuplicate() const	{ return usesShading();}
    visSurvey::SurveyObject*	duplicate(TaskRunner*) const;

    static bool			canUseVolRenShading();
    void			allowShading(bool yn);
    bool			usesShading() const;

    void			getChildren(TypeSet<int>&) const;

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			setRightHandSystem(bool yn);

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    const char*			errMsg() const { return errmsg_.str(); }

    bool			writeVolume(int attrib,const char* fnm) const;

    void			setDisplayTransformation(const mVisTrans*);

    bool			canEnableTextureInterpolation() const;
    bool			textureInterpolationEnabled() const;
    void			enableTextureInterpolation(bool);

    static const char*		sKeyVolDepthKey()	{ return "VolDepthKey";}
    static const char*		sKeyVolPlaneKey()	{ return "VolPlaneKey";}
    static const char*		sKeyInDepthVolResize()
						{ return "InDepthVolResize"; }

protected:
				~VolumeDisplay();

    Notifier<VolumeDisplay>	boxMoving;

    bool			updateSeedBasedSurface(int,TaskRunner* = 0);
    void			materialChange(CallBacker*);
    void			updateIsoSurfColor();
    bool			pickable() const { return true; }
    bool			rightClickable() const { return true; }
    bool			selectable() const { return true; }
    bool			isSelected() const;
    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }
    void			setScene(Scene*);
    void			updateAttribEnabling();
    void			getObjectInfoText(uiString& info,
						  bool compact) const;

    visBase::BoxDragger*			boxdragger_;
    visBase::VolumeRenderScalarField*		scalarfield_;
    visBase::TextureChannel2RGBA*		texchannel2rgba_;
/* OSG_TODO: Replace VolrenDisplay with OSG equivalent
    visBase::VolrenDisplay*			volren_;
*/
    ObjectSet<visBase::OrthogonalSlice>		slices_;
    ObjectSet<visBase::MarchingCubesSurface>	isosurfaces_;
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
    void			updateMouseCursorCB(CallBacker*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    ZAxisTransform*		datatransform_;
    ZAxisTransformer*		datatransformer_;

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
    visBase::EventCatcher*	eventcatcher_;

    bool			isinited_;
    bool			ismanip_;
    bool                        onoffstatus_;

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


