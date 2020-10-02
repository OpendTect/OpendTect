#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "uidialog.h"
#include "uilistbox.h"
#include "segyvintageimporter.h"

class uiTextEdit;
class uiComboBox;
namespace SEGY{ class ImpType; }
namespace File{ class Path; }

mExpClass(uiSEGYTools) uiSEGYFileSelector : public uiDialog
{ mODTextTranslationClass(uiSEGYFileSelector)
public:
		uiSEGYFileSelector(uiParent*, const char* fnm,
				   const SEGY::ImpType&,
				   const ObjectSet<SEGY::Vintage::Info>&,
				   const bool editmode=false,
				   const char* vntnm=0);
		~uiSEGYFileSelector();
    bool	isEmpty() const
		{ return filenmsfld_->isEmpty(); }

    void	getSelNames(BufferStringSet&);
    void	setSelectableNames(const BufferStringSet&, bool chosen=false);
    void	getSelectableFiles(const BufferString& dirpath,
				   BufferStringSet&);
    void	getVintagName(BufferString& vntnm)
		{ vntnm = vintagefld_->text(); }
    void	setVintagName( const char* vntnm )
		{ vintagefld_->setText(vntnm); }

protected:
    void	selChgCB(CallBacker*);
    void	quickScanCB(CallBacker*);
    void	vntChgCB(CallBacker*);
    bool	acceptOK();
    bool	rejectOK();

    uiListBox*				filenmsfld_;
    uiTextEdit*				txtfld_;
    uiComboBox*				vintagefld_;
    const File::Path&			fp_;
    const SEGY::ImpType&		imptype_;
    const bool				editmode_;
    const ObjectSet<SEGY::Vintage::Info>&	vntinfos_;
};
