#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.6 2002-05-07 08:04:34 kristofer Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"

namespace Geometry { class Pos; };
namespace visBase { class Transformation; };
namespace visSurvey
{

/*!\brief


*/

class SurveyObject 
{
public:
    virtual float		calcDist( const Geometry::Pos& ) const
    					{ return mUndefValue; }

    virtual NotifierAccess*	getMovementNotification() { return 0; }
};


class SurveyParamManager : public CallBackClass
{
public:
    				SurveyParamManager();
				~SurveyParamManager();
    void			setAppVel( float );
    float			getAppVel() const
				{ return displaytransform ? appvel : defappvel;}

    Geometry::Pos		coordDispl2XYT( const Geometry::Pos& ) const;
    Geometry::Pos		coordXYT2Display( const Geometry::Pos& ) const;

    const visBase::Transformation*	getDisplayTransform() const;
    const visBase::Transformation*	getAppvelTransform() const;
    const visBase::Transformation*	getInlCrlTransform() const;

    Notifier<SurveyParamManager>	appvelchange;

protected:
    void			createTransforms();
    static float		defappvel;

    void			removeTransforms(CallBacker*);
    
    float			appvel;
    visBase::Transformation*	displaytransform;
    visBase::Transformation*	appveltransform;
    visBase::Transformation*	inlcrltransform;
};

SurveyParamManager& SPM();

}; // Namespace


#endif
