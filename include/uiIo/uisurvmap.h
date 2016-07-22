#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id$
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


mExpClass(uiIo) uiSurveyBoxObject : public uiBaseMapObject
{
public:
			uiSurveyBoxObject(BaseMapObject*);

    const char*		getType() const			{ return "SurveyBox"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo*);
    void		showLabels(bool yn);
    bool		labelsShown() const;

    const LineStyle&	getLineStyle() const	{ return ls_; }
    void		setLineStyle(const LineStyle&);
    void		setVisibility(bool);

protected:

    uiPolygonItem*		frame_;
    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;
    LineStyle			ls_;
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


mExpClass(uiIo) uiSurveyMap : public uiBaseMap
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

    virtual void	reDraw(bool deep=true);
};

#endif
