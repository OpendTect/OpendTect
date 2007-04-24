#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uimergeseis.h,v 1.8 2007-04-24 16:38:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class IOObj;
class IOPar;
class SeisTrc;
class uiSeisSel;
class CtxtIOObj;
class uiCheckBox;
class uiLabeledListBox;
class SeisSingleTraceProc;


class uiMergeSeis : public uiDialog
{
public:
                        uiMergeSeis(uiParent*);
			~uiMergeSeis();

protected:

    uiLabeledListBox*   seisinpfld;
    uiSeisSel*		seisoutfld;
    uiCheckBox*		remfld;

    ObjectSet<MultiID>	ioobjids;
    ObjectSet<IOObj>	selobjs;
    ObjectSet<IOPar>	seliops;
    CtxtIOObj&		ctio;
    int			inpsz;
    bool		rev;

    virtual bool	acceptOK(CallBacker*);
    bool		handleInput(int&,int&);
    int			checkRanges();
};

#endif
