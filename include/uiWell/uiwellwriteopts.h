#ifndef uiwellwriteopts_h
#define uiwellwriteopts_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiioobjselwritetransl.h"
class uiGenInput;


mExpClass(uiWell) uiODWellWriteOpts : public uiIOObjTranslatorWriteOpts
{
public:

			uiODWellWriteOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiODWellWriteOpts);

protected:

    uiGenInput*		wrlogbinfld_;
    bool		defbinwrite_;

};


#endif
