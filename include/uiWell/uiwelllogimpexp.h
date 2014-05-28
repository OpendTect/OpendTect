#ifndef uiwelllogimpexp_h
#define uiwelllogimpexp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

class IOObj;
class uiGenInput;
class uiFileInput;
class uiListBox;
class uiButtonGroup;
class uiLabel;
class uiWellSel;

namespace Well { class Log; class Data; }



/*!\brief Dialog for loading logs from las file */

mExpClass(uiWell) uiImportLogsDlg : public uiDialog
{ mODTextTranslationClass(uiImportLogsDlg);
public:
			uiImportLogsDlg(uiParent*,const IOObj*);

protected:

    uiFileInput*	lasfld_;
    uiGenInput*		intvfld_;
    uiGenInput*		intvunfld_;
    uiGenInput*		istvdfld_;
    uiGenInput*		udffld_;
    uiLabel*		unitlbl_;
    uiListBox*		logsfld_;
    uiWellSel*		wellfld_;

    bool		acceptOK(CallBacker*);
    void		lasSel(CallBacker*);
};


/*!\brief Dialog for writing logs to an ASCII file */

mExpClass(uiWell) uiExportLogs : public uiDialog
{
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
    uiFileInput*	outfld_;
    uiGenInput*		multiwellsnamefld_;

    void		setDefaultRange(bool);
    void		writeHeader(od_ostream&,const Well::Data&);
    void		writeLogs(od_ostream&,const Well::Data&);

    void		typeSel(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
};


#endif
