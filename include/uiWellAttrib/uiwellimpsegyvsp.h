#ifndef uiwellimpsegyvsp_h
#define uiwellimpsegyvsp_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
 RCS:           $Id: uiwellimpsegyvsp.h,v 1.1 2009-01-08 15:47:48 cvsbert Exp $
 _______________________________________________________________________

      -*/


#include "uidialog.h"
class uiSEGYVSPBasicPars;


mClass uiWellImportSEGYVSP : public uiDialog
{
public:
    				uiWellImportSEGYVSP(uiParent*);
				~uiWellImportSEGYVSP();

protected:
    
    uiSEGYVSPBasicPars*		bparsfld_;

};

#endif
