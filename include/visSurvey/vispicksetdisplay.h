#ifndef vissurvpickset_h
#define vissurvpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.7 2002-03-20 08:28:10 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class Color;

namespace visBase { class SceneObjectGroup; };
namespace Geometry { class Pos; }

namespace visSurvey
{

class Scene;

/*!\brief


*/

class PickSet : public visBase::VisualObjectImpl
{
public:
    static PickSet*	create()
			mCreateDataObj0arg(PickSet);

    int			nrPicks() const;
    Geometry::Pos	getPick( int idx ) const;
    void		addPick( const Geometry::Pos& );
    void		removePick( const Geometry::Pos& );
    void		removeAll();

    float		getInlSz() const { return inlsz; }
    float		getCrlSz() const { return inlsz; }
    float		getTSz() const { return inlsz; }

    void		setSize( float inl, float crl, float t );

    Notifier<PickSet>	addedpoint;
    Notifier<PickSet>	removedpoint;

protected:
    		PickSet();
    virtual	~PickSet();

    visBase::SceneObjectGroup*	group;

    float	inlsz;
    float	crlsz;
    float	tsz;
};

};


#endif
