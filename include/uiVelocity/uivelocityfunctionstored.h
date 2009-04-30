#ifndef uivelocityfunctionstored_h
#define uivelocityfunctionstored_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocityfunctionstored.h,v 1.2 2009-04-30 14:22:59 cvskris Exp $
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
