#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
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

    mDecluiIOObjTranslatorWriteOptsStdFns(uiODWellWriteOpts);

protected:

    uiGenInput*		wrlogbinfld_;
    bool		defbinwrite_;

};
