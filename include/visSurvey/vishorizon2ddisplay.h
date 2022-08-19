#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "emposid.h"
#include "multiid.h"
#include "visemobjdisplay.h"
#include "geom2dintersections.h"

namespace visBase { class PolyLine3D; class PointSet;  class MarkerSet; }
namespace EM { class Horizon2D; }
class ZAxisTransform;
class Line2DInterSection;
class Line2DInterSectionSet;

namespace visSurvey
{

class Seis2DDisplay;

mExpClass(visSurvey) Horizon2DDisplay : public EMObjectDisplay
{ mODTextTranslationClass(Horizon2DDisplay);
public:
				Horizon2DDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,Horizon2DDisplay,
				    "Horizon2DDisplay",
				    toUiString(sFactoryKeyword()));

    void			setDisplayTransformation(
						const mVisTrans*) override;

    void			getMousePosInfo( const visBase::EventInfo& e,
						 IOPar& i ) const override
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    Coord3&,BufferString& val,
					    BufferString& info) const override;
    void			setLineStyle(const OD::LineStyle&) override;

    bool			setEMObject(const EM::ObjectID&,
					    TaskRunner*) override;
    EM::SectionID		getSectionID(VisID visid) const override;
    TypeSet<EM::SectionID>	getSectionIDs() const	{ return sids_; }

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const OD::Color		getLineColor() const;

    const visBase::PointSet*	getPointSet(const EM::SectionID&) const;
    const visBase::PolyLine3D*	getLine(const EM::SectionID&) const;
    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj) override;
    void			setPixelDensity(float) override;
    void			initSelectionDisplay(bool erase);
    void			updateSelectionsHor2D();
    void			clearSelectionsHor2D();

    Coord3			getTranslation() const override;
    void			setTranslation(const Coord3&) override;

protected:
    friend			class Horizon2DDisplayUpdater;
				~Horizon2DDisplay();

    void			removeSectionDisplay(
						const EM::SectionID&) override;
    bool			addSection(const EM::SectionID&,
					   TaskRunner*) override;

    struct LineRanges
    {
	TypeSet<TypeSet<Interval<int> > >	trcrgs;
	TypeSet<TypeSet<Interval<float> > >	zrgs;
    };

    static bool			withinRanges(const RowCol&,float z,
					     const LineRanges& );
    void			updateSection(int idx,const LineRanges* lr=0);

    void			emChangeCB(CallBacker*) override;

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj) override;
    void			updateLinesOnSections(
					const ObjectSet<const Seis2DDisplay>&);
    void			updateSeedsOnSections(
					const ObjectSet<const Seis2DDisplay>&);

    void			zAxisTransformChg(CallBacker*);
    void			removeVolumesOfInterest();

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			calcLine2DIntersections(
						const TypeSet<Pos::GeomID>&,
						    Line2DInterSectionSet&);

    void			calcLine2DInterSectionSet();

    void			updateIntersectionMarkers(
					const ObjectSet<const Seis2DDisplay>&);
    void			updateIntersectionPoint(const Pos::GeomID,
							const Pos::GeomID,
						const Line2DInterSection*);
    bool			shouldDisplayIntersections(
							const Seis2DDisplay&);

    ObjectSet<visBase::PolyLine3D>	lines_;
    ObjectSet<visBase::PointSet>	points_;
    TypeSet<EM::SectionID>		sids_;
    TypeSet<int>			volumeofinterestids_;
    visBase::MarkerSet*			intersectmkset_;
    bool				updateintsectmarkers_;
    int					nr2dlines_;
    Line2DInterSectionSet*		ln2dset_;
    visBase::PointSet*			selections_;

    mVisTrans*				translation_;
    Coord3				translationpos_		= Coord3::udf();
};

} // namespace visSurvey
