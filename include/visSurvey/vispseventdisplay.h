#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________

-*/

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
				    SurveyObject,PSEventDisplay,
				    "PSEventDisplay",
				     ::toUiString(sFactoryKeyword()) );
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    bool			isInlCrl() const { return true; }

    void			setEventManager(PreStack::EventManager*);
    void			setHorizonID(int);

    enum MarkerColor		{ Single, Quality, Velocity, VelocityFit };
				mDeclareEnumUtils(MarkerColor);

    void			setMarkerColor(MarkerColor,bool update=true);
    MarkerColor			getMarkerColor() const;
    void			setColTabMapper(int,const ColTab::Mapper&,
						TaskRunner*);
    void			setColTabRange(Interval<float>);
    virtual const ColTab::Mapper& getColTabMapper(int) const;
    virtual const ColTab::Sequence& getColTabSequence(int ch=0) const;
    virtual void		setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*);
    virtual bool		canSetColTabSequence() const { return true; }
    virtual int			nrAttribs() const { return 1; }
    virtual void		setPixelDensity(float);

    enum DisplayMode		{ ZeroOffset, FullOnSections,
				  ZeroOffsetOnSections, FullOnGathers };
				mDeclareEnumUtils(DisplayMode);
    void			setDisplayMode(DisplayMode);
    DisplayMode			getDisplayMode() const;

    void			setLineStyle(const OD::LineStyle&);
    OD::LineStyle			getLineStyle() const;

    void			setMarkerStyle(const OD::MarkerStyle3D&);
    void			setMarkerStyle(const OD::MarkerStyle3D&,
					       bool updat);
    virtual bool		hasColor() const { return true; }
    virtual Color		getColor() const;
    const uiStringSet&		markerColorNames()const;
    const uiStringSet&		displayModeNames()const;
    bool			hasParents() const;
    bool			supportsDisplay() const;


protected:
    void			clearAll();
				~PSEventDisplay();
    void			otherObjectsMoved( const ObjectSet<
					const SurveyObject>&, int whichobj );

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			eventChangeCB(CallBacker*);
    void			eventForceReloadCB(CallBacker*);


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
    void				retrieveParents();
    void				ensureDistribSet(const TypeSet<float>&);
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
    ConstRefMan<ColTab::Mapper>		ctabmapper_;
    ConstRefMan<ColTab::Sequence>	ctabsequence_;
    OD::MarkerStyle3D			markerstyle_;
    visBase::MarkerSet*			eventmarkerset_;
    mutable Threads::Lock		lock_;

};

} // namespace visSurvey
