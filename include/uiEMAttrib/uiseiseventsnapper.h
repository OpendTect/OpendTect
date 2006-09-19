#ifndef uiseiseventsnapper_h
#define uiseiseventsnapper_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.h,v 1.1 2006-09-19 09:28:11 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace EM { class Horizon; }

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

class uiSeisEventSnapper : public uiDialog
{
public:
			uiSeisEventSnapper(uiParent*);
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

    EM::Horizon*	horizon_;
    CtxtIOObj&		horinctio_;
    CtxtIOObj&		horoutctio_;
    CtxtIOObj&		seisctio_;

};

#endif
