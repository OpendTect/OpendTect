#ifndef uisegyexp_h
#define uisegyexp_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyexp.h,v 1.2 2008-09-23 12:17:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "seistype.h"
class uiSeisSel;
class uiCheckBox;
class uiSeisTransfer;
class uiSEGYFilePars;
class uiSEGYFileSpec;


class uiSEGYExp : public uiDialog
{
public:

			uiSEGYExp(uiParent*,Seis::GeomType);
			~uiSEGYExp();

protected:

    CtxtIOObj&		ctio_;
    Seis::GeomType	geom_;

    uiSeisSel*		seissel_;
    uiSeisTransfer*	transffld_;
    uiSEGYFilePars*	fpfld_;
    uiSEGYFileSpec*	fsfld_;
    uiCheckBox*		morebut_;

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    friend class	uiSEGYExpMore;
    bool		doWork(const IOObj&,const IOObj&,
	    			const char*,const char*);

};


#endif
