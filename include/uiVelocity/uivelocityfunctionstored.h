#ifndef uivelocityfunctionstored_h
#define uivelocityfunctionstored_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionstored.h,v 1.6 2012-08-03 13:01:17 cvskris Exp $
________________________________________________________________________


-*/

#include "uivelocitymod.h"
#include "uiselectvelocityfunction.h"
#include "velocityfunctionstored.h"

class uiIOObjSel;
class uiGenInput;

namespace Vel
{
class StoredFunctionSource;

mClass(uiVelocity) uiStoredFunction : public uiFunctionSettings
{
public:
    mDefaultFactoryInstanciationBase(
	    StoredFunctionSource::sFactoryKeyword(),
	    StoredFunctionSource::sFactoryDisplayName() );


    			uiStoredFunction(uiParent*,StoredFunctionSource*);
    			~uiStoredFunction();

    FunctionSource*	getSource();
    bool		acceptOK();

protected:
    static uiFunctionSettings*	create(uiParent*,FunctionSource*);

    uiIOObjSel*			funcsel_;
    StoredFunctionSource*	source_;
};


}; //namespace

#endif

