#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.4 2002-03-06 07:39:54 kristofer Exp $
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
    				Scene();
    				~Scene();

    int				addXYZObject( SceneObject* );
    int				addXYTObject( SceneObject* );
    int				addInlCrlTObject( SceneObject* );

    virtual int			size() const;
    virtual int			getId(int idx) const;
    virtual SceneObject*	getObject( int id );
    virtual void		removeObject( int id );

    float			apparentVel() const;
    void			settApparentVel() const;

protected:
    visBase::SceneObjectGroup*	xytworld;
    visBase::SceneObjectGroup*	inlcrlworld;
    visBase::Transformation*	xytranslation;
    visBase::Transformation*	timetransformation;
    visBase::Transformation*	inlcrltransformation;

    static int			xytidoffset;
    static int			inlcrloffset;

private:
    float			appvel;
};

};


#endif
