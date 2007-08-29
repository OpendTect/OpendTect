#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.46 2007-08-29 14:24:15 cvskris Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class ColorAttribSel;
class CubeSampling;
class MarchingCubesSurface;
class ZAxisTransform;

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
		      public visSurvey::SurveyObject
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

    int				addIsoSurface();
    				/*!\note return with removeChild(displayid). */
    void			removeChild(int displayid);

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
    float			isoValue(
	    			    const visBase::MarchingCubesSurface*) const;
    void			setIsoValue(
	    			    const visBase::MarchingCubesSurface*,float);

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
    CubeSampling		getCubeSampling(bool manippos,int attrib) const;

    visBase::Transformation*			voltrans_;
    visBase::BoxDragger*			boxdragger_;
    visBase::VolumeRenderScalarField*		scalarfield_;
    visBase::VolrenDisplay*			volren_;
    ObjectSet<visBase::OrthogonalSlice>		slices_;
    ObjectSet<visBase::MarchingCubesSurface>	isosurfaces_;
    TypeSet<float>				isovalues_;

    void			manipMotionFinishCB(CallBacker*);
    void			sliceMoving(CallBacker*);
    void			setData(const Attrib::DataCubes*,
	    				int datatype=0);

    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool updateic,bool updatez);

    ZAxisTransform*		datatransform_;
    int				datatransformvoihandle_;

    DataPack::ID		cacheid_;
    const Attrib::DataCubes*	cache_;
    Attrib::SelSpec&		as_;
    bool			allowshading_;
    BufferString		sliceposition_;
    BufferString		slicename_;

    static visBase::FactoryEntry oldnameentry;

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

};

} // namespace visSurvey


#endif
