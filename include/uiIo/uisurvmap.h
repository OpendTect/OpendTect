#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.17 2009-07-22 16:01:22 cvsbert Exp $
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
