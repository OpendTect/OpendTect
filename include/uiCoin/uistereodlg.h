#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.8 2012-08-03 13:00:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "uidialog.h"
class uiSliderExtra;
class ui3DViewer;

mClass(uiCoin) uiStereoDlg : public uiDialog
{
public:
			uiStereoDlg(uiParent*,ObjectSet<ui3DViewer>&);

protected:

    uiSliderExtra*	sliderfld;
    ObjectSet<ui3DViewer> vwrs;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);

};

#endif

