#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.15 2002-04-22 14:41:18 kristofer Exp $
________________________________________________________________________


-*/

#include "visscene.h"
#include "geompos.h"

namespace visBase
{
    class Transformation;
    class Annotation;
    class EventCatcher;
};

namespace visSurvey
{

/*!\brief
The global coordinate system is given in xyz in metres. It is however
convenient to enter horizons in xyt or slices in inl/crl/t. SurveyScene
facilitates objects to be represented in any of the three coordinate system.

Normally, a survey can be thouthands of meters long & wide, but only a few
secs long. In order not to squeeze the display, the time is converted to
metres with a velocity. This velocity is unique for each scene, and can be set
at any time.
*/

class Scene : public visBase::Scene
{
public:
    static Scene*		create()
				mCreateDataObj0arg(Scene);

    void			addXYZObject( SceneObject* );
    void			addXYTObject( SceneObject* );
    void			addInlCrlTObject( SceneObject* );

    float			apparentVel() const;
    void			setApparentVel( float );

    void			showAnnotText( bool yn );
    bool			isAnnotTextShown() const;

    Geometry::Pos		getRealCoord( Geometry::Pos display) const;
    Geometry::Pos		getDisplayCoord( Geometry::Pos real ) const;

    Notifier<Scene>		appvelchange;
    Notifier<Scene>		mouseposchange;
    Geometry::Pos		getMousePos( bool xyt ) const;
    				/*! If ont xyt it is inlcrlt */
    float			getMousePosValue() const { return mouseposval;}

protected:
    				~Scene();
    void			mouseMoveCB( CallBacker* = 0 );

    visBase::Transformation*	timetransformation;
    visBase::Transformation*	inlcrltransformation;
    visBase::Annotation*	annot;
    visBase::EventCatcher*	eventcatcher;

    Geometry::Pos		xytmousepos;
    float			mouseposval;

private:
    float			appvel;
    float			xoffset;
    float			yoffset;

};

};


#endif
