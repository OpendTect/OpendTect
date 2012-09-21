#ifndef uiprestackexpevent_h
#define uiprestackexpevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiIOObjSel;
class uiSeisSubSel;
class uiFileInput;
class MultiID;

namespace PreStack
{


mClass(uiPreStackProcessing) uiEventExport : public uiDialog
{
public:
    			uiEventExport(uiParent*, const MultiID*);

protected:
    bool		acceptOK(CallBacker*);

    uiIOObjSel*		eventsel_;
    uiSeisSubSel*	subsel_;
    uiFileInput*	outputfile_;
};


}; //namespace

#endif

