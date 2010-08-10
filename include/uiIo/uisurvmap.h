#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.21 2010-08-10 06:05:58 cvsraman Exp $
________________________________________________________________________

-*/

#include "uibasemap.h"

class SurveyInfo;
class uiArrowItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;
class SurvBoxObject;
class NorthArrowObject;

mClass uiSurveyBoxObject : public uiBaseMapObject
{
public:
    			uiSurveyBoxObject(SurvBoxObject*);

    const char*		getType() const		{ return "SurveyBox"; }

    void		updateGeometry();
    void		setSurveyInfo(const SurveyInfo&);

protected:

    ObjectSet<uiMarkerItem>	vertices_;
    ObjectSet<uiLineItem>	edges_;
    ObjectSet<uiTextItem>	labels_;

    const SurveyInfo*		survinfo_;

};


mClass uiNorthArrowObject : public uiBaseMapObject
{
public:
    			uiNorthArrowObject(NorthArrowObject*);

    const char*		getType() const		{ return "NorthArrow"; }

    void		updateGeometry();
    void		setSurveyInfo(const SurveyInfo&);

protected:

    uiArrowItem*	arrow_;
    uiLineItem*		angleline_;
    uiTextItem*		anglelabel_;

    const SurveyInfo*	survinfo_;

};


mClass uiSurveyMap : public uiBaseMap
{
public:
			uiSurveyMap(uiParent*,bool withtitle=true);

    void		drawMap(const SurveyInfo*);

protected:

    uiSurveyBoxObject*	survbox_;
    uiNorthArrowObject*	northarrow_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    void		reDraw();
};

#endif
