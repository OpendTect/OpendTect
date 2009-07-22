#ifndef uivelocityfunctionstored_h
#define uivelocityfunctionstored_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionstored.h,v 1.3 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________


-*/

#include "uiselectvelocityfunction.h"

class uiIOObjSel;
class uiGenInput;

namespace Vel
{
class StoredFunctionSource;

mClass uiStoredFunction : public uiFunctionSettings
{
public:
    static void		initClass();

    			uiStoredFunction(uiParent*,StoredFunctionSource*);
    			~uiStoredFunction();

    FunctionSource*	getSource();
    bool		acceptOK();

protected:
    static uiFunctionSettings*
				create(uiParent*,FunctionSource*);

    uiIOObjSel*			funcsel_;
    StoredFunctionSource*	source_;
};


}; //namespace

#endif
