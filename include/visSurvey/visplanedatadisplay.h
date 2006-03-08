#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.74 2006-03-08 13:48:50 cvsnanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class CubeSampling;
template <class T> class Array2D;

namespace visBase
{
    class Coordinates;
    class DepthTabPlaneDragger;
    class FaceSet;
    class GridLines;
    class MultiTexture2; 
    class PickStyle;
};


namespace Attrib { class SelSpec; class ColorSelSpec; class DataCubes; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setOrientation(Orientation)</code> for
    setting the requested orientation of the slice.
*/

class PlaneDataDisplay :  public visBase::VisualObjectImpl,
			  public visSurvey::SurveyObject
{
public:

    bool			isInlCrl() const { return true; }

    enum Orientation		{ Inline=0, Crossline=1, Timeslice=2 };
    				DeclareEnumUtils(Orientation);

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setOrientation(Orientation);
    Orientation			getOrientation() const { return orientation_; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier();
    NotifierAccess*		getMovementNotification()
    				{ return &movefinished_; }

    bool			allowMaterialEdit() const	{ return true; }

    //int				nrResolutions() const;
    //int				getResolution() const;
    //void			setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const;
    bool			canHaveMultipleAttribs() const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);

    bool			hasColorAttribute() const	{ return false;}
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    bool 			isClassification(int attrib) const;
    void			setClassification(int attrib,bool yn);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    const TypeSet<float>*	getHistogram(int) const;
    int				getColTabID(int) const;

    CubeSampling		getCubeSampling(int attrib=-1) const;
    CubeSampling		getCubeSampling(bool manippos,
	    					bool displayspace,
						int attrib=-1) const;
    void			setCubeSampling(CubeSampling);
    bool			setDataVolume(int attrib,
	    				      const Attrib::DataCubes*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
   
    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures(int attrib) const;
    void			selectTexture(int attrib, int texture );
    int				selectedTexture(int attrib) const;

    visBase::GridLines*		gridlines()		{ return gridlines_; }

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					float& val,
	    					BufferString& info) const;

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual bool		allowPicks() const		{ return true; }

    bool			setDataTransform(ZAxisTransform*);

protected:
				~PlaneDataDisplay();

    void			setData(int attrib,const Attrib::DataCubes*);
    void			updateRanges();
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerMotion(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos(const CubeSampling&);
    void			dataTransformCB(CallBacker*);

    CubeSampling		snapCubeSampling(const CubeSampling&) const;

    visBase::DepthTabPlaneDragger*	dragger_;
    visBase::PickStyle*			rectanglepickstyle_;
    visBase::MultiTexture2*		texture_;
    visBase::FaceSet*			rectangle_;
    visBase::GridLines*			gridlines_;
    Orientation				orientation_;


    ObjectSet<Attrib::SelSpec>		as_;
    BoolTypeSet				isclassification_;
    ObjectSet<const Attrib::DataCubes>	cache_;

    BinID			curicstep_;
    float			curzstep_;
    ZAxisTransform*		datatransform_;
    int				datatransformvoihandle_;
    Notifier<PlaneDataDisplay>	moving_;
    Notifier<PlaneDataDisplay>	movefinished_;


    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyTextureRect() { return "Texture rectangle";}
};

};


#endif
