#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"

#include "uivolprocstepdlg.h"

#include "volprocudfreplacer.h"

class uiGenInput;


namespace VolProc
{


mExpClass(uiVolumeProcessing) uiUdfValReplacer : public uiStepDialog
{ mODTextTranslationClass(uiSmoother)
public:

	mDefaultFactoryInstantiationBase(
		UdfReplacer::sFactoryKeyword(),
		UdfReplacer::sFactoryDisplayName())
		mDefaultFactoryInitClassImpl(uiStepDialog,createInstance)

				uiUdfValReplacer(uiParent*,UdfReplacer*,
						 bool is2d);
				~uiUdfValReplacer();

private:

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);
    virtual bool		acceptOK();

    UdfReplacer*		replacer_;

    uiGenInput*			replvalfld_;
    uiGenInput*			padtrcsfls_;

};

} // namespace VolProc
