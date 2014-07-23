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
class uiGenInput;
class uiSEGYFilePars;


mExpClass(uiSeis) uiCBVSVolOpts : public uiIOObjTranslatorWriteOpts
{
public:

			uiCBVSVolOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiCBVSVolOpts);

protected:

    uiGenInput*		stortypfld_;
    uiGenInput*		optimdirfld_;

};


mExpClass(uiSeis) uiSEGYDirectVolOpts : public uiIOObjTranslatorWriteOpts
{
public:

			uiSEGYDirectVolOpts(uiParent*);

    mDecluiIOObjTranslatorWriteOptsStdFns(uiSEGYDirectVolOpts);

protected:

    uiSEGYFilePars*	parsfld_;

};


#endif
