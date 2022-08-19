#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

#include "factory.h"


namespace PreStack
{

class SemblanceAlgorithm;

/*! Base class for algorithms that computes semblance along a moveout */
mExpClass(uiPreStackProcessing) uiSemblanceAlgorithm : public uiDialog
{
mODTextTranslationClass(uiSemblanceAlgorithm)
public:
    			mDefineFactory1ParamInClass(uiSemblanceAlgorithm,
						    uiParent*,factory);
    virtual bool	populateUI(const SemblanceAlgorithm*) 		= 0;
    virtual bool	populateObject(SemblanceAlgorithm*) const 	= 0;

protected:
			uiSemblanceAlgorithm(uiParent*,const HelpKey&);
};

} // namespace PreStack
