#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.11 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class IOObj;
class IOPar;
class CtxtIOObj;
class uiListBox;
class uiSeisSel;
class uiGenInput;


mClass uiMergeSeis : public uiDialog
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
