#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.31 2004-09-17 15:13:50 nanne Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"
#include "position.h"
#include "ranges.h"
#include "color.h"
#include "cubesampling.h"

class AttribSelSpec;
class AttribSliceSet;
class BinIDValueSet;
class ColorAttribSel;
class MultiID;
class SeisTrcBuf;

namespace visBase { class Transformation; };

namespace visSurvey
{

/*!\brief Base class for all 'Display' objects
*/

class SurveyObject 
{
public:
    virtual float		calcDist(const Coord3&) const
				{ return mUndefValue; }
    				/*<\Calculates distance between pick and 
				    object*/
    virtual float		maxDist() const		{ return sDefMaxDist; }
    				/*<\Returns maximum allowed distance between 
				    pick and object. If calcDist() > maxDist()
				    pick will not be displayed. */
    virtual bool		allowPicks() const	{ return false; }
    				/*<\Returns whether picks can be created 
				    on object. */

    virtual NotifierAccess*	getMovementNotification()	{ return 0; }
    virtual bool		isInlCrl() const	    	{ return false;}

    const char*			errMsg() const		    	{return errmsg;}

    virtual void		getChildren( TypeSet<int>& ) const	{}

    virtual bool		canDuplicate() const		{ return false;}
    virtual SurveyObject*	duplicate() const		{ return 0; }

    virtual const MultiID*	getMultiID() const		{ return 0; }

    virtual void		showManipulator(bool yn)	{}
    virtual bool		isManipulatorShown() const	{ return false;}
    virtual bool		isManipulated() const		{ return false;}
    virtual bool		canResetManipulation() const	{ return false;}
    virtual void		resetManipulation()		{}
    virtual void		acceptManipulation()		{}
    virtual BufferString	getManipulationString() const	{ return ""; }
    virtual NotifierAccess*	getManipulationNotifier()	{ return 0; }

    virtual bool		allowMaterialEdit() const	{ return false;}
    				/*!\note Mofication of color should be done
				  	 with setMaterial on
					 visBase::VisualObject */

    virtual bool		hasColor() const		{ return false;}
    virtual void		setColor(Color)			{}
    virtual Color		getColor() const		
    				{ return Color::DgbColor; }

    virtual int			nrResolutions() const		{ return 1; }
    virtual BufferString	getResolutionName(int) const;
    virtual int			getResolution() const		{ return 0; }
    virtual void		setResolution(int)		{}
    
    virtual int			getAttributeFormat() const	{ return -1; }
    				/*<\retval 0	volume
				   \retval 1	traces
				   \retval 2	random pos
				   \retval -1	Does not have attribs 
				*/
    virtual const AttribSelSpec* getSelSpec() const		{ return 0; }
    virtual const ColorAttribSel* getColorSelSpec() const	{ return 0; }
    virtual const TypeSet<float>* getHistogram() const		{ return 0; }
    virtual int			getColTabID() const		{ return -1; }

    virtual void		setSelSpec(const AttribSelSpec&)	{}
    virtual void		setColorSelSpec(const ColorAttribSel&)	{}

    virtual bool		canHaveMultipleTextures() const { return false;}
    virtual int			nrTextures() const		{ return 0; }
    virtual void		selectTexture(int)			{}
    virtual float		getValue(const Coord3&) const
				{ return mUndefValue; }
   
   				//Volume data 
    virtual CubeSampling	getCubeSampling() const
				{ CubeSampling cs; return cs; }
    virtual void		setCubeSampling(CubeSampling)		{}
    virtual bool		setDataVolume(bool color,AttribSliceSet* slc);
				/*!<\note slc becomes mine */
    virtual const AttribSliceSet* getCacheVolume( bool color ) const
				{ return 0; }

    				//Trace-data
    virtual void		getDataTraceBids(TypeSet<BinID>&) const	{}
    virtual Interval<float>	getDataTraceRange() const
    				{ return Interval<float>(0,0); }
    virtual void		setTraceData(bool forcolor,SeisTrcBuf&);

				// Link to outside world
    virtual void		fetchData(ObjectSet<BinIDValueSet>&) const {}
				/*!< Content of objectset becomes callers.
				     Every patch is put in a BinIDValueSet */
    virtual void		stuffData(bool forcolordata,
	    				  const ObjectSet<BinIDValueSet>*) {}
    				/*!< Every patch should have a BinIDValueSet */

    static float		sDefMaxDist;

protected:
    BufferString		errmsg;
};


/*!\brief Manager for survey parameters and transformations

  Manages the Z-scale of the survey, and the transformations between different
  coordinate systems.<br>
  inlcrl2displaytransform: transforms between inline/crossline and the
  internal coordinate system.<br>
  utm2displaytransform: transforms between x/y and the internal coordinate 
  system.<br>
  For more information, look at the class documentation of visSurvey::Scene.
  
*/

class SurveyParamManager : public CallBackClass
{
public:
    				SurveyParamManager();
				~SurveyParamManager();
    void			setZScale( float );
    float			getZScale() const;

    visBase::Transformation*	getUTM2DisplayTransform();
    visBase::Transformation*	getZScaleTransform();
    visBase::Transformation*	getInlCrl2DisplayTransform();

    Notifier<SurveyParamManager>	zscalechange;
    static const char*		zscalestr;

protected:
    void			createTransforms();
    static float		defzscale;

    void			removeTransforms(CallBacker*);
    
    float			zscale;
    visBase::Transformation*	utm2displaytransform;
    visBase::Transformation*	zscaletransform;
    visBase::Transformation*	inlcrl2displaytransform;
};

SurveyParamManager& SPM();

}; // Namespace visSurvey


/*! \mainpage 3D Visualisation - OpendTect specific

  This module contains front-end classes for displaying 3D objects. Most 
  functions in these classes deal with the geometry or position of the object, 
  as well as handling new data and information about the attribute 
  displayed.

*/


#endif
