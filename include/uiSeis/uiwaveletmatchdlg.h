#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "dbkey.h"
#include "uidialog.h"

class uiFunctionDisplay;
class uiGenInput;
class uiWaveletIOObjSel;
class Wavelet;

mExpClass(uiSeis) uiWaveletMatchDlg : public uiDialog
{ mODTextTranslationClass(uiWaveletMatchDlg)
public:
			uiWaveletMatchDlg(uiParent*);
			~uiWaveletMatchDlg();

    const DBKey&	getDBKey() const	{ return wvltid_; }

protected:

    void		inpSelCB(CallBacker*);
    void		filterSzCB(CallBacker*);
    bool		acceptOK();
    bool		calcFilter();

    uiFunctionDisplay*	wvlt0disp_;
    uiFunctionDisplay*	wvlt1disp_;
    uiFunctionDisplay*	wvltoutdisp_;
    uiFunctionDisplay*	wvltqcdisp_;

    uiWaveletIOObjSel*	wvlt0fld_;
    uiWaveletIOObjSel*	wvlt1fld_;
    uiWaveletIOObjSel*	wvltoutfld_;
    uiGenInput*		filterszfld_;

    DBKey		wvltid_;
    Wavelet&		outputwvlt_;
};
