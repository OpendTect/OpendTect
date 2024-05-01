#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "draw.h"
#include "prestackevents.h"
#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "vismarkerset.h"
#include "visobject.h"
#include "vispolyline.h"
#include "vissurvobj.h"
#include "vistransform.h"

namespace PreStack { class EventManager; class EventSet; }


namespace visSurvey
{

mExpClass(visSurvey) PSEventDisplay : public visBase::VisualObjectImpl
				    , public SurveyObject
{
public:
				PSEventDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, PSEventDisplay,
				    "PSEventDisplay",
				    ::toUiString(sFactoryKeyword()) )

    bool			isInlCrl() const override { return true; }

    void			setEventManager(PreStack::EventManager*);
    void			setHorizonID(int);

    enum MarkerColor		{ Single, Quality, Velocity, VelocityFit };
				mDeclareEnumUtils(MarkerColor);

    void			setMarkerColor(MarkerColor,bool update=true);
    MarkerColor			getMarkerColor() const;
    void			setColTabMapper(const ColTab::MapperSetup&,
						bool update=true);
    const ColTab::MapperSetup&	getColTabMapper() const;
    void			setColTabSequence(const ColTab::Sequence&,
						  bool update=true);
    const ColTab::Sequence*	getColTabSequence(int ch=0) const override;
    void			setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*) override;
    bool			canSetColTabSequence() const override
				{ return true; }
    int				nrAttribs() const override { return 1; }
    const ColTab::MapperSetup* getColTabMapperSetup(int,int) const override;
    void			setPixelDensity(float) override;

    enum DisplayMode		{ ZeroOffset, FullOnSections,
				  ZeroOffsetOnSections, FullOnGathers };
				mDeclareEnumUtils(DisplayMode);
    void			setDisplayMode(DisplayMode);
    DisplayMode			getDisplayMode() const;

    void			setLineStyle(const OD::LineStyle&) override;
    OD::LineStyle		getLineStyle() const;

    void			setMarkerStyle(const MarkerStyle3D&) override;
    const MarkerStyle3D*	markerStyle() const override;
    bool			hasColor() const  override { return true; }
    OD::Color			getColor() const override;
    const char**		markerColorNames() const;
    const char**		displayModeNames() const;
    bool			hasParents() const;
    bool			supportsDisplay() const;

protected:
				~PSEventDisplay();

    void			clearAll();
    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					const VisID& whichobj) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    //bool			filterBinID(const BinID&) const;
				/*!<\returns true if the binid should not be
				     viewed. */

    //RefMan<visBase::PickStyle> pickstyle_;
    void			eventChangeCB(CallBacker*);
    void			eventForceReloadCB(CallBacker*);


    //TypeSet<TrcKeySampling>	sectionranges_;
    struct ParentAttachedObject
    {
					ParentAttachedObject(const VisID&);
					~ParentAttachedObject();

	RefMan<visBase::DataObjectGroup> objectgroup_;
	RefMan<visBase::PolyLine3D>	lines_;

	RefMan<visBase::MarkerSet>	markerset_;
	RefObjectSet<PreStack::EventSet> eventsets_;
	TrcKeySampling			tks_;

	const VisID			parentid_;
    };

    void				updateDisplay();
    void				updateDisplay(ParentAttachedObject*);
    void				clearDisplay(ParentAttachedObject*);
    void				retriveParents();
    float				getMoveoutComp(const TypeSet<float>&,
					const TypeSet<float>&) const;

    RefMan<visBase::DrawStyle>		linestyle_;
    ConstRefMan<mVisTrans>		displaytransform_;
    ObjectSet<ParentAttachedObject>	parentattached_;

    DisplayMode				displaymode_	= ZeroOffsetOnSections;

    PreStack::EventManager*		eventman_	= nullptr;
    int					horid_		= -1;
    Interval<float>			qualityrange_;
    float				offsetscale_	= 1.f;

    MarkerColor				markercolor_	= Single;
    ColTab::Mapper			ctabmapper_;
    ColTab::Sequence			ctabsequence_;
    MarkerStyle3D			markerstyle_;
    RefMan<visBase::MarkerSet>		eventmarkerset_;
    mutable Threads::Lock		lock_;
};

} // namespace visSurvey
