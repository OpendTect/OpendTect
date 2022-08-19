#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uiioobjselwritetransl.h"
#include "uistring.h"

class uiGenInput;
class uiSEGYFilePars;


mExpClass(uiSEGYTools) uiSEGYDirectVolOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectVolOpts);
public:

			uiSEGYDirectVolOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectVolOpts);

protected:

    uiSEGYFilePars*	parsfld_;

};


mExpClass(uiSEGYTools) uiCBVSPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSPS3DOpts);
public:

			uiCBVSPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSPS3DOpts);

protected:

    uiGenInput*		stortypfld_;

};


mExpClass(uiSEGYTools) uiSEGYDirectPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectPS3DOpts);
public:

			uiSEGYDirectPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectPS3DOpts);

protected:

    uiSEGYFilePars*	parsfld_;
    uiGenInput*		nrinlpfilefld_;

};
