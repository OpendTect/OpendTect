#ifndef uigraphicsscalebar_h
#define uigraphicsscalebar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigraphicsitem.h"

class uiRectItem;
class uiAdvancedTextItem;
class SurveyInfo;

/*!
\brief It adds a graphic scale made from polygons
*/

mExpClass(uiIo) uiScaleBarItem : public uiGraphicsItem
{
public:
			uiScaleBarItem(int pxwidth=100,int pxheight=6);
			~uiScaleBarItem();

    inline int		getPxHeight() const		{ return pxheight_; }
    inline int		getLength() const		{ return length_; }
    inline int		getPxWidth() const		{ return pxwidth_; }
    inline void		setPxHeight(int pxh)		{ pxheight_ = pxh; }
    inline void		setLength(int len)		{ length_ = len; }
    inline void		setPxWidth(int pxwidth)		{ pxwidth_ = pxwidth; }

    void		update();

protected:
    int			length_;
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
    void		setPolygons(int,int);

};
#endif

