#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "trigonometry.h"
#include "visdragger.h"
#include "vislocationdisplay.h"
#include "vismarkerset.h"
#include "vispolyline.h"
#include "visrandompos2body.h"


namespace visSurvey
{

/*!\brief Used for displaying picks.

  Picks are positions in 3d (x,y,z) selected by the user by mouseclicks,
  or generated at random. Picks have a constant size in pixels, and can be
  visualized by a number of shapes.
*/

mExpClass(visSurvey) PickSetDisplay : public LocationDisplay
{ mODTextTranslationClass(PickSetDisplay)
public:

				PickSetDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, PickSetDisplay,
				    "PickSetDisplay",
				    ::toUiString(sFactoryKeyword()) )

    void			setSet(Pick::Set*) override;
    bool			isPolygon() const;

    bool			hasColor() const override	{ return true; }
    void			setColor(OD::Color) override;

    void			displayBody(bool);
    bool			isBodyDisplayed() const;

    bool			setBodyDisplay();
    const visBase::RandomPos2Body* getDisplayBody() const
				{ return bodydisplay_.ptr(); }

    void			getPickingMessage(BufferString&) const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setPixelDensity(float) override;
    float			getPixelDensity() const override;

    bool			needLine();
    void			createLine();
    void			redrawLine();
    void			showLine(bool);
    bool			lineShown() const;

    void			showDragger(bool yn);
    bool			draggerShown() const;

    void			redrawAll(int draggeridx=-1);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
				~PickSetDisplay();

    void			setPosition(int loc,const Pick::Location&);
    void			setPosition(int idx,const Pick::Location&,
					    bool add=false) override;
    Coord3			getPosition(int loc) const;
    void			removePosition(int idx) override;
    void			removeAll() override;
    void			redrawMultiSets() override;

    void			setPolylinePos(int,const Coord3&);
    void			removePolylinePos(int);

    ::Quaternion		getDirection(const Pick::Location&) const;

    void			dispChg(CallBacker*) override;
    void			locChg(CallBacker*) override;

    int				clickedMarkerIndex(
				    const visBase::EventInfo&) const override;
    bool			isMarkerClick(
				    const visBase::EventInfo&) const override;

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID&) override;
    void			updateDragger() override;

    void			turnOnSelectionMode(bool) override;
    void			polygonFinishedCB(CallBacker*);
    void			updateSelections(
					    const visBase::PolygonSelection*);
    bool			updateMarkerAtSection(const SurveyObject*,int);
    void			updateLineAtSection();

    bool			removeSelections(TaskRunner*) override;
    bool			draggerNormal() const override;
    void			setDraggerNormal(const Coord3&) override;

    RefMan<visBase::MarkerSet>	markerset_;
    bool			needline_	= false;

    RefMan<visBase::RandomPos2Body> bodydisplay_;
    bool			shoulddisplaybody_	= false;
    RefMan<visBase::Dragger>	dragger_;
    int				draggeridx_	= -1;
    bool			showdragger_	= false;

    static const char*		sKeyNrPicks();
    static const char*		sKeyPickPrefix();
    static const char*		sKeyDisplayBody();

private:

    RefMan<visBase::MarkerSet>	createOneMarker() const;
    void			setPickSelect(int,bool);
    void			unSelectAll();
    void			updateLineStyle();

    OD::Color			color_;		// White
    BoolTypeSet			pickselstatus_;
    RefMan<visBase::PolyLine>	polylines_;
};

} // namespace visSurvey
