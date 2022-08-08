#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
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


}; //namespace

