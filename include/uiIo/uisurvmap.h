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

class BaseMapObject;
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

protected:

    uiPolygonItem*		frame_;
    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;
    LineStyle			ls_;
    bool			showlabels_;

    void			setVisibility(bool);

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


mExpClass(uiIo) uiMapScaleObject : public uiBaseMapObject
{
public:
			uiMapScaleObject(BaseMapObject*);

    inline const char*	getType() const			{ return "MapScale"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo*);
    void		setPixelPos(int x,int y);

    void		setScaleLen(float);
    float		getScaleLen() const		{ return scalelen_; }

    const LineStyle&	getLineStyle() const		{ return ls_; }
    void		setLineStyle(const LineStyle&);

protected:
    float		scalelen_;
    uiPoint		uistartposition_;

    LineStyle		ls_;
    uiLineItem*		scaleline_;
    uiLineItem*		leftcornerline_;
    uiLineItem*		rightcornerline_;
    uiTextItem*		scalelabelorigin_;
    uiTextItem*		scalelabelend_;

    const SurveyInfo*	survinfo_;

    void		setVisibility(bool);

};


mExpClass(uiIo) uiSurveyMap : public uiBaseMap
{
public:
			uiSurveyMap(uiParent*,bool withtitle=true,
				    bool withnortharrow=false,
				    bool withmapscale=false);

    void		setSurveyInfo(const SurveyInfo*);

    uiMapScaleObject*	getMapScale()	const;
    uiGraphicsItem*	getNorthArrow() const;
    uiSurveyBoxObject*	getSurveyBox() const;

protected:
    uiMapScaleObject*	mapscale_;
    uiPixmapItem*	northarrow_;
    uiSurveyBoxObject*	survbox_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    virtual void	reDraw(bool deep=true);
};

#endif

