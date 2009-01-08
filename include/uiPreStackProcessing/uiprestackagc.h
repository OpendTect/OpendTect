#ifndef uiprestackagc_h
#define uiprestackagc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackagc.h,v 1.2 2009-01-08 08:56:15 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class AGC;
class Processor;

mClass uiAGC : public uiDialog
{
public:
    static void		initClass();
			uiAGC(uiParent*,AGC*);

protected:
    bool		acceptOK(CallBacker*);
    static uiDialog*	create(uiParent*,Processor*);

    AGC*		processor_;
    uiGenInput*		windowfld_;
    uiGenInput*		lowenergymute_;
};


}; //namespace

#endif
