#ifndef uiseiseventsnapper_h
#define uiseiseventsnapper_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.h,v 1.3 2006-10-19 11:53:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace EM { class Horizon; }

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

class uiSeisEventSnapper : public uiDialog
{
public:
			uiSeisEventSnapper(uiParent*,const IOObj*);
			~uiSeisEventSnapper();

protected:

    uiIOObjSel*		horinfld_;
    uiIOObjSel*		horoutfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		eventfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		savefld_;

    virtual bool	acceptOK(CallBacker*);
    void		saveSel(CallBacker*);
    bool		readHorizon();
    bool		saveHorizon();

    EM::Horizon*	horizon_;
    CtxtIOObj&		horinctio_;
    CtxtIOObj&		horoutctio_;
    CtxtIOObj&		seisctio_;

};

#endif
