#ifndef vispicksetdisplay_h
#define vispicksetdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.57 2007-03-06 07:46:20 cvsnanne Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"


namespace visSurvey
{

/*!\brief Used for displaying picks.

  Picks are positions in 3d (x,y,z) selected by the user by mouseclicks,
  or generated at random. Picks have a constant size in pixels, and can be
  visualized by a number of shapes.
*/

class PickSetDisplay :	public LocationDisplay
{
public:
    static PickSetDisplay*	create()
				mCreateDataObj(PickSetDisplay);

    int				usePar(const IOPar&);
protected:
    visBase::VisualObject*	createLocation() const;
    void			setPosition(int loc,const Pick::Location&);
    Coord3			getPosition(int loc) const;
    ::Sphere			getDirection(int loc) const;
    int				isMarkerClick(const TypeSet<int>&) const;

    void			dispChg(CallBacker*);

    static const char*		sKeyNrPicks();
    static const char*		sKeyPickPrefix();
};

} // namespace visSurvey

#endif
