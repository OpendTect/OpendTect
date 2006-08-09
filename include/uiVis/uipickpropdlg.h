#ifndef uipickpropdlg_h
#define uipickpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.h,v 1.2 2006-08-09 10:04:41 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

namespace Pick { class Set; };


class uiPickPropDlg : public uiMarkerStyleDlg
{
public:
			uiPickPropDlg( uiParent* p, Pick::Set& set )
			    : uiMarkerStyleDlg(p,"Pick properties"), set_(set)	{}

protected:

    Pick::Set&		set_;

    void		doFinalise(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
};

#endif
