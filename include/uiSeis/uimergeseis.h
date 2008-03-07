#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.9 2008-03-07 12:39:46 cvsbert Exp $
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


class uiMergeSeis : public uiDialog
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
