#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvobj.h,v 1.1 2002-04-29 09:39:43 kristofer Exp $
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
    virtual float		calcDist( Geometry::Pos& ) const
    					{ return mUndefValue; }
};


class SurveyParamManager : public CallBackClass
{
public:
    				SurveyParamManager();
				~SurveyParamManager();
    void			setAppVel( float );
    float			getAppVel() const { return appvel; }

    Geometry::Pos		coordDispl2XYT( const Geometry::Pos& ) const;
    Geometry::Pos		coordXYT2Display( const Geometry::Pos& ) const;

    const visBase::Transformation*	getDisplayTransform() const;
    const visBase::Transformation*	getAppvelTransform() const;
    const visBase::Transformation*	getInlCrlTransform() const;

    Notifier<SurveyParamManager>	appvelchange;

protected:
    float			appvel;
    visBase::Transformation*	displaytransform;
    visBase::Transformation*	appveltransform;
    visBase::Transformation*	inlcrltransform;
};

SurveyParamManager& SPM();

}; // Namespace


#endif
