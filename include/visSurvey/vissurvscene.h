#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.17 2002-04-29 10:35:06 nanne Exp $
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

    void			showAnnotText(bool);
    bool			isAnnotTextShown() const;
    void			showAnnotScale(bool);
    bool			isAnnotScaleShown() const;
    void			showAnnot(bool);
    bool			isAnnotShown() const;

    Notifier<Scene>		mouseposchange;
    Geometry::Pos		getMousePos( bool xyt ) const;
    				/*! If ont xyt it is inlcrlt */
    float			getMousePosValue() const { return mouseposval;}

protected:
    				~Scene();
    void			mouseMoveCB( CallBacker* = 0 );

    const visBase::Transformation*	timetransformation;
    const visBase::Transformation*	inlcrltransformation;
    visBase::Annotation*		annot;
    visBase::EventCatcher*		eventcatcher;

    Geometry::Pos		xytmousepos;
    float			mouseposval;
};

};


#endif
