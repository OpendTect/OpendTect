#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmadagascarmod.h"
#include "uidialog.h"
#include "iopar.h"

class SeisTrcBufDataPack;
class uiFileInput;
class uiGenInput;

/*! \brief Madagascar tutorial plugin interface */

mClass(uiTutMadagascar) uiTutODMad : public uiDialog
{ mODTextTranslationClass(uiTutODMad);
public:

			uiTutODMad(uiParent*);

protected:

    void		createAndDisplay2DViewer();
    bool		acceptOK(CallBacker*);

    uiFileInput*	maddatafld_;
    uiGenInput*		dowigglesfld_;
    IOPar		iop_;
    RefMan<SeisTrcBufDataPack> bufdtpack_;
};
