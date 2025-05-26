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

private:

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;

    static uiIOObjTranslatorWriteOpts* create(uiParent*);

    uiSEGYFilePars*	parsfld_;

public:
    static void		initClass();

};


mExpClass(uiSEGYTools) uiSEGYDirectPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiSEGYDirectPS3DOpts);
public:

			uiSEGYDirectPS3DOpts(uiParent*);

private:

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;

    static uiIOObjTranslatorWriteOpts* create(uiParent*);

    uiSEGYFilePars*	parsfld_;
    uiGenInput*		nrinlpfilefld_;

public:
    static void		initClass();

};
