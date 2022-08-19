/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisemblancealgo.h"
#include "ptrman.h"

namespace PreStack
{

mImplFactory1Param( uiSemblanceAlgorithm, uiParent*,
		    uiSemblanceAlgorithm::factory);

uiSemblanceAlgorithm::uiSemblanceAlgorithm( uiParent* p, const HelpKey& helpkey)
    : uiDialog( p,
	  uiDialog::Setup(uiStrings::sSetup(),tr("Semblance parameters"),
                          helpkey).canceltext(uiStrings::sEmptyString()))
{}


} // namespace PreStack
