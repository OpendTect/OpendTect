#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.19 2004-02-23 12:15:45 nanne Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"
#include "position.h"

namespace visBase { class Transformation; };
namespace visSurvey
{

/*!\brief Base class for all 'Display' objects
*/

class SurveyObject 
{
public:
    virtual float		calcDist( const Coord3& ) const
    					{ return mUndefValue; }

    virtual NotifierAccess*	getMovementNotification()   { return 0; }
    virtual bool		isInlCrl() const	    { return false; }

    const char*			errMsg() const		    { return errmsg; }

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

}; // Namespace


/*! \mainpage 3D Visualisation - OpendTect specific

  This module contains front-end classes for displaying 3D objects. Most 
  functions in these classes deal with the geometry or position of the object, 
  as well as handling new data and information about the attribute 
  displayed.

*/


#endif
