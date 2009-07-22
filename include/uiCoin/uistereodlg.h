#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id: uistereodlg.h,v 1.6 2009-07-22 16:01:21 cvsbert Exp $
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
