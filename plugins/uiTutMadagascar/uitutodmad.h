#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          June 2009
________________________________________________________________________

-*/

#include "uitutmadagascarmod.h"
#include "uidialog.h"
#include "iopar.h"

class SeisTrcBufDataPack;
class uiFileSel;
class uiGenInput;

/*! \brief Madagascar tutorial plugin interface */

mClass(uiTutMadagascar) uiTutODMad : public uiDialog
{ mODTextTranslationClass(uiTutODMad);
public:

			uiTutODMad(uiParent*);

protected:

    void		createAndDisplay2DViewer();
    bool		acceptOK();

    uiFileSel*		maddatafld_;
    uiGenInput*		dowigglesfld_;
    IOPar		iop_;
    SeisTrcBufDataPack* bufdtpack_;
};
