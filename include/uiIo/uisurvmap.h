#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uibasemap.h"
#include "draw.h"

class uiArrowItem;
class uiLineItem;
class uiMarkerItem;
class uiGraphicsItem;
class uiPixmapItem;
class uiPolygonItem;
class uiTextItem;
class SurveyInfo;


mExpClass(uiIo) uiSurveyBoxObject : public uiBasemapObject
{
public:
			uiSurveyBoxObject(BasemapObject*);
			~uiSurveyBoxObject();

    const char*		getType() const			{ return "SurveyBox"; }

    void		update() override;
    void		setSurveyInfo(const SurveyInfo*);

    void		showLabels(bool yn);
    bool		labelsShown() const;

    void		setAsWorkArea(bool yn);
    bool		asWorkArea() const;

    const OD::LineStyle& getLineStyle() const	{ return ls_; }
    void		setLineStyle(const OD::LineStyle&);
    void		setVisibility(bool);

protected:

    uiPolygonItem*		frame_;
    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;
    OD::LineStyle		ls_;
    bool			showlabels_;
    bool			asworkarea_;
};


mExpClass(uiIo) uiNorthArrowObject : public uiBasemapObject
{
public:
			uiNorthArrowObject(BasemapObject*,bool);

    const char*		getType() const			{ return "NorthArrow"; }

    void		update() override;
    void		setSurveyInfo(const SurveyInfo*);
    void		setPixelPos(int x,int y);

protected:
    uiPoint		uistartposition_;

    uiArrowItem*	arrow_;
    uiLineItem*		angleline_;
    uiTextItem*		anglelabel_;

    const SurveyInfo*	survinfo_;

    void		setVisibility(bool);

};


mExpClass(uiIo) uiSurveyMap : public uiBasemap
{
public:
			uiSurveyMap(uiParent*,bool withtitle=true);

    void		setSurveyInfo(const SurveyInfo*);

    uiSurveyBoxObject*	getSurveyBox() const;

    static SurveyInfo*	getEmptySurvInfo();

protected:
    uiSurveyBoxObject*	survbox_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    void		reDraw(bool deep=true) override;
};
