#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.13 2003-08-22 11:32:31 nanne Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"
#include "position.h"

namespace visBase { class Transformation; };
namespace visSurvey
{

/*!\brief


*/

class SurveyObject 
{
public:
    virtual float		calcDist( const Coord3& ) const
    					{ return mUndefValue; }

    virtual NotifierAccess*	getMovementNotification()   { return 0; }

    float			zFactor() const;

    const char*			errMsg() const		    { return errmsg; }

protected:
    BufferString		errmsg;
};


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

protected:
    void			createTransforms();
    static float		defzscale;

    void			removeTransforms(CallBacker*);
    
    float			zscale;
    float			zfactor;
    visBase::Transformation*	utm2displaytransform;
    visBase::Transformation*	zscaletransform;
    visBase::Transformation*	inlcrl2displaytransform;
};

SurveyParamManager& SPM();

}; // Namespace


#endif
