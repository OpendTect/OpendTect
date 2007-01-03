#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.35 2007-01-03 18:29:06 cvskris Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class ColorAttribSel;
class CubeSampling;

namespace Attrib { class SelSpec; class DataCubes; }

namespace visBase
{
    class VisColorTab;
    class Material;
    class BoxDragger;
    class VolumeTexture;
    class VolrenDisplay;
    class OrthogonalSlice;
};

namespace visSurvey
{
class Scene;


class VolumeDisplay :  public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:

    static VolumeDisplay*	create()
				mCreateDataObj(VolumeDisplay);
    bool			isInlCrl() const { return true; }

    int				addSlice(int dim);
    void			removeSlice(int displayid);
    void			showVolRen(bool yn);
    bool			isVolRenShown() const;
    int				volRenID() const;

    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const;
    void			resetManipulation();
    void			acceptManipulation();
    NotifierAccess*		getMovementNotification() {return &slicemoving;}
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
    bool			setDataVolume( int attrib,
	    				       const Attrib::DataCubes* );
				/*!<\note slc becomes mine */
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
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

    Notifier<VolumeDisplay>	slicemoving;
    BufferString		sliceposition;
    BufferString		slicename;

    void			getChildren(TypeSet<int>&) const;

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~VolumeDisplay();
    CubeSampling		getCubeSampling(bool manippos,int attrib) const;
    void			setUpConnections();

    visBase::Transformation*		voltrans;
    visBase::BoxDragger*		boxdragger;
    visBase::VolumeTexture*		texture;
    visBase::VolrenDisplay*		volren;
    ObjectSet<visBase::OrthogonalSlice>	slices;

    void			manipMotionFinishCB(CallBacker*);
    void			sliceMoving(CallBacker*);
    void			setData(const Attrib::DataCubes*,int datatype=0);

    int				useOldPar(const IOPar&);

    const Attrib::DataCubes*	cache;
    Attrib::SelSpec&		as;

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

};


#endif
