#ifndef uibasemapscalebar_h
#define uibasemapscalebar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"

#include "uibasemap.h"
#include "draw.h"

class uiLineItem;
class uiTextItem;
class SurveyInfo;

mExpClass(uiBasemap) uiMapScaleObject : public uiBaseMapObject
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

#endif
