#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.66 2006-01-24 06:32:32 cvskris Exp $
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
    Orientation			getOrientation() const { return orientation; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier();
    NotifierAccess*		getMovementNotification() {return &moving;}

    bool			allowMaterialEdit() const	{ return true; }

    //int				nrResolutions() const;
    //int				getResolution() const;
    //void			setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const;
    bool			canHaveMultipleAttribs() const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			removeAttrib( int attrib );
    bool			swapAttribs( int attrib0, int attrib1 );

    bool			hasColorAttribute() const	{ return false;}
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    const TypeSet<float>*	getHistogram(int) const;
    int				getColTabID(int) const;

    CubeSampling		getCubeSampling() const;
    CubeSampling		getCubeSampling(bool manippos,
	    					bool displayspace ) const;
    void			setCubeSampling(CubeSampling);
    bool			setDataVolume( int attrib,
	    				       const Attrib::DataCubes* );
    const Attrib::DataCubes*	getCacheVolume( int attrib ) const;
   
    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures( int attrib ) const;
    void			selectTexture(int attrib, int texture );
    int				selectedTexture(int attrib) const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					float& val,
	    					BufferString& info) const;

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Coord3& ) const;
    virtual float		maxDist() const;
    virtual bool		allowPicks() const		{ return true; }

    bool			setDataTransform( ZAxisTransform* );

protected:
				~PlaneDataDisplay();

    void			setData( int attrib, const Attrib::DataCubes* );
    void			updateRanges();
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos( const CubeSampling& );

    CubeSampling		snapCubeSampling( const CubeSampling& ) const;

    visBase::DepthTabPlaneDragger*	dragger;
    visBase::PickStyle*			rectanglepickstyle;
    visBase::MultiTexture2*		texture;
    visBase::FaceSet*			rectangle;
    Orientation				orientation;


    ObjectSet<Attrib::SelSpec>		as;
    ObjectSet<const Attrib::DataCubes>	cache;

    BinID			curicstep;
    float			curzstep;
    ZAxisTransform*		datatransform;
    int				datatransformvoihandle;


    static const char*		sKeyNrAttribs() { return "Nr Attribs"; }
    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyAttribs() { return "Attrib "; }
    static const char*		sKeyColTabID() { return "Colortable ID"; }
    static const char*		sKeyTextureRect() { return "Texture rectangle";}
    Notifier<PlaneDataDisplay>	moving;
};

};


#endif
