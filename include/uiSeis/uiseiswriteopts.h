#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiioobjselwritetransl.h"
#include "uistring.h"

class uiCheckBox;
class uiGenInput;
class uiSEGYFilePars;


mExpClass(uiSeis) uiCBVSVolOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSVolOpts);
public:

			uiCBVSVolOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSVolOpts);

protected:

    uiGenInput*		stortypfld_;
    uiCheckBox*		optimdirfld_;

};


mExpClass(uiSeis) uiCBVSPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSPS3DOpts);
public:

			uiCBVSPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSPS3DOpts);

protected:

    uiGenInput*		stortypfld_;

};
