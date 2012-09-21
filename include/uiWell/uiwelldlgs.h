#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"

class uiButtonGroup;
class uiCheckBox;
class uiComboBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiLabeledListBox;
class uiTable;
class Coord3;
class CtxtIOObj;
class UnitOfMeasure;
class StreamData;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; class Log; }


/*! \brief Dialog for D2T Model editing. */

mClass(uiWell) uiWellTrackDlg : public uiDialog
{
public:
				uiWellTrackDlg(uiParent*,Well::Data&);
				~uiWellTrackDlg();

protected:

    Well::Data&			wd_;
    Well::Track&		track_;
    Well::Track*		orgtrack_;

    Table::FormatDesc&		fd_;
    uiTable*			tbl_;
    uiCheckBox*			zinftfld_;

    bool			fillTable(CallBacker* cb=0);
    bool			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			exportCB(CallBacker*);
};


mClass(uiWell) uiD2TModelDlg : public uiDialog
{
public:
				uiD2TModelDlg(uiParent*,Well::Data&,bool chksh);
				~uiD2TModelDlg();

protected:

    Well::Data&			wd_;
    bool			cksh_;
    Well::D2TModel*		orgd2t_; // Must be declared *below* others

    uiTable*			tbl_;
    uiCheckBox*			unitfld_;

    void			fillTable(CallBacker*);
    void			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			expData(CallBacker*);
    void			getModel(Well::D2TModel&);
};



/*! \brief
Dialog for loading logs from las file
*/

mClass(uiWell) uiLoadLogsDlg : public uiDialog
{
public:
    				uiLoadLogsDlg(uiParent*,Well::Data&);

protected:

    uiFileInput*		lasfld;
    uiGenInput*			intvfld;
    uiGenInput*			intvunfld;
    uiGenInput*			istvdfld;
    uiGenInput*			udffld;
    uiLabel*			unitlbl;
    uiLabeledListBox*		logsfld;

    Well::Data&			wd;

    bool			acceptOK(CallBacker*);
    void			lasSel(CallBacker*);
};



mClass(uiWell) uiExportLogs : public uiDialog
{
public:
    				uiExportLogs(uiParent*,
					const ObjectSet<Well::Data>&,
					const BufferStringSet&);
protected:
    const ObjectSet<Well::Data>& wds_;
    const BufferStringSet&	logsel_;

    uiGenInput*			typefld_;
    uiButtonGroup*		zunitgrp_;
    uiGenInput*			zrangefld_;
    uiFileInput*		outfld_;
    uiGenInput*			multiwellsnamefld_;

    void			setDefaultRange(bool);
    void			writeHeader(StreamData&,const Well::Data&);
    void			writeLogs(StreamData&,const Well::Data&);

    void			typeSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mClass(uiWell) uiNewWellDlg : public uiGetObjectName
{
public:
    				uiNewWellDlg(uiParent*);
    				~uiNewWellDlg();

    const Color&		getWellColor();
    const char* 		getName() const		{ return name_; }
				
protected:

    uiColorInput*		colsel_;
    BufferString		name_;
    BufferStringSet*		nms_;

    virtual bool        	acceptOK(CallBacker*);
    const BufferStringSet&	mkWellNms();
};


/* brief some editable uom for the logs */
mClass(uiWell) uiWellLogUOMDlg : public uiDialog
{
public:
				uiWellLogUOMDlg(uiParent*,Well::Log&);

protected:

    uiComboBox*                 unfld_;
    Well::Log&			log_;

    bool			acceptOK(CallBacker*);
};


#endif

