#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.5 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class IOObj;
class IOPar;
class SeisTrc;
class CtxtIOObj;
class uiCheckBox;
class uiIOObjSel;
class SeisRequester;
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

    ObjectSet<MultiID>	ioobjids;
    ObjectSet<IOObj>	selobjs;
    ObjectSet<IOPar>	seliops;
    SeisRequester*	req;
    SeisSingleTraceProc* proc;
    CtxtIOObj&		ctio;
    int			inpsz;
    bool		rev;

    virtual bool	acceptOK(CallBacker*);
    bool		handleInput();
    int			checkRanges();
};

#endif
