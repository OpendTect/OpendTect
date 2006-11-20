#ifndef uichangesurfacedlg_h
#define uichangesurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2006
 RCS:           $Id: uichangesurfacedlg.h,v 1.1 2006-11-20 16:57:42 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace EM { class Horizon; }

class Executor;
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiArr2DInterpolPars;
template <class T> class Array2D;

/*!\brief Base class for surface changers. At the moment only does horizons. */

class uiChangeSurfaceDlg : public uiDialog
{
public:
				uiChangeSurfaceDlg(uiParent*,EM::Horizon*);
				~uiChangeSurfaceDlg();

protected:

    uiIOObjSel*			inputfld_;
    uiGenInput*			savefld_;
    uiIOObjSel*			outputfld_;
    uiGroup*			parsgrp_;

    EM::Horizon*		horizon_;
    CtxtIOObj*			ctioin_;
    CtxtIOObj*			ctioout_;

    void			saveCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			readHorizon();
    bool			saveHorizon();
    bool			doProcessing();

    void			attachPars();	//!< To be called by subclass
    virtual const char*		infoMsg() const	{ return 0; }
    virtual Executor*		getWorker(Array2D<float>&) = 0;

};

#endif
