#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "uistring.h"

class IOObj;
class uiButtonGroup;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiTable;
class uiWellSel;

namespace Coords { class uiCoordSystemSel; }
namespace Well { class Log; class Data; }



/*!\brief Dialog for loading logs from las file */

mExpClass(uiWell) uiImportLogsDlg : public uiDialog
{ mODTextTranslationClass(uiImportLogsDlg);
public:
			uiImportLogsDlg(uiParent*,const IOObj*,
					bool wtable=false);
			~uiImportLogsDlg();

protected:

    uiFileInput*	lasfld_;
    uiGenInput*		intvfld_;
    uiGenInput*		intvunfld_	    = nullptr;
    uiGenInput*		istvdfld_;
    uiGenInput*		udffld_;
    uiLabel*		unitlbl_;
    uiWellSel*		wellfld_;
    uiTable*		logstable_	    = nullptr;
    uiGenInput*		lognmfld_;
    uiListBox*		logsfld_	    = nullptr;

    bool		acceptOK(CallBacker*) override;
    void		lasSel(CallBacker*);
};


/*!\brief Dialog for writing logs to an ASCII file */

mExpClass(uiWell) uiExportLogs : public uiDialog
{ mODTextTranslationClass(uiExportLogs);
public:
			uiExportLogs(uiParent*,
				const ObjectSet<Well::Data>&,
				const BufferStringSet&);
			~uiExportLogs();

protected:

    const ObjectSet<Well::Data>& wds_;
    const BufferStringSet& logsel_;

    uiGenInput*		typefld_;
    uiButtonGroup*	zunitgrp_;
    uiGenInput*		zrangefld_;
    uiFileInput*	outfld_;
    uiGenInput*		multiwellsnamefld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    void		setDefaultRange(bool);
    void		writeHeader(od_ostream&,const Well::Data&);
    void		writeLogs(od_ostream&,const Well::Data&);

    void		typeSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    uiString		getDlgTitle( const ObjectSet<Well::Data>& wds,
				     const BufferStringSet& lognms );

};
