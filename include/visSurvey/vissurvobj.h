#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.9 2002-10-14 15:10:53 niclas Exp $
________________________________________________________________________


-*/

#include "gendefs.h"
#include "callback.h"
#include "position.h"

namespace Geometry { class Pos; };
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

    Coord3		coordDispl2XYT( const Coord3& ) const;
    Coord3		coordDispl2XYZ( const Coord3& ) const;
    Coord3		coordXYT2Display( const Coord3& ) const;
    Coord3		coordXYT2XYZ( const Coord3& ) const;

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
