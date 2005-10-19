#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.45 2005-10-19 22:03:43 cvskris Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "gendefs.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "color.h"
#include "cubesampling.h"
#include "vissurvscene.h"

class BinIDValueSet;
class LineStyle;
class MultiID;
class SeisTrcBuf;
class ZAxisTransform;

namespace visBase { class Transformation; class EventInfo; };
namespace Attrib  { class SelSpec; class SliceSet; class ColorSelSpec; }

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
    virtual void		snapToTracePos(Coord3&)	{}
    				//<\Snaps coordinate to a trace position

    virtual NotifierAccess*	getMovementNotification()	{ return 0; }
    				/*!<Gives access to a notifier that is triggered
				    when object is moved or modified. */

    virtual void		otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj ) {}
    				/*!< If other objects are moved, removed or
				     added in the scene, this function is
				     called. \note that it only notifies on
				     objects that return something on
				     getMovementNotification().
				     \param whichobj refers to id of the
				     moved object */
    virtual bool		isInlCrl() const	    	{ return false;}

    const char*			errMsg() const		    	{return errmsg;}

    virtual void		getChildren( TypeSet<int>& ) const	{}

    virtual bool		canDuplicate() const	{ return false;}
    virtual SurveyObject*	duplicate() const	{ return 0; }

    virtual MultiID		getMultiID() const	{ return MultiID(-1); }

    virtual void		showManipulator(bool yn)	{}
    virtual bool		isManipulatorShown() const	{ return false;}
    virtual bool		isManipulated() const		{ return false;}
    virtual bool		canResetManipulation() const	{ return false;}
    virtual void		resetManipulation()		{}
    virtual void		acceptManipulation()		{}
    virtual BufferString	getManipulationString() const	{ return ""; }
    virtual NotifierAccess*	getManipulationNotifier()	{ return 0; }

    virtual bool		allowMaterialEdit() const	{ return false;}
    				/*!\note Modification of color should be done
				  	 with setMaterial on
					 visBase::VisualObject */

    virtual const LineStyle*	lineStyle() const { return 0; }
    				/*!<If the linestyle can be set, a non-zero
				    pointer should be return. */
    virtual void		setLineStyle(const LineStyle&) {}
    virtual bool		hasSpecificLineColor() const { return false; }
    				/*!<Specifies wether setLineStyle takes
				    regard to the color of the linestyle. */

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
				   \retval 3	tracking volume
				   \retval -1	Does not have attribs 
				*/
    virtual const Attrib::SelSpec* getSelSpec() const		{ return 0; }
    virtual const Attrib::ColorSelSpec* getColorSelSpec() const	{ return 0; }
    virtual const TypeSet<float>* getHistogram() const		{ return 0; }
    virtual int			getColTabID() const		{ return -1; }

    virtual void		setSelSpec(const Attrib::SelSpec&)	{}
    virtual void		setColorSelSpec(const Attrib::ColorSelSpec&) {}

    virtual bool		canHaveMultipleTextures() const { return false;}
    virtual int			nrTextures() const		{ return 0; }
    virtual void		selectTexture(int)			{}
    virtual void		selectNextTexture(bool)			{}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    const Coord3& xyzpos, float& val,
					    BufferString& info) const
				{ val = mUndefValue; info = ""; }
   
   				//Volume data 
    virtual CubeSampling	getCubeSampling() const
				{ CubeSampling cs; return cs; }
    virtual void		setCubeSampling(CubeSampling)		{}
    virtual bool		setDataVolume(bool color,Attrib::SliceSet* slc);
				/*!<\note slc becomes mine */
    virtual const Attrib::SliceSet* getCacheVolume( bool color ) const
				{ return 0; }

    				//Trace-data
    virtual void		getDataTraceBids(TypeSet<BinID>&) const	{}
    virtual Interval<float>	getDataTraceRange() const
    				{ return Interval<float>(0,0); }
    virtual void		setTraceData(bool forcolor,SeisTrcBuf&);

				// Link to outside world
    virtual void		fetchData(ObjectSet<BinIDValueSet>&) const {}
				/*!< Content of objectset becomes callers.
				     Every patch is put in a BinIDValueSet.
				     The first value int the bidset is the
				     depth, the eventual second value is the
				     cached value */
    virtual void		stuffData(bool forcolordata,
	    				  const ObjectSet<BinIDValueSet>*) {}
    				/*!< Every patch should have a BinIDValueSet */
    virtual void		readAuxData()	{}

    void			setScene(Scene*);

    virtual bool		setDataTransform( ZAxisTransform* );

    static float		sDefMaxDist;

protected:
    				SurveyObject() : scene_(0)	{}

    BufferString		errmsg;
    Scene*			scene_;

    virtual void		setUpConnections()		{}
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

/*
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
*/


}; // Namespace visSurvey


/*! \mainpage 3D Visualisation - OpendTect specific

  This module contains front-end classes for displaying 3D objects. Most 
  functions in these classes deal with the geometry or position of the object, 
  as well as handling new data and information about the attribute 
  displayed.

*/


#endif
