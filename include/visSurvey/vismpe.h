#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.1 2005-01-06 10:49:17 kristofer Exp $
________________________________________________________________________


-*/

#include "cubesampling.h"
#include "visobject.h"
#include "vissurvobj.h"

class AttribSelSpec;
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class DepthTabPlaneDragger;
    class FaceSet;
    class Transformation;
    class DataObjectGroup;
};

namespace MPE { class Plane; class Engine; };


namespace visSurvey
{

/*!\brief

*/

class MPEDisplay :  public visBase::VisualObjectImpl,
				   public visSurvey::SurveyObject
{
public:

    static MPEDisplay*		create()
				mCreateDataObj(MPEDisplay);
    bool			isInlCrl() const { return true; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

protected:
				~MPEDisplay();
    CubeSampling		getCubePosition() const;
    void			setCubePosition( const CubeSampling& );

    void			setSceneEventCatcher( visBase::EventCatcher* );

				//Callbacks from boxdragger
    void			boxDraggerFinishCB( CallBacker* );

    				//Callbacks from rectangle
    void			rectangleMovedCB( CallBacker* );
    void			rectangleStartCB( CallBacker* );
    void			rectangleStopCB( CallBacker* );

    				//Callbacks from user
    void			mouseClickCB( CallBacker* );

    MPE::Engine&		engine;

    visBase::BoxDragger*	boxdragger;

    visBase::DataObjectGroup*		draggerrect;
    visBase::FaceSet*			rectangle;
    visBase::DepthTabPlaneDragger*	dragger;

    visBase::EventCatcher*		sceneeventcatcher;
};

};


#endif
