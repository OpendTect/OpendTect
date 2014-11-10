#ifndef uiseiswriteopts_h
#define uiseiswriteopts_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
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


mExpClass(uiSeis) uiSEGYDirectVolOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectVolOpts);
public:

			uiSEGYDirectVolOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectVolOpts);

protected:

    uiSEGYFilePars*	parsfld_;

};


mExpClass(uiSeis) uiCBVSPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSPS3DOpts);
public:

			uiCBVSPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSPS3DOpts);

protected:

    uiGenInput*		stortypfld_;

};


mExpClass(uiSeis) uiSEGYDirectPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectPS3DOpts);
public:

			uiSEGYDirectPS3DOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectPS3DOpts);

protected:

    uiSEGYFilePars*	parsfld_;
    uiGenInput*		nrinlpfilefld_;

};


#endif
