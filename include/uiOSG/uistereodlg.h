#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
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

    uiSlider*		sliderfld_;
    ObjectSet<ui3DViewer> vwrs_;

    bool		acceptOK(CallBacker*) override;
    void		sliderMove(CallBacker*);
    void		doFinalize(CallBacker*);
};

