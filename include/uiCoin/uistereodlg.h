#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiSlider;
class uiSoViewer;

class uiStereoDlg : public uiDialog
{
public:
			uiStereoDlg(uiParent*,ObjectSet<uiSoViewer>&);

protected:

    uiSlider*		sliderfld;
    ObjectSet<uiSoViewer> vwrs;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);

};

#endif
