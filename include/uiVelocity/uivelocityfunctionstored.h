#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "uiselectvelocityfunction.h"
#include "velocityfunctionstored.h"

class uiIOObjSel;

namespace Vel
{

class StoredFunctionSource;

mExpClass(uiVelocity) uiStoredFunction : public uiFunctionSettings
{ mODTextTranslationClass(uiStoredFunction);
public:
    mDefaultFactoryInstanciationBase(
	    StoredFunctionSource::sFactoryKeyword(),
	    StoredFunctionSource::sFactoryDisplayName() );


    			uiStoredFunction(uiParent*,StoredFunctionSource*);
    			~uiStoredFunction();

    FunctionSource*	getSource() override;
    bool		acceptOK() override;

protected:
    static uiFunctionSettings*	create(uiParent*,FunctionSource*);

    uiIOObjSel*			funcsel_;
    StoredFunctionSource*	source_;
};

} // namespace Vel
