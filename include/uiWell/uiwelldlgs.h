#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.37 2010-12-07 05:51:22 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"

class uiButtonGroup;
class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiLabeledListBox;
class uiTable;
class Coord3;
class CtxtIOObj;
class StreamData;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; };


/*! \brief Dialog for D2T Model editing. */

mClass uiWellTrackDlg : public uiDialog
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

    void			fillTable(CallBacker* cb=0);
    void			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
};


mClass uiD2TModelDlg : public uiDialog
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

mClass uiLoadLogsDlg : public uiDialog
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



mClass uiExportLogs : public uiDialog
{
public:
    				uiExportLogs(uiParent*,const Well::Data&,
					    const BoolTypeSet&);

protected:
    const Well::Data&		wd;
    const BoolTypeSet&		logsel;

    uiGenInput*			typefld;
    uiButtonGroup*		zunitgrp;
    uiGenInput*			zrangefld;
    uiFileInput*		outfld;

    void			setDefaultRange(bool);
    void			typeSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);
    void			writeHeader(StreamData&);
    void			writeLogs(StreamData&);
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mClass uiNewWellDlg : public uiGetObjectName
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

#endif
