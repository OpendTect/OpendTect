#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sep 2020
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class BfferStringSet;
class uiCheckBox;
class uiTable;

/*!\brief Interface for selecting line names for multi-file SEGY import */

mExpClass(uiSEGY) uiSEGYMultiLineSel : public uiDialog
{ mODTextTranslationClass(uiSEGYMultiLineSel);
public:

			uiSEGYMultiLineSel(uiParent*,const SEGY::FileSpec& fs,
					int& wcidx,BufferStringSet& linenames);
			~uiSEGYMultiLineSel();


protected:

    void		updateLineAvailability(int rowidx);
    int			guessWCIdx() const;
    void		checkCB(CallBacker*);
    void		lineEditCB(CallBacker*);
    void		initTable();
    bool		acceptOK(CallBacker*);

    SEGY::FileSpec	filespec_;
    int			nrwc_;
    int&		selwcidx_;
    int			curwcidx_;
    BufferStringSet&	linenames_;

    uiTable*		tbl_;

    ObjectSet<uiCheckBox>	checkboxes_;

};
