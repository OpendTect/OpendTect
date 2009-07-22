#ifndef uiseiseventsnapper_h
#define uiseiseventsnapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.h,v 1.6 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace EM { class Horizon3D; }

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

mClass uiSeisEventSnapper : public uiDialog
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

    EM::Horizon3D*	horizon_;
    CtxtIOObj&		horinctio_;
    CtxtIOObj&		horoutctio_;
    CtxtIOObj&		seisctio_;

};

#endif
