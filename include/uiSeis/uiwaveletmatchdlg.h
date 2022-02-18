#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "multiid.h"
#include "uidialog.h"

class uiFuncDispBase;
class uiGenInput;
class uiWaveletSel;
class Wavelet;

mExpClass(uiSeis) uiWaveletMatchDlg : public uiDialog
{ mODTextTranslationClass(uiWaveletMatchDlg)
public:
			uiWaveletMatchDlg(uiParent*);
			~uiWaveletMatchDlg();

    const MultiID&	getMultiID() const	{ return wvltid_; }

protected:

    void		inpSelCB(CallBacker*);
    void		filterSzCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		calcFilter();

    uiFuncDispBase*	wvlt0disp_;
    uiFuncDispBase*	wvlt1disp_;
    uiFuncDispBase*	wvltoutdisp_;
    uiFuncDispBase*	wvltqcdisp_;

    uiWaveletSel*	wvlt0fld_;
    uiWaveletSel*	wvlt1fld_;
    uiWaveletSel*	wvltoutfld_;
    uiGenInput*		filterszfld_;

    MultiID		wvltid_;
    Wavelet&		outputwvlt_;
};

