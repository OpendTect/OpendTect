#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.27 2004-05-10 14:17:39 kristofer Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"
#include "position.h"
#include "ranges.h"
#include "color.h"

class AttribSelSpec;
class ColorAttribSel;
class AttribSliceSet;
class CubeSampling;
class SeisTrc;
class MultiID;

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
    virtual float		maxDist() const		{ return mUndefValue; }
    				/*<\Returns maximum allowed distance between 
				    pick and object. If calcDist() > maxDist()
				    pick will not be displayed. */

    virtual NotifierAccess*	getMovementNotification()   { return 0; }
    virtual NotifierAccess*	rightClickNotifier()	    { return 0; }
    virtual bool		isInlCrl() const	    { return false; }

    const char*			errMsg() const		    { return errmsg; }

    virtual void		getChildren( TypeSet<int>& ) const;

    virtual bool		canDuplicate() const;
    virtual SurveyObject*	duplicate() const;

    virtual const MultiID*	getMultiID() const;

    virtual void		showManipulator(bool yn);
    virtual bool		isManipulatorShown() const;
    virtual bool		isManipulated() const;
    virtual bool		canResetManipulation() const;
    virtual void		resetManipulation();
    virtual void		acceptManipulation();
    virtual BufferString	getManipulationString() const;
    virtual NotifierAccess*	getManipulationNotifier();

    virtual bool		hasColor() const;
    virtual bool		hasMaterial() const;

    virtual void		setColor(Color)		{}
    virtual Color		getColor() const;

    virtual int			nrResolutions() const;
    virtual BufferString	getResolutionName(int) const;
    virtual int			getResolution() const;
    virtual void		setResolution(int);
    
    virtual int			getAttributeFormat() const;
    				/*<\retval 0	volume
				   \retval 1	traces
				   \retval 2	random pos
				   \retval -1	Does not have attribs 
				*/
    virtual bool		hasColorAttribute() const;
    virtual const AttribSelSpec* getSelSpec() const;
    virtual const ColorAttribSel* getColorSelSpec() const;
    virtual const TypeSet<float>* getHistogram() const;
    virtual int			getColTabID() const;

    virtual void		setSelSpec( const AttribSelSpec& );
    virtual void		setColorSelSpec(const ColorAttribSel&);

    virtual bool		canHaveMultipleTextures() const;
    virtual int			nrTextures() const;
    virtual void		selectTexture(int);
    virtual float		getValue(const Coord3&) const;
   
   				//Volume data 
    virtual CubeSampling	getCubeSampling() const;
    virtual void		setCubeSampling(CubeSampling);
    virtual bool		setDataVolume( bool color, AttribSliceSet* slc);
				/*!<\note slc becomes mine */
    virtual const AttribSliceSet* getCacheVolume( bool color ) const;

    				//Trace-data
    virtual void		getDataTraceBids(TypeSet<BinID>&) const;
    virtual Interval<float>	getDataTraceRange() const;
    virtual void		setTraceData( bool color,
	    					      ObjectSet<SeisTrc>* trcs);
    				/*!< trcs becomes mine */

				//Random access
    virtual void		getDataRandomPos( ObjectSet<
			    			TypeSet<BinIDZValues> >&) const;
				/*!< Content of objectset becomes
				     callers */
    virtual void		setRandomPosData(bool color,
	    			    const ObjectSet<const TypeSet<const BinIDZValues> >*);
    				/*!< The data should have exactly the
				     same structure as the positions
				     given in setRandomPosData
				*/


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
