#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.12 2002-04-15 10:05:39 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "geompos.h"

class Color;

namespace visBase { class SceneObjectGroup; class EventCatcher; };
namespace Geometry { class Pos; }

namespace visSurvey
{

class Scene;

/*!\brief


*/

class PickSetDisplay : public visBase::VisualObjectImpl
{
public:
    static PickSetDisplay*	create(visSurvey::Scene& scene)
				mCreateDataObj1arg(PickSetDisplay,
					visSurvey::Scene&, scene );

    int				nrPicks() const;
    Geometry::Pos		getPick( int idx ) const;
    void			addPick( const Geometry::Pos& );
    void			removePick( const Geometry::Pos& );
    void			removeAll();

    float			getXSz() const { return xsz; }
    float			getYSz() const { return ysz; }
    float			getZSz() const { return zsz; }

    void			setSize( float inl, float crl, float t );

    Notifier<PickSetDisplay>	changed;

protected:
    virtual		~PickSetDisplay();

    void		pickCB( CallBacker* =0 );
    void		updateCubeSz( CallBacker* =0 );

    float		xsz;
    float		ysz;
    float		zsz;

    int			mousepressid;
    Geometry::Pos	mousepressposition;

    visBase::SceneObjectGroup*	group;
    visBase::EventCatcher*	eventcatcher;
    visSurvey::Scene&		scene;
};

};


#endif
