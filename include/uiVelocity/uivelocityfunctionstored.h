#ifndef uivelocityfunctionstored_h
#define uivelocityfunctionstored_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionstored.h,v 1.5 2010/11/10 19:53:12 cvskris Exp $
________________________________________________________________________


-*/

#include "uiselectvelocityfunction.h"
#include "velocityfunctionstored.h"

class uiIOObjSel;
class uiGenInput;

namespace Vel
{
class StoredFunctionSource;

mClass uiStoredFunction : public uiFunctionSettings
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
