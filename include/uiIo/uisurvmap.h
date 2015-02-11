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

class SurveyInfo;
class uiArrowItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;
class BaseMapObject;

mExpClass(uiIo) uiSurveyBoxObject : public uiBaseMapObject
{
public:
    			uiSurveyBoxObject(BaseMapObject*,bool);

    const char*		getType() const		{ return "SurveyBox"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo*);

protected:

    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiLineItem>	edges_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;

    void			setVisibility(bool);

};


mExpClass(uiIo) uiNorthArrowObject : public uiBaseMapObject
{
public:
    			uiNorthArrowObject(BaseMapObject*,bool);

    const char*		getType() const { return "NorthArrow"; }

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

    inline const char*	getType() const { return "MapScale"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo*);
    void		setPixelPos(int x,int y);

    inline float	getScaleLen() const { return scalelen_; }
    inline LineStyle&	getScaleStyle() const { return scalestyle_; }

    void		setScaleLen(const float);
    void		setScaleStyle(const LineStyle&);
protected:
    float		scalelen_;
    uiPoint		uistartposition_;

    LineStyle&		scalestyle_;
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

    uiMapScaleObject*	getMapScale()	const { return mapscale_; }
    uiNorthArrowObject* getNorthArrow() const { return northarrow_; }

protected:
    uiMapScaleObject*	mapscale_;
    uiNorthArrowObject*	northarrow_;
    uiSurveyBoxObject*	survbox_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    virtual void	reDraw(bool deep=true);
};

#endif

