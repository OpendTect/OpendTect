#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.47 2004-05-19 14:59:18 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class ColorAttribSel;
class AttribSliceSet;
class CubeSampling;

namespace visBase { class TextureRect; class VisColorTab; };

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setType(Type)</code> for setting the 
    requested orientation of the slice.
*/

class PlaneDataDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    bool			isInlCrl() const { return true; }

    enum Type			{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setType(Type type);
    Type			getType() const { return type; }

    void			setGeometry(bool manip=false,bool init_=false);
    void			setRanges(const StepInterval<float>&,
					  const StepInterval<float>&,
					  const StepInterval<float>&,
					  bool manip=false);
    				//!< Sets the maximum range in each direction

    void			setOrigo(const Coord3&);
    void			setWidth(const Coord3&);

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;
    
    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier()    { return &moving; }
    BufferString		getManipulationPos() const;

    bool			allowMaterialEdit() const	{ return true; }
    void			setMaterial(visBase::Material*);
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    int				nrResolutions() const;
    BufferString		getResolutionName(int) const;
    int				getResolution() const;
    void			setResolution(int);

    int				getAttributeFormat() const 	{ return 0; }

    bool			hasColorAttribute() const	{ return true; }
    const AttribSelSpec*	getSelSpec() const;
    void			setSelSpec(const AttribSelSpec&);
    void			setColorSelSpec(const ColorAttribSel&);
    const ColorAttribSel*	getColorSelSpec() const;
    const TypeSet<float>*	getHistogram() const;
    int				getColTabID() const;

    CubeSampling		getCubeSampling() const;
    void			setCubeSampling(CubeSampling);
    bool			setDataVolume(bool color,AttribSliceSet*);
    				/*!< Becomes mine */
    const AttribSliceSet*	getCacheVolume(bool color) const;
   
    bool			canHaveMultipleTextures() const {return true;}
    int				nrTextures() const;
    void			selectTexture(int);

    void			turnOn(bool);
    bool			isOn() const;

    float			getValue( const Coord3& ) const;

    SoNode*			getInventorNode();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Coord3& ) const;

protected:
				~PlaneDataDisplay();

    void			setTextureRect(visBase::TextureRect*);
    void			setData(const AttribSliceSet*,int datatype=0);
    void			appVelChCB(CallBacker*);
    void			manipChanged(CallBacker*);
    CubeSampling		getCubeSampling(bool manippos) const;

    visBase::TextureRect*	trect;

    Type			type;
    AttribSelSpec&		as;
    ColorAttribSel&		colas;

    AttribSliceSet*             cache;
    AttribSliceSet*             colcache;

    static const char*		trectstr;
    Notifier<PlaneDataDisplay>	moving;
};

};


#endif
