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
			~uiCBVSVolOpts();

private:

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;

    static uiIOObjTranslatorWriteOpts* create(uiParent*);

    uiGenInput*		stortypfld_;
    uiCheckBox*		optimdirfld_;

public:
    static void		initClass();

};


mExpClass(uiSeis) uiCBVSPS3DOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiCBVSPS3DOpts);
public:
			uiCBVSPS3DOpts(uiParent*);
			~uiCBVSPS3DOpts();

private:

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;

    static uiIOObjTranslatorWriteOpts* create(uiParent*);

    uiGenInput*		stortypfld_;

public:
    static void		initClass();

};
