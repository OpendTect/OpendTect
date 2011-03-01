#ifndef uisegymanip_h
#define uisegymanip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegymanip.h,v 1.2 2011-03-01 11:42:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "strmdata.h"
class uiLabel;
class uiListBox;
class uiTextEdit;
class uiFileInput;
class uiSEGYBinHdrEd;
namespace SEGY { class TxtHeader; class BinHeader; }


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
    BufferString	errmsg_;
    StreamData		sd_;

    uiTextEdit*		txthdrfld_;
    uiSEGYBinHdrEd*	binhdrfld_;
    uiListBox*		trchdrfld_;
    uiFileInput*	fnmfld_;
    uiLabel*		errlbl_;

    void		fileSel(CallBacker*);

    bool		openFile();
    bool		acceptOK(CallBacker*);

};


#endif
