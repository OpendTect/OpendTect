#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.16 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"

class uiGraphicsScene;
class SurveyInfo;


mClass uiSurveyMap : public uiGraphicsView
{
public:
			uiSurveyMap(uiParent*);
    void		drawMap(const SurveyInfo*);

protected:
    uiGraphicsScene*	mapscene_;
};

#endif
