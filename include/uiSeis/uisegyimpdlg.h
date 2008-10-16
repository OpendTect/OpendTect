#ifndef uisegyimpdlg_h
#define uisegyimpdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.h,v 1.9 2008-10-16 16:31:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyreaddlg.h"
class uiSeisSel;
class CtxtIOObj;
class uiCheckBox;
class uiSeisTransfer;


/*!\brief Dialog to import SEG-Y files after basic setup. */

class uiSEGYImpDlg : public uiSEGYReadDlg
{
public :

			uiSEGYImpDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYImpDlg();

    void		use(const IOObj*,bool force);


protected:

    CtxtIOObj&		ctio_;

    uiSeisTransfer*	transffld_;
    uiSeisSel*		seissel_;
    uiCheckBox*		morebut_;

    virtual bool	doWork(const IOObj&);

    friend class	uiSEGYImpSimilarDlg;
    bool		impFile(const IOObj&,const IOObj&,
	    			const char*,const char*);

};


#endif
