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

    uiFunctionDisplay*	wvlt0disp_;
    uiFunctionDisplay*	wvlt1disp_;
    uiFunctionDisplay*	wvltoutdisp_;
    uiFunctionDisplay*	wvltqcdisp_;

    uiWaveletSel*	wvlt0fld_;
    uiWaveletSel*	wvlt1fld_;
    uiWaveletSel*	wvltoutfld_;
    uiGenInput*		filterszfld_;

    MultiID		wvltid_;
    Wavelet&		outputwvlt_;
};

#endif

