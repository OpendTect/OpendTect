#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.1 2002-02-05 16:34:50 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ctxtioobj.h"
#include "multiid.h"

class SeisRequester;
class SeisTrc;
class UserIDSet;
class uiCheckBox;
class uiIOObjSel;
class uiLabeledListBox;
class SeisSingleTraceProc;


class uiMergeSeis : public uiDialog
{
public:
                        uiMergeSeis(uiParent*);
			~uiMergeSeis();

protected:

    uiLabeledListBox*   seisinpfld;
    uiIOObjSel*		seisoutfld;
    uiCheckBox*		remfld;

    UserIDSet&		ioobjnms;
    ObjectSet<MultiID>	ioobjids;
    ObjectSet<IOObj>	selobjs;
    ObjectSet<IOPar>	seliops;
    SeisRequester*	req;
    SeisSingleTraceProc* proc;
    CtxtIOObj		ctio;
    int			inpsz;
    bool		rev;

    virtual bool	acceptOK(CallBacker*);
    bool		handleInput();
    bool		checkRanges();
};

#endif
