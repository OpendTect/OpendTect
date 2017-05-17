#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsitem.h"
#include "uiworld2ui.h"

class uiAdvancedTextItem;
class uiRectItem;
class SurveyInfo;

/*!
\brief It adds a graphic scale made from polygons
*/

mExpClass(uiTools) uiScaleBarItem : public uiGraphicsItem
{
public:
			uiScaleBarItem(int pxwidth=100,int pxheight=6);
			~uiScaleBarItem();

    inline int		getPxHeight() const	{ return pxheight_; }
    inline float	getLength() const	{ return worldwidth_; }
    inline int		getPxWidth() const	{ return pxwidth_; }
    inline void		setPxHeight(int pxh)	{ pxheight_ = pxh; }
    inline void		setLength(float len)	{ worldwidth_ = len; }
    inline void		setPxWidth(int pxw)	{ pxwidth_ = pxw;
						  preferablepxwidth_ = pxw; }

    void		setWorld2Ui(const uiWorld2Ui&);

    void		update();

protected:
    float		worldwidth_;
    int			pxwidth_;
    int			pxheight_;

    uiRectItem*		upperright_;
    uiRectItem*		uppermid_;
    uiRectItem*		upperleft_;
    uiRectItem*		lowerright_;
    uiRectItem*		lowermid_;
    uiRectItem*		lowerleft_;
    uiAdvancedTextItem*	startnr_;
    uiAdvancedTextItem*	midnr_;
    uiAdvancedTextItem*	stopnr_;

private:
    void		initDefaultScale();
    void		adjustValues();
    void		setPolygons(int,int);
    uiWorld2Ui		w2ui_;
    int			preferablepxwidth_;

};
