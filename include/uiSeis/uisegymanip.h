#ifndef uisegymanip_h
#define uisegymanip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegymanip.h,v 1.4 2011-03-02 16:11:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "strmdata.h"
class uiLabel;
class uiTable;
class uiListBox;
class uiTextEdit;
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

protected:

    BufferString	fname_;
    SEGY::TxtHeader&	txthdr_;
    SEGY::BinHeader&	binhdr_;
    SEGY::HdrCalcSet&	calcset_;
    BufferString	errmsg_;
    BoolTypeSet		trchdrdefined_;
    StreamData		sd_;

    uiTextEdit*		txthdrfld_;
    uiSEGYBinHdrEd*	binhdrfld_;
    uiListBox*		trchdrfld_;
    uiListBox*		avtrchdrsfld_;
    uiFileInput*	fnmfld_;
    uiToolButton*	edbut_;
    uiToolButton*	rmbut_;
    uiToolButton*	savebut_;
    uiTable*		thtbl_;
    uiLabel*		errlbl_;

    void		selChg(CallBacker*);
    void		addReq(CallBacker*);
    void		edReq(CallBacker*);
    void		rmReq(CallBacker*);
    void		openReq(CallBacker*);
    void		saveReq(CallBacker*);

    bool		openFile();
    void		fillAvtrcHdrFld(int);
    void		fillDefCalcs(int);
    uiGroup*		mkTrcGroup();

    bool		acceptOK(CallBacker*);

};


#endif
