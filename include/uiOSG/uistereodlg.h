#ifndef uistereodlg_h
#define uistereodlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uidialog.h"

class uiSlider;
class ui3DViewer;

/*!
\brief Dialog for stereo.
*/

mExpClass(uiOSG) uiStereoDlg : public uiDialog
{ mODTextTranslationClass(uiStereoDlg);
public:
			uiStereoDlg(uiParent*,ObjectSet<ui3DViewer>&);

protected:

    uiSlider*		sliderfld;
    ObjectSet<ui3DViewer> vwrs;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		sliderMove(CallBacker*);

};

#endif

