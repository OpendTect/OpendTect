#ifndef uiprestackeventexport_h
#define uiprestackeventexport_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackexpevent.h,v 1.2 2009-01-08 08:56:15 cvsranojay Exp $
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
