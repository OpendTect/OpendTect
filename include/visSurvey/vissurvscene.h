#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.8 2002-03-18 11:01:21 kristofer Exp $
________________________________________________________________________


-*/

#include "visscene.h"

namespace visBase { class Transformation; };

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
    int				getFirstIdx( int id ) const;
    int				getFirstIdx(  const SceneObject* ) const;
    void			removeObject( int idx );

    virtual int			size() const;

    float			apparentVel() const;
    void			setApparentVel( float );

protected:
    				Scene();
    				~Scene();

    visBase::SceneObjectGroup*	xytworld;
    visBase::SceneObjectGroup*	inlcrlworld;
    visBase::Transformation*	xytranslation;
    visBase::Transformation*	timetransformation;
    visBase::Transformation*	inlcrltransformation;

private:
    float			appvel;
};

};


#endif
