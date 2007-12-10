#ifndef uipickpropdlg_h
#define uipickpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.h,v 1.4 2007-12-10 03:56:57 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

class uiCheckBox;

namespace Pick { class Set; };


class uiPickPropDlg : public uiMarkerStyleDlg
{
public:
			uiPickPropDlg(uiParent* p,Pick::Set& set);

protected:

    uiCheckBox*		linefld_;

    Pick::Set&		set_;

    void		doFinalise(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
    void		connectSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
