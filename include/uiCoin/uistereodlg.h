#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.1 2002-07-15 08:01:00 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiSlider;
class uiSoViewer;

class uiStereoDlg : public uiDialog
{
public:
			uiStereoDlg(uiParent*,uiSoViewer*);

protected:

    uiSlider*		sliderfld;
    uiSoViewer*		vwr;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);

};

#endif
