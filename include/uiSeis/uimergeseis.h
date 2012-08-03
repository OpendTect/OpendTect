#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.12 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class IOObj;
class IOPar;
class CtxtIOObj;
class uiListBox;
class uiSeisSel;
class uiGenInput;


mClass(uiSeis) uiMergeSeis : public uiDialog
{
public:
                        uiMergeSeis(uiParent*);
			~uiMergeSeis();

protected:

    uiListBox*		inpfld_;
    uiGenInput*		stackfld_;
    uiSeisSel*		outfld_;

    ObjectSet<MultiID>	ioobjids_;
    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    bool		getInput(ObjectSet<IOPar>&,IOPar&);
};

#endif

