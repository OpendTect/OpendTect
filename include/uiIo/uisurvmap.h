#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.18 2009-10-16 10:58:27 cvsbert Exp $
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
    void		drawAngleN(float);

};

#endif
