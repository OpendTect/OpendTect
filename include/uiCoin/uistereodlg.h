#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.2 2002-08-20 07:42:10 nanne Exp $
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
