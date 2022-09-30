#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiStereoDlg();

protected:

    uiSlider*		sliderfld_;
    ObjectSet<ui3DViewer> vwrs_;

    bool		acceptOK(CallBacker*) override;
    void		sliderMove(CallBacker*);
    void		doFinalize(CallBacker*);
};
