#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.9 2002-04-10 07:46:29 kristofer Exp $
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
    static PickSetDisplay*	create()
				mCreateDataObj0arg(PickSetDisplay);

    int				nrPicks() const;
    Geometry::Pos		getPick( int idx ) const;
    void			addPick( const Geometry::Pos& );
    void			removePick( const Geometry::Pos& );
    void			removeAll();

    float			getInlSz() const { return inlsz; }
    float			getCrlSz() const { return inlsz; }
    float			getTSz() const { return inlsz; }

    void			setSize( float inl, float crl, float t );

    Notifier<PickSet>		addedpoint;
    Notifier<PickSet>		removedpoint;

protected:
    virtual	~PickSetDisplay();

    void	pickCB( CallBacker* =0 );

    float	inlsz;
    float	crlsz;
    float	tsz;

    bool	selected;

    int			mousepressid;
    Geometry::Pos	mousepressposition;

    visBase::SceneObjectGroup*	group;
    visBase::EventCatcher*	eventcatcher;
};

};


#endif
