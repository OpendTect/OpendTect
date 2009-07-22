#ifndef uiprestackeventexport_h
#define uiprestackeventexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackexpevent.h,v 1.3 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

class CtxtIOObj;
class uiIOObjSel;
class uiSeisSubSel;
class uiFileInput;
class MultiID;

namespace PreStack
{


mClass uiEventExport : public uiDialog
{
public:
    			uiEventExport(uiParent*, const MultiID*);
    			~uiEventExport();

protected:
    bool		acceptOK(CallBacker*);

    CtxtIOObj*		ctxt_;
    uiIOObjSel*		eventsel_;
    uiSeisSubSel*	subsel_;
    uiFileInput*	outputfile_;
};


}; //namespace

#endif
