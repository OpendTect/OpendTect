#ifndef uiwellimpsegyvsp_h
#define uiwellimpsegyvsp_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
 RCS:           $Id: uiwellimpsegyvsp.h,v 1.2 2009-01-08 16:20:56 cvsbert Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
#include "iopar.h"
class uiSEGYVSPBasicPars;


mClass uiWellImportSEGYVSP : public uiDialog
{
public:
    				uiWellImportSEGYVSP(uiParent*);
				~uiWellImportSEGYVSP();

protected:

    uiSEGYVSPBasicPars*		bparsfld_;

    IOPar			sgypars_;

    friend class		uiSEGYVSPBasicPars;

};

#endif
