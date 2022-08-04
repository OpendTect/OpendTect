#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "vissurvobj.h"

#include "draw.h"
#include "visobject.h"
#include "coltabmapper.h"
#include "coltabsequence.h"

namespace visBase
{
    class PolyLine3D;
    class DrawStyle;
    class DataObjectGroup;
};

namespace PreStack { class EventManager; class EventSet; }

namespace visSurvey
{

mExpClass(visSurvey) PSEventDisplay : public visBase::VisualObjectImpl,
				      public SurveyObject
{
public:
				PSEventDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,PSEventDisplay,
				    "PSEventDisplay",
				     ::toUiString(sFactoryKeyword()) );

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
    void			clearAll();
				~PSEventDisplay();

    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					int whichobj) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    //bool			filterBinID(const BinID&) const;
				/*!<\returns true if the binid should not be
				     viewed. */


    //visBase::PickStyle*		pickstyle_;
    void			eventChangeCB(CallBacker*);
    void			eventForceReloadCB(CallBacker*);


    //TypeSet<TrcKeySampling>	sectionranges_;
    struct ParentAttachedObject
    {
					ParentAttachedObject(int);
					~ParentAttachedObject();
	visBase::DataObjectGroup*	objectgroup_;
	visBase::PolyLine3D*		lines_;

	visBase::MarkerSet*		markerset_;
	ObjectSet<PreStack::EventSet>	eventsets_;
	TrcKeySampling			tks_;

	const int			parentid_;
    };

    void				updateDisplay();
    void				updateDisplay(ParentAttachedObject*);
    void				clearDisplay(ParentAttachedObject*);
    void				retriveParents();
    float				getMoveoutComp(const TypeSet<float>&,
					const TypeSet<float>&) const;

    visBase::DrawStyle*			linestyle_;
    const mVisTrans*			displaytransform_;
    ObjectSet<ParentAttachedObject>	parentattached_;

    DisplayMode				displaymode_;

    PreStack::EventManager*		eventman_;
    int					horid_;
    Interval<float>			qualityrange_;
    float				offsetscale_;

    MarkerColor				markercolor_;
    ColTab::Mapper			ctabmapper_;
    ColTab::Sequence			ctabsequence_;
    MarkerStyle3D			markerstyle_;
    visBase::MarkerSet*			eventmarkerset_;
    mutable Threads::Lock		lock_;
};

} // namespace visSurvey

