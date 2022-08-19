#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class BfferStringSet;
class uiCheckBox;
class uiTable;

/*!\brief Interface for selecting line names for multi-file SEGY import */

mExpClass(uiSEGYTools) uiSEGYMultiLineSel : public uiDialog
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
