#ifndef uipickpropdlg_h
#define uipickpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.h,v 1.3 2007-06-27 10:10:43 cvsraman Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

class uiCheckBox;

namespace Pick { class Set; };


class uiPickPropDlg : public uiMarkerStyleDlg
{
public:
			uiPickPropDlg(uiParent* p,Pick::Set& set, bool line);

    bool		toDraw()	{ return drawline_; }
protected:

    uiCheckBox*		linefld_;
    bool		drawline_;

    Pick::Set&		set_;

    void		doFinalise(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
