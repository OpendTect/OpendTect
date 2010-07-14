#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.19 2010-07-14 06:09:58 cvsraman Exp $
________________________________________________________________________

-*/

#include "uibasemap.h"

class SurveyInfo;
class uiArrowItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;

mClass uiSurveyBoxObject : public uiBaseMapObject
{
public:
    			uiSurveyBoxObject(bool withlabels);

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
    			uiNorthArrowObject(bool withangle);

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
			uiSurveyMap(uiParent*);

    void		drawMap(const SurveyInfo*);

protected:

    uiSurveyBoxObject*	survbox_;
    uiNorthArrowObject*	northarrow_;
    uiTextItem*		title_;

    const SurveyInfo*	survinfo_;

    void		reSizeCB(CallBacker*);
};

#endif
