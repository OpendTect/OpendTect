#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
________________________________________________________________________


-*/

#include "vissurveycommon.h"
#include "emposid.h"
#include "dbkey.h"
#include "visemobjdisplay.h"

namespace visBase { class PolyLine3D; class PointSet; class MarkerSet; }
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
				    SurveyObject,Horizon2DDisplay,
				    "Horizon2DDisplay",
				    toUiString(sFactoryKeyword()));
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    void			setDisplayTransformation(const mVisTrans*);

    void			getMousePosInfo(const visBase::EventInfo& e,
						IOPar& i ) const
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
						BufferString& info) const;
    void			setLineStyle(const OD::LineStyle&);

    bool			setEMObject(const DBKey&,TaskRunner*);
    EM::SectionID		getSectionID(int visid) const;
    TypeSet<EM::SectionID>	getSectionIDs() const{ return sids_; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    const visBase::PointSet*	getPointSet(const EM::SectionID&) const;
    const visBase::PolyLine3D*	getLine(const EM::SectionID&) const;
    const Color			getLineColor() const;

    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    virtual void		setPixelDensity(float);
    void			initSelectionDisplay(bool erase);
    virtual void		updateSelections();
    virtual void		clearSelections();


protected:
    friend			class Horizon2DDisplayUpdater;
				~Horizon2DDisplay();
    void			removeSectionDisplay(const EM::SectionID&);
    bool			addSection(const EM::SectionID&,TaskRunner*);

    struct LineRanges
    {
	TypeSet<TypeSet<Interval<int> > >	trcrgs;
	TypeSet<TypeSet<Interval<float> > >	zrgs;
    };

    static bool			withinRanges(const RowCol&,float z,
					     const LineRanges& );
    void			updateSection(int idx,const LineRanges* lr=0);

    void			emChangeCB(CallBacker*);

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			updateLinesOnSections(
					const ObjectSet<const Seis2DDisplay>&);
    void			updateSeedsOnSections(
					const ObjectSet<const Seis2DDisplay>&);

    void			zAxisTransformChg(CallBacker*);
    void			removeVolumesOfInterest();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    bool			calcLine2DIntersections(const GeomIDSet&,
							Line2DInterSectionSet&);

    void			calcLine2DInterSectionSet();

    void			updateIntersectionMarkers(
					const ObjectSet<const Seis2DDisplay>&);
    void			updateIntersectionPoint(const Pos::GeomID,
							const Pos::GeomID,
						const Line2DInterSection*);
    bool			shouldDisplayIntersections(
							const Seis2DDisplay&);

    ObjectSet<visBase::PolyLine3D>		lines_;
    ObjectSet<visBase::PointSet>		points_;
    TypeSet<EM::SectionID>			sids_;
    TypeSet<int>				volumeofinterestids_;
    visBase::MarkerSet*				intersectmkset_;
    bool					updateintsectmarkers_;
    int						nr2dlines_;
    Line2DInterSectionSet*			ln2dset_;
    visBase::PointSet*				selections_;
};

} // namespace visSurvey
