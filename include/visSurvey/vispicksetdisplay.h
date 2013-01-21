#ifndef vispicksetdisplay_h
#define vispicksetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "vislocationdisplay.h"

class BufferString;
class SoSeparator;
namespace visBase { class PolyLine; class DrawStyle; class RandomPos2Body; }

namespace visSurvey
{

/*!\brief Used for displaying picks.

  Picks are positions in 3d (x,y,z) selected by the user by mouseclicks,
  or generated at random. Picks have a constant size in pixels, and can be
  visualized by a number of shapes.
*/

mExpClass(visSurvey) PickSetDisplay : public LocationDisplay
{
public:
    static PickSetDisplay*	create()
				mCreateDataObj(PickSetDisplay);
    				~PickSetDisplay();

    void			getPickingMessage(BufferString&) const;

    void			displayBody(bool);
    bool			isBodyDisplayed() const;
    bool			setBodyDisplay();
    visBase::RandomPos2Body*	getDisplayBody() const	{ return bodydisplay_; }

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);
    void			setScene(Scene*);

protected:

    visBase::VisualObject*	createLocation() const;
    void			setPosition(int loc,const Pick::Location&);
    Coord3			getPosition(int loc) const;
    ::Sphere			getDirection(int loc) const;
    int				isMarkerClick(const TypeSet<int>&) const;

    void			dispChg(CallBacker*);
    void			locChg(CallBacker*);
    void			setChg(CallBacker*);
    void			sceneZChangeCB(CallBacker*);
    bool			hasColor() const	{ return true; }
    void			setColor(Color);

    static const char*		sKeyNrPicks();
    static const char*		sKeyPickPrefix();
    static const char*		sKeyDisplayBody();

    visBase::RandomPos2Body*	bodydisplay_;
    bool			shoulddisplaybody_;
};

} // namespace visSurvey

#endif

