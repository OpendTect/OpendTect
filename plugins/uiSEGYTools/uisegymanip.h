#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyhdr.h"
#include "od_iosfwd.h"

class uiLabel;
class uiTable;
class uiSpinBox;
class uiListBox;
class uiTextEdit;
class uiCheckBox;
class uiFileInput;
class uiToolButton;
class uiSEGYBinHdrEd;
namespace SEGY { class TxtHeader; class BinHeader; class HdrCalcSet; }


/*!\brief UI for SEG-Y file manipulation */

mExpClass(uiSEGYTools) uiSEGYFileManip : public uiDialog
{ mODTextTranslationClass(uiSEGYFileManip);
public:

			uiSEGYFileManip(uiParent*,const char* filenm);
			~uiSEGYFileManip();

    const char*		fileName() const	{ return fname_; }
    inline od_istream&	strm()			{ return *strm_; }

    od_int64		traceBytes() const;

    static uiString	sByte()		{ return uiStrings::sByte(); }

protected:

    BufferString	fname_;
    SEGY::TxtHeader&	txthdr_;
    SEGY::BinHeader&	binhdr_;
    SEGY::HdrCalcSet&	calcset_;
    uiString		errmsg_;
    BoolTypeSet		trchdrdefined_;
    od_istream*		strm_;
    od_int64		filesize_;
    unsigned char	inphdrbuf_[SegyTrcHeaderLength];
    unsigned char	curhdrbuf_[SegyTrcHeaderLength];

    uiTextEdit*		txthdrfld_;
    uiSEGYBinHdrEd*	binhdrfld_;
    uiListBox*		trchdrfld_;
    uiListBox*		avtrchdrsfld_;
    uiFileInput*	fnmfld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	savebut_;
    uiToolButton*	plotbut_;
    uiTable*		thtbl_;
    uiCheckBox*		plotallbox_;
    uiSpinBox*		trcnrfld_;
    uiLabel*		errlbl_;

    void		initWin(CallBacker*);
    void		addReq(CallBacker*);
    void		edReq(CallBacker*);
    void		rmReq(CallBacker*);
    void		openReq(CallBacker*);
    void		saveReq(CallBacker*);
    void		plotReq(CallBacker*);
    void		selChg(CallBacker*);
    void		trcNrChg(CallBacker*);
    void		rowClck(CallBacker*);
    void		cellClck(CallBacker*);

    uiGroup*		mkTrcGroup();
    bool		openInpFile();
    void		fillAvTrcHdrFld(int);
    void		fillDefCalcs(int);
    void		updTrcVals();
    void		rowSel(int);

    bool		acceptOK(CallBacker*);

    friend class	uiSEGYFileManipDataExtracter;

};
