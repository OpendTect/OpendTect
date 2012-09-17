#ifndef uiprestackagc_h
#define uiprestackagc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackagc.h,v 1.3 2009/07/22 16:01:22 cvsbert Exp $
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
