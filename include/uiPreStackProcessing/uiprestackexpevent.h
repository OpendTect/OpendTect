#ifndef uiprestackeventexport_h
#define uiprestackeventexport_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackexpevent.h,v 1.1 2008-11-28 19:51:53 cvskris Exp $
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


class uiEventExport : public uiDialog
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
