#ifndef uisegymanip_h
#define uisegymanip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegymanip.h,v 1.11 2011/03/16 12:10:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "strmdata.h"
#include "segyhdr.h"
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

mClass uiSEGYFileManip : public uiDialog
{
public:

    			uiSEGYFileManip(uiParent*,const char* filenm);
    			~uiSEGYFileManip();

    const char*		fileName() const	{ return fname_; }
    inline std::istream& strm()			{ return *sd_.istrm; }

    od_int64		traceBytes() const;

protected:

    BufferString	fname_;
    SEGY::TxtHeader&	txthdr_;
    SEGY::BinHeader&	binhdr_;
    SEGY::HdrCalcSet&	calcset_;
    BufferString	errmsg_;
    BoolTypeSet		trchdrdefined_;
    StreamData		sd_;
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


#endif
