#ifndef uiprestackagc_h
#define uiprestackagc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackagc.h,v 1.1 2007-03-26 21:12:02 cvskris Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class AGC;
class Processor;

class uiAGC : public uiDialog
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
