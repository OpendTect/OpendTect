#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2015
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


mExpClass(uiSEGYTools) uiSEGYDirectPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectPS3DOpts);
public:

			uiSEGYDirectPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectPS3DOpts);

protected:

    uiSEGYFilePars*	parsfld_;
    uiGenInput*		nrinlpfilefld_;

};
