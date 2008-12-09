#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.60 2008-12-09 19:45:25 cvsyuancheng Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "mousecursor.h"
#include "vissurvobj.h"
#include "ranges.h"

class CubeSampling;
class MarchingCubesSurface;
class ZAxisTransform;
class TaskRunner;
class ZAxisTransformer;
template <class T> class Array3D;

namespace Attrib { class SelSpec; class DataCubes; }

namespace visBase
{
    class MarchingCubesSurface;
    class VisColorTab;
    class Material;
    class BoxDragger;
    class VolumeRenderScalarField;
    class VolrenDisplay;
    class OrthogonalSlice;
}


namespace visSurvey
{

class Scene;

class VolumeDisplay : public visBase::VisualObjectImpl,
		      public SurveyObject
{
public:
    static VolumeDisplay*	create()
				mCreateDataObj(VolumeDisplay);
    bool			isInlCrl() const	{ return true; }

    static int			cInLine() 		{ return 2; }
    static int			cCrossLine() 		{ return 1; }
    static int			cTimeSlice() 		{ return 0; }

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
    const int			getNrIsoSurfaces();
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
    void			setSeedsID(const visBase::MarchingCubesSurface*,					   MultiID);

    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const;
    void			resetManipulation();
    void			acceptManipulation();
    NotifierAccess*		getMovementNotifier() { return &slicemoving; }
    NotifierAccess*		getManipulationNotifier() {return &slicemoving;}
    BufferString		getManipulationString() const;

    visSurvey::SurveyObject::AttribFormat getAttributeFormat() const;
    const Attrib::SelSpec*	getSelSpec(int attrib) const;
    const TypeSet<float>* 	getHistogram(int attrib) const;
    int				getColTabID(int attrib) const;
    void			setSelSpec(int attrib,const Attrib::SelSpec&);

    float			slicePosition(visBase::OrthogonalSlice*) const;
    float			getValue(const Coord3&) const;

    CubeSampling		getCubeSampling(int attrib) const;
    void			setCubeSampling(const CubeSampling&);
    bool			setDataVolume(int attrib,
	    				      const Attrib::DataCubes*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
    bool			setDataPackID(int attrib,DataPack::ID);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID     getDataPackMgrID() const
	                                { return DataPackMgr::CubeID; }
    void			getMousePosInfo(const visBase::EventInfo&,
	    			     		const Coord3&,BufferString& val,
	    					BufferString& info) const;

    void			setColorTab(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTab();
    const visBase::VisColorTab&	getColorTab() const;

    void			setMaterial(visBase::Material*);
    bool			allowMaterialEdit() const	{ return true; }
    virtual bool		allowPicks() const;
    bool			canDuplicate() const		{ return true; }
    visSurvey::SurveyObject*	duplicate() const;

    void			allowShading(bool yn ) { allowshading_ = yn; }

    SoNode*			getInventorNode();

    Notifier<VolumeDisplay>	slicemoving;

    void			getChildren(TypeSet<int>&) const;

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~VolumeDisplay();
    bool			updateSeedBasedSurface(int,TaskRunner* = 0);
    CubeSampling		getCubeSampling(bool manippos,bool display,
	    					int attrib) const;
    void			materialChange(CallBacker*);
    void			colTabChange(CallBacker*);
    bool			pickable() const { return true; }
    bool			rightClickable() const { return true; }
    bool			selectable() const { return false; }
    				//!<Makes impossible to click on it and
				//!<by mistake get the drag-box up
    bool			isSelected() const;
    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    visBase::Transformation*			voltrans_;
    visBase::BoxDragger*			boxdragger_;
    visBase::VolumeRenderScalarField*		scalarfield_;
    visBase::VolrenDisplay*			volren_;
    ObjectSet<visBase::OrthogonalSlice>		slices_;
    ObjectSet<visBase::MarchingCubesSurface>	isosurfaces_;
    struct IsosurfaceSetting
    {
				IsosurfaceSetting();
	bool			operator==(const IsosurfaceSetting&) const;
	IsosurfaceSetting& 	operator=(const IsosurfaceSetting&);

	float			isovalue_;
	char			mode_;
	char			seedsaboveisoval_;
	MultiID			seedsid_;
    };

    TypeSet<IsosurfaceSetting>	settings_;
    TypeSet<char>		sections_;

    void			manipMotionFinishCB(CallBacker*);
    void			sliceMoving(CallBacker*);
    void			setData(const Attrib::DataCubes*,
	    				int datatype=0);

    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool updateic,bool updatez);
    void			updateMouseCursorCB(CallBacker*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			triggerSel() { updateMouseCursorCB( 0 ); }
    void			triggerDeSel() { updateMouseCursorCB( 0 ); }


    ZAxisTransform*		datatransform_;
    ZAxisTransformer*		datatransformer_;

    DataPack::ID		cacheid_;
    const Attrib::DataCubes*	cache_;
    Attrib::SelSpec&		as_;
    bool			allowshading_;
    BufferString		sliceposition_;
    BufferString		slicename_;
    CubeSampling		csfromsession_;

    MouseCursor			mousecursor_;
    visBase::EventCatcher*	eventcatcher_;


    static const char*		volumestr;
    static const char*		inlinestr;
    static const char*		crosslinestr;
    static const char*		timestr;
    static const char*		volrenstr;
    static const char*		nrslicesstr;
    static const char*		slicestr;
    static const char*		texturestr;

    static const char*		inlineposstr;
    static const char*		crosslineposstr;
    static const char*		timeposstr;
    static const char*		inlineshowstr;
    static const char* 		crosslineshowstr;
    static const char* 		timeshowstr;

    static const char*		sKeyNrIsoSurfaces() { return "Nr Isosurfaces"; }
    static const char*		sKeyIsoValueStart() { return "Iso Value "; }
    static const char*		sKeyIsoOnStart() { return "Iso Surf On "; }
    static const char*		sKeySurfMode() { return "Surf Mode"; }
    static const char*		sKeySeedsMid() { return "Surf Seeds Mid"; }
    static const char*		sKeySeedsAboveIsov() { return "Above IsoVal"; }

};

} // namespace visSurvey


#endif
