#ifndef uimadiosel_h
#define uimadiosel_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: uimadiosel.h,v 1.1 2007-11-15 12:55:26 cvsbert Exp $
-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "iopar.h"
class uiLabel;
class uiGenInput;
class CtxtIOObj;
class uiSeisSel;
class uiIOObjSel;
class uiFileInput;
class uiSeisSubSel;
class uiSeis2DSubSel;
class uiSeis3DSubSel;


class uiMadIOSel : public uiCompoundParSel
{
public:
			uiMadIOSel(uiParent*,bool isinp);

    void		fillPar( IOPar& iop ) const	{ iop.merge(iop_); }

protected:

    bool		isinp_;
    IOPar		iop_;

    virtual BufferString getSummary() const;
    void		doDlg(CallBacker*);

};


class uiMadIOSelDlg : public uiDialog
{
public:

			uiMadIOSelDlg(uiParent*,IOPar&,bool isinp);
			~uiMadIOSelDlg();

    inline bool		isInp() const
    			{ return subsel3dfld_ || subsel2dfld_; }

protected:

    CtxtIOObj&		ctio3d_;
    CtxtIOObj&		ctio2d_;
    CtxtIOObj&		ctiops_;
    int			idx3d_, idx2d_, idxps_, idxmad_, idxnone_;
    IOPar&		iop_;

    uiGenInput*		typfld_;
    uiSeisSel*		seis3dfld_;
    uiSeisSel*		seis2dfld_;
    uiIOObjSel*		seispsfld_;
    uiFileInput*	madfld_;
    // Inp only:
    uiSeis3DSubSel*	subsel3dfld_;
    uiSeis2DSubSel*	subsel2dfld_;
    uiSeisSubSel*	subselpsfld_;
    uiGenInput*		subselmadfld_;
    uiLabel*		subselmadlbl_;

    void		typSel(CallBacker*);
    void		selChg(CallBacker*);

    bool		getInp();
    bool		acceptOK(CallBacker*);

};


#endif
