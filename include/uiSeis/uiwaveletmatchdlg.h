#ifndef uiwaveletmatchdlg_h
#define uiwaveletmatchdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "multiid.h"
#include "uidialog.h"

class uiFunctionDisplay;
class uiGenInput;
class uiWaveletSel;

mExpClass(uiSeis) uiWaveletMatchDlg : public uiDialog
{ mODTextTranslationClass(uiWaveletMatchDlg)
public:
			uiWaveletMatchDlg(uiParent*);

    const MultiID&	getMultiID() const	{ return wvltid_; }

protected:

    void		inpSelCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiFunctionDisplay*	wvlt0disp_;
    uiFunctionDisplay*	wvlt1disp_;
    uiFunctionDisplay*	wvltoutdisp_;

    uiWaveletSel*	wvlt0fld_;
    uiWaveletSel*	wvlt1fld_;
    uiWaveletSel*	outwvltfld_;
    uiGenInput*		filterszfld_;

    MultiID		wvltid_;
};

#endif
