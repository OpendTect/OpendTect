#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.5 2009-01-08 10:32:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiSliderExtra;
class uiSoViewer;

mClass uiStereoDlg : public uiDialog
{
public:
			uiStereoDlg(uiParent*,ObjectSet<uiSoViewer>&);

protected:

    uiSliderExtra*	sliderfld;
    ObjectSet<uiSoViewer> vwrs;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);

};

#endif
