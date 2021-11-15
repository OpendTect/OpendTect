#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"
#include "position.h"
#include "uistring.h"

class uiButtonGroup;
class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiMultiWellSel;
class uiPushButton;
class uiTable;
class uiTableImpDataSel;
class uiTextBrowser;
class uiUnitSel;
class uiMnemonicsSel;
class uiWellSel;
class BufferStringSet;
class Mnemonic;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; class Log;
		 class LogSet;}

/*! \brief Dialog to show information on Well Manager */

mExpClass(uiWell) uiWellMgrInfoDlg : public uiDialog
{ mODTextTranslationClass(uiWellMgrInfoDlg);
public:
			uiWellMgrInfoDlg(uiParent*);
			~uiWellMgrInfoDlg();

    void		refresh(CallBacker*);

protected:
    uiTextBrowser*	browser_;
};


/*! \brief Dialog for Well track editing. */

mExpClass(uiWell) uiWellTrackDlg : public uiDialog
{ mODTextTranslationClass(uiWellTrackDlg);
public:
			uiWellTrackDlg(uiParent*,Well::Data&);
			~uiWellTrackDlg();

    static const uiString	sCkShotData();
    static const uiString	sTimeDepthModel();

protected:

    Well::Data&		wd_;
    Well::Track&	track_;
    Well::Track*	orgtrack_;
    bool		writable_;

    Table::FormatDesc&	fd_;
    uiTable*		tbl_;
    uiGenInput*		uwifld_;
    uiCheckBox*		zinftfld_;
    uiGenInput*		wellheadxfld_;
    uiGenInput*		wellheadyfld_;
    uiGenInput*		kbelevfld_;
    uiGenInput*		glfld_;

    Coord3		origpos_;
    float		origgl_;

    void		fillTable(CallBacker* cb);
    bool		fillTable();
    void		fillSetFields(CallBacker* cb=0);
    void		updNow(CallBacker*);
    bool		updNow();
    void		readNew(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		exportCB(CallBacker*);
    void		updateXpos(CallBacker*);
    void		updateYpos(CallBacker*);
    void		updateKbElev(CallBacker*);

    double		getX(int row) const;
    double		getY(int row) const;
    double		getZ(int row) const;
    float		getMD(int row) const;
    void		setX(int row,double);
    void		setY(int row,double);
    void		setZ(int row,double);
    void		setMD(int row,float);

    void		updatePos(bool isx);
    bool		rowIsIncomplete(int) const;
    bool		rowIsNotSet(int) const;
};


/*! \brief Dialog for D2T Model editing. */
mExpClass(uiWell) uiD2TModelDlg : public uiDialog
{ mODTextTranslationClass(uiD2TModelDlg);
public:
			uiD2TModelDlg(uiParent*,Well::Data&,bool chksh);
			~uiD2TModelDlg();

protected:

    Well::Data&		wd_;
    bool		cksh_;
    bool		writable_;
    Well::D2TModel*	orgd2t_; // Must be declared *below* others
    float		origreplvel_;

    uiTable*		tbl_;
    uiCheckBox*		zinftfld_;
    uiCheckBox*		timefld_;
    uiGenInput*		replvelfld_;

    void		fillTable(CallBacker*);
    void		fillReplVel(CallBacker*);
    bool		getFromScreen();
    void		updNow(CallBacker*);
    void		updReplVelNow(CallBacker*);
    void		dtpointChangedCB(CallBacker*);
    void		dtpointRemovedCB(CallBacker*);
    bool		updateDtpointDepth(int row);
    bool		updateDtpointTime(int row);
    bool		updateDtpoint(int row,float oldval);
    void		readNew(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		expData(CallBacker*);
    void		getModel(Well::D2TModel&);
    void		correctD2TModelIfInvalid();

    void		getColLabels(BufferStringSet&) const;
    int			getTVDGLCol() const;
    int			getTVDSDCol() const;
    int			getTVDSSCol() const;
    int			getTimeCol() const;
    int			getVintCol() const;
    bool		rowIsIncomplete(int row) const;
    int			getPreviousCompleteRowIdx(int row) const;
    int			getNextCompleteRowIdx(int row) const;
    void		setDepthValue(int irow,int icol,float);
    float		getDepthValue(int irow,int icol) const;
    void		setTimeValue(int irow,float);
    float		getTimeValue(int irow) const;
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mExpClass(uiWell) uiNewWellDlg : public uiGetObjectName
{ mODTextTranslationClass(uiNewWellDlg);
public:
			uiNewWellDlg(uiParent*);
			~uiNewWellDlg();

    const OD::Color&	getWellColor();
    const char*		getWellName() const	{ return wellname_; }

protected:

    uiColorInput*	colsel_;
    BufferString	wellname_;
    BufferStringSet*	nms_;

    virtual bool	acceptOK(CallBacker*);
    const BufferStringSet& mkWellNms();
};


/* brief Dialog for editing the logs's unit of measure */

mExpClass(uiWell) uiWellLogUOMDlg : public uiDialog
{ mODTextTranslationClass(uiWellLogUOMDlg);
public:
			uiWellLogUOMDlg(uiParent*,
					ObjectSet<ObjectSet<Well::Log>>& wls,
					TypeSet<MultiID>& keys,
					const BufferStringSet& wellnms);
			~uiWellLogUOMDlg();

protected:

    ObjectSet<ObjectSet<Well::Log>>&		wls_;
    ObjectSet<uiUnitSel>			unflds_;
    TypeSet<MultiID>&				keys_;
    uiTable*					uominfotbl_;

    void		initDlg(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		fillTable(const BufferStringSet& wellnms);
    bool		setUoMValues();
};


/* brief Dialog for editing the logs's mnemonic */

mExpClass(uiWell) uiWellLogMnemDlg : public uiDialog
{ mODTextTranslationClass(uiWellLogMnemDlg);
public:
			uiWellLogMnemDlg(uiParent*,
					ObjectSet<ObjectSet<Well::Log>>& wls,
					TypeSet<MultiID>& keys,
					const BufferStringSet& wellnms);
			~uiWellLogMnemDlg();

protected:

    ObjectSet<ObjectSet<Well::Log>>&		wls_;
    ObjectSet<uiMnemonicsSel>			mnemflds_;
    TypeSet<MultiID>&				keys_;
    uiTable*					mneminfotbl_;

    void		initDlg(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		fillTable(const BufferStringSet&);
    bool		setMnemonics();
};


/* brief Dialog for setting/editing default logs for mnemonics */

mExpClass(uiWell) uiWellDefMnemLogDlg : public uiDialog
{ mODTextTranslationClass(uiWellDefMnemLogDlg);
public:
            uiWellDefMnemLogDlg(uiParent*,
                                const TypeSet<MultiID>& keys);
            ~uiWellDefMnemLogDlg();

protected:

    uiTable*                table_ = nullptr;
    uiListBox*              welllist_ = nullptr;
    ObjectSet<Well::Data>   wds_;

    uiTable*        createLogTable();
    void			fillTable(const MultiID&);
    void            fillLogRow(const Well::LogSet&,
                               const ObjectSet<const Mnemonic>&);

    void            wellChangedCB(CallBacker*);
};

/* brief Dialog to set Depth-to-Time model to selected wells */

mExpClass(uiWell) uiSetD2TFromOtherWell : public uiDialog
{ mODTextTranslationClass(uiSetD2TFromOtherWell);
public:
			uiSetD2TFromOtherWell(uiParent*);
			~uiSetD2TFromOtherWell();

    void		setSelected(const TypeSet<MultiID>&);

protected:

    void		inpSelCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiWellSel*		inpwellfld_;
    uiGenInput*		replvelfld_;
    uiMultiWellSel*	wellfld_;
};


/* brief Dialog to copy a well */
// This tool simply reads the well and all auxilary data
// and writes it back to disk under another name

mExpClass(uiWell) uiCopyWellDlg : public uiDialog
{ mODTextTranslationClass(uiCopyWellDlg)
public:
			uiCopyWellDlg(uiParent*);
			~uiCopyWellDlg();

    void		setKey(const MultiID&);
    MultiID		getKey() const;

protected:

    void		inpSelCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiWellSel*		infld_;
    uiWellSel*		outfld_;
};
