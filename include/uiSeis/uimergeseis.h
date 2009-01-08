#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.10 2009-01-08 08:31:03 cvsranojay Exp $
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
