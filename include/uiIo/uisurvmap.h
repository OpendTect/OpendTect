#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.25 2012-08-03 13:01:02 cvskris Exp $
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

mClass(uiIo) uiSurveyBoxObject : public uiBaseMapObject
{
public:
    			uiSurveyBoxObject(BaseMapObject*,bool);

    const char*		getType() const		{ return "SurveyBox"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo&);

protected:

    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiLineItem>	edges_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;

};


mClass(uiIo) uiNorthArrowObject : public uiBaseMapObject
{
public:
    			uiNorthArrowObject(BaseMapObject*,bool);

    const char*		getType() const		{ return "NorthArrow"; }

    void		update();
    void		setSurveyInfo(const SurveyInfo&);

protected:

    uiArrowItem*	arrow_;
    uiLineItem*		angleline_;
    uiTextItem*		anglelabel_;

    const SurveyInfo*	survinfo_;

};


mClass(uiIo) uiSurveyMap : public uiBaseMap
{
public:
			uiSurveyMap(uiParent*,bool withtitle=true);

    void		drawMap(const SurveyInfo*);

protected:

    uiSurveyBoxObject*	survbox_;
    uiNorthArrowObject*	northarrow_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    virtual void	reDraw(bool deep=true);
};

#endif

