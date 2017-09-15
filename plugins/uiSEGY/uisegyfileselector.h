#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "uilistbox.h"
#include "uisegybulkimporter.h"

class uiTextEdit;
namespace SEGY{ class ImpType; }
namespace File{ class Path; }

mExpClass(uiSEGY) uiSEGYFileSelector : public uiDialog
{ mODTextTranslationClass(uiSEGYFileSelector)
public:
		uiSEGYFileSelector(uiParent*, const char* fnm,
				   const char* vntname,
				   const SEGY::ImpType&,
				   const ObjectSet<uiSEGYVintageInfo>&);
		~uiSEGYFileSelector();
    bool	isEmpty() const
		{ return filenmsfld_->isEmpty(); }

    void	getSelNames(BufferStringSet&);

protected:
    void	selChgCB(CallBacker*);
    void	quickScanCB(CallBacker*);
    bool	acceptOK();
    bool	rejectOK();
    void	getSelectableFiles(const BufferString& dirpath,
				   BufferStringSet&);

    uiListBox*				filenmsfld_;
    uiTextEdit*				txtfld_;
    const File::Path&			fp_;
    const SEGY::ImpType&		imptype_;
    const ObjectSet<uiSEGYVintageInfo>&		vntinfos_;
};
