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

namespace visBase { class MarkerSet; class PolyLine;
		    class DrawStyle; class RandomPos2Body; }

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

				PickSetDisplay();
				mDefaultFactoryInstantiation( 
				    visSurvey::SurveyObject,PickSetDisplay,
				    "PickSetDisplay", sFactoryKeyword() );
    
    void			setSet(Pick::Set*);

    bool			hasColor() const	{ return true; }
    void			setColor(Color);

    void			displayBody(bool);
    bool			isBodyDisplayed() const;

    bool			setBodyDisplay();
    visBase::RandomPos2Body*	getDisplayBody() const	{ return bodydisplay_; }

    void			getPickingMessage(BufferString&) const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setPixelDensity(float);
    float			getPixelDensity() const;

    bool			needLine();
    void                        createLine();
    void			redrawLine();
    void                        showLine(bool);
    bool                        lineShown() const;

    void			redrawAll();
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
protected:
				~PickSetDisplay();

    void			setPosition(int loc,const Pick::Location&);
    Coord3			getPosition(int loc) const;
    void			removePosition(int idx);
    void			removeAll();

    void			setPolylinePos(int,const Coord3&);
    void			removePolylinePos(int);
    
    

    ::Sphere			getDirection(int loc) const;

    void			dispChg(CallBacker*);
    void			locChg(CallBacker*);

    int				clickedMarkerIndex(
					   const visBase::EventInfo& evi) const;
    bool			isMarkerClick(
					   const visBase::EventInfo& evi) const;

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    
    visBase::MarkerSet*		markerset_;
    visBase::PolyLine*          polyline_;
    bool			needline_;

    static const char*		sKeyNrPicks();
    static const char*		sKeyPickPrefix();
    static const char*		sKeyDisplayBody();

    visBase::RandomPos2Body*	bodydisplay_;
    bool			shoulddisplaybody_;
};

} // namespace visSurvey

#endif

