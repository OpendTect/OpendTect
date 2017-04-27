#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
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


mExpClass(uiSeis) uiSeisBlocksOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSeisBlocksOpts);
public:

			uiSeisBlocksOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSeisBlocksOpts);

protected:

    uiGenInput*		stortypfld_;

};


mExpClass(uiSeis) uiCBVSPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSPS3DOpts);
public:

			uiCBVSPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSPS3DOpts);

protected:

    uiGenInput*		stortypfld_;

};
