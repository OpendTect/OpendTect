#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiioobjselwritetransl.h"
#include "uistring.h"

class uiGenInput;


mExpClass(uiWell) uiODWellWriteOpts : public uiIOObjTranslatorWriteOpts
{ mODTextTranslationClass(uiODWellWriteOpts);
public:
			uiODWellWriteOpts(uiParent*);
			~uiODWellWriteOpts();

    mDecluiIOObjTranslatorWriteOptsStdFns(uiODWellWriteOpts);

protected:

    uiGenInput*		wrlogbinfld_;
    bool		defbinwrite_;

};
