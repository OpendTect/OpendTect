#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.22 2002-07-31 11:09:40 kristofer Exp $
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

    void			updateRange()	{ setCube(); }
    void			addDisplayObject( SceneObject* );
    void			addXYZObject( SceneObject* );
    void			addXYTObject( SceneObject* );
    void			addInlCrlTObject( SceneObject* );

    virtual void		insertObject( int idx, SceneObject* );
    virtual void		addObject( SceneObject* );
    virtual void		removeObject( int idx );

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

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    void			filterPicks( CallBacker* = 0 );

protected:
    				~Scene();
    void			setCube();
    void			mouseMoveCB( CallBacker* = 0 );
    
    const visBase::Transformation*	displaytransformation;
    const visBase::Transformation*	timetransformation;
    const visBase::Transformation*	inlcrltransformation;
    visBase::Annotation*		annot;
    visBase::EventCatcher*		eventcatcher;

    Geometry::Pos		xytmousepos;
    float			mouseposval;

    static const char*		displobjprefixstr;
    static const char*		nodisplobjstr;
    static const char*		xyzobjprefixstr;
    static const char*		noxyzobjstr;
    static const char*		xytobjprefixstr;
    static const char*		noxytobjstr;
    static const char*		inlcrltobjprefixstr;
    static const char*		noinlcrltobjstr;
    static const char*		annottxtstr;
    static const char*		annotscalestr;
    static const char*		annotcubestr;
};

};


#endif
