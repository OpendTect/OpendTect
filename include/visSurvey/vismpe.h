#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.2 2005-01-20 08:41:35 kristofer Exp $
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
    CubeSampling		getBoxPosition() const;

    bool			getPlanePosition( CubeSampling& ) const;
    void			setPlanePosition( const CubeSampling& );

    void			setSceneEventCatcher( visBase::EventCatcher* );

				//Callbacks from boxdragger
    void			boxDraggerFinishCB( CallBacker* );

    				//Callbacks from rectangle
    void			rectangleMovedCB( CallBacker* );
    void			rectangleStartCB( CallBacker* );
    void			rectangleStopCB( CallBacker* );

    				//Callbacks from user
    void			mouseClickCB( CallBacker* );

    				//Callbacks from MPE
    void			updateDraggerPosition( CallBacker* );
    void			updateBoxPosition( CallBacker* );

    MPE::Engine&		engine;

    visBase::BoxDragger*	boxdragger;

    visBase::DataObjectGroup*		draggerrect;
    visBase::FaceSet*			rectangle;
    visBase::DepthTabPlaneDragger*	dragger;

    visBase::EventCatcher*		sceneeventcatcher;
};

};


#endif
