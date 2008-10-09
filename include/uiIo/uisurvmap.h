#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.15 2008-10-09 17:43:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"

class uiGraphicsScene;
class SurveyInfo;


class uiSurveyMap : public uiGraphicsView
{
public:
			uiSurveyMap(uiParent*);
    void		drawMap(const SurveyInfo*);

protected:
    uiGraphicsScene*	mapscene_;
};

#endif
