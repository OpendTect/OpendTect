#ifndef uiseiseventsnapper_h
#define uiseiseventsnapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.h,v 1.8 2009-11-12 21:34:40 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace EM { class Horizon3D; }

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiHorSaveFieldGrp;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

mClass uiSeisEventSnapper : public uiDialog
{
public:
			uiSeisEventSnapper(uiParent*,const IOObj*);
			~uiSeisEventSnapper();
    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }	

protected:

    uiHorSaveFieldGrp*	savefldgrp_;
    uiIOObjSel*		horinfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		eventfld_;
    uiGenInput*		gatefld_;

    virtual bool	acceptOK(CallBacker*);
    bool		readHorizon();

    EM::Horizon3D*	horizon_;
    CtxtIOObj&		horinctio_;
    CtxtIOObj&		seisctio_;
};

#endif
