#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "uistring.h"

class IOObj;
class uiButtonGroup;
class uiFileSel;
class uiGenInput;
class uiLabel;
class uiTable;
class uiWellSel;

namespace Well { class Log; class Data; }


/*!\brief Dialog for loading logs from las file */

mExpClass(uiWell) uiImportLogsDlg : public uiDialog
{ mODTextTranslationClass(uiImportLogsDlg)
public:
			uiImportLogsDlg(uiParent*,const IOObj*);

protected:

    uiFileSel*		lasfld_;
    uiGenInput*		intvfld_;
    uiGenInput*		istvdfld_;
    uiGenInput*		udffld_;
    uiLabel*		unitlbl_;
    uiWellSel*		wellfld_;
    uiTable*		logstable_;
    uiGenInput*		lognmfld_;

    bool		acceptOK();
    void		lasSel(CallBacker*);
};


/*!\brief Dialog for writing logs to an ASCII file */

mExpClass(uiWell) uiExportLogs : public uiDialog
{ mODTextTranslationClass(uiExportLogs)
public:
			uiExportLogs(uiParent*,
				const ObjectSet<Well::Data>&,
				const BufferStringSet&);

protected:

    const ObjectSet<Well::Data>& wds_;
    const BufferStringSet& logsel_;

    uiGenInput*		typefld_;
    uiButtonGroup*	zunitgrp_;
    uiGenInput*		zrangefld_;
    uiFileSel*		outfld_;
    uiGenInput*		multiwellsnamefld_;

    void		setDefaultRange(bool);
    void		writeHeader(od_ostream&,const Well::Data&);
    void		writeLogs(od_ostream&,const Well::Data&);

    void		typeSel(CallBacker*);
    virtual bool	acceptOK();
    uiString		getDlgTitle( const ObjectSet<Well::Data>& wds,
				     const BufferStringSet& lognms );
};
