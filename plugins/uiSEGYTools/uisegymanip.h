#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyhdr.h"
#include "uistrings.h"
#include "od_iosfwd.h"

class uiCheckBox;
class uiFileSel;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiTextEdit;
class uiSEGYBinHdrEd;
class uiSpinBox;
class uiTable;
class uiToolButton;
namespace SEGY { class TxtHeader; class BinHeader; class HdrCalcSet; }


/*!\brief UI for SEG-Y file manipulation */

mExpClass(uiSEGYTools) uiSEGYFileManip : public uiDialog
{ mODTextTranslationClass(uiSEGYFileManip);
public:

    mUseType( SEGY,	TxtHeader );
    mUseType( SEGY,	BinHeader );
    mUseType( SEGY,	HdrCalcSet );

			uiSEGYFileManip(uiParent*,const char* filenm);
			~uiSEGYFileManip();

    const char*		fileName() const	{ return fname_; }
    inline od_istream&	strm()			{ return *strm_; }

    od_int64		traceBytes() const;

    static uiWord	sByte()		{ return uiStrings::sByte(); }

protected:

    BufferString	fname_;
    TxtHeader&		txthdr_;
    BinHeader&		binhdr_;
    HdrCalcSet&		calcset_;
    uiString		errmsg_;
    BoolTypeSet		trchdrdefined_;
    od_istream*		strm_;
    od_int64		filesize_;
    unsigned char	inphdrbuf_[SegyTrcHeaderLength];
    unsigned char	curhdrbuf_[SegyTrcHeaderLength];
    bool		dotxthd_;
    bool		dobinhd_;
    int			databytespertrace_;

    uiTextEdit*		txthdrfld_;
    uiSEGYBinHdrEd*	binhdrfld_;
    uiListBox*		trchdrfld_;
    uiListBox*		avtrchdrsfld_;
    uiFileSel*		outfnmfld_;
    uiFileSel*		inpfnmsfld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	savebut_;
    uiToolButton*	plotbut_;
    uiTable*		thtbl_;
    uiCheckBox*		plotallbox_;
    uiCheckBox*		dotxthdbox_;
    uiCheckBox*		dobinhdbox_;
    uiSpinBox*		trcnrfld_;
    uiLabel*		errlbl_;
    uiGenInput*		selmultifld_;
    uiGenInput*		postfixfld_;

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
    void		destSelCB(CallBacker*);

    uiGroup*		mkTrcGroup();
    bool		openInpFile();
    void		fillAvTrcHdrFld(int);
    void		fillDefCalcs(int);
    void		updTrcVals();
    void		rowSel(int);

    bool		acceptOK();
    bool		handleFile(const char*);

    friend class	uiSEGYFileManipDataExtracter;

};
