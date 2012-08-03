#ifndef uiprestackagc_h
#define uiprestackagc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackagc.h,v 1.4 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class AGC;
class Processor;

mClass(uiPreStackProcessing) uiAGC : public uiDialog
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

