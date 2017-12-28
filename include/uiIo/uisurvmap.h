#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "grid2d.h"
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


mExpClass(uiIo) uiSurveyBoxObject : public uiBaseMapObject
{
public:
			uiSurveyBoxObject(BaseMapObject*);

    const char*		getType() const			{ return "SurveyBox"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo*);
    void		showLabels(bool yn);
    bool		labelsShown() const;

    const OD::LineStyle&	getLineStyle() const	{ return ls_; }
    void			setLineStyle(const OD::LineStyle&);
    void			setVisibility(bool);

protected:

    uiPolygonItem*		frame_;
    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;
    OD::LineStyle			ls_;
    bool			showlabels_;

};


mExpClass(uiIo) uiNorthArrowObject : public uiBaseMapObject
{
public:
			uiNorthArrowObject(BaseMapObject*,bool);

    const char*		getType() const			{ return "NorthArrow"; }

    void		update();
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


mExpClass(uiIo) uiGrid2DMapObject : public uiBaseMapObject
{
public:
			uiGrid2DMapObject();

    const char*		getType();
    void		setGrid(const Grid2D*,const SurveyInfo* si=0);
    void		setBaseLine(const Grid2D::Line*);
    void		setLineStyle(const OD::LineStyle&);
    void		update();

protected:

    ObjectSet<uiLineItem>   lines_;
    const Grid2D::Line*	    baseline_;
    const Grid2D*	    grid_;
    OD::LineStyle	    ls_;
    const SurveyInfo*	    survinfo_;
};


mExpClass(uiIo) uiSurveyMap : public uiBaseMap
{ mODTextTranslationClass(uiSurveyMap)
public:
			uiSurveyMap(uiParent*,bool withtitle=true);

    void		setSurveyInfo(const SurveyInfo*);

    uiSurveyBoxObject*	getSurveyBox() const;

protected:

    uiSurveyBoxObject*	survbox_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    virtual void	reDraw(bool deep=true);
};
