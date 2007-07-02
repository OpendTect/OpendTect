#ifndef vispicksetdisplay_h
#define vispicksetdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispicksetdisplay.h,v 1.59 2007-07-02 12:12:48 cvsraman Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

class SoSeparator;
namespace visBase { class PolyLine; class DrawStyle; }

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
    				~PickSetDisplay();

    void		        createLine();
    void		        showLine(bool);
    bool			lineShown();
    int				usePar(const IOPar&);
    void			setDisplayTransformation(mVisTrans*);
    void			locChg(CallBacker*);
protected:
    visBase::VisualObject*	createLocation() const;
    void			setPosition(int loc,const Pick::Location&);
    Coord3			getPosition(int loc) const;
    ::Sphere			getDirection(int loc) const;
    int				isMarkerClick(const TypeSet<int>&) const;

    visBase::PolyLine*		polyline_;
    visBase::DrawStyle*         drawstyle_;
    SoSeparator*		linesep_;
    bool			needline_;

    void			dispChg(CallBacker*);

    static const char*		sKeyNrPicks();
    static const char*		sKeyPickPrefix();
};

} // namespace visSurvey

#endif
