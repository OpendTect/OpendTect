#ifndef uivelocityfunctionstored_h
#define uivelocityfunctionstored_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionstored.h,v 1.1 2009-03-18 18:45:26 cvskris Exp $
________________________________________________________________________


-*/

#include "uiselectvelocityfunction.h"

class CtxtIOObj;
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
    CtxtIOObj*			ctxtioobj_;

    StoredFunctionSource*	source_;
};


}; //namespace

#endif
