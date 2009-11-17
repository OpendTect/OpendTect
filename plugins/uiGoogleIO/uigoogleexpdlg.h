#ifndef uigoogleexpdlg_h
#define uigoogleexpdlg_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpdlg.h,v 1.2 2009-11-17 14:50:34 cvsbert Exp $
-*/

#include "uidialog.h"
class uiFileInput;

#define mDecluiGoogleExpStd \
    uiFileInput*	fnmfld_; \
    bool		acceptOK(CallBacker*)

#define mImplFileNameFld \
    fnmfld_ = new uiFileInput( this, "Output file", \
	    	uiFileInput::Setup(uiFileDialog::Gen,GetDataDir()) \
		.forread(false).filter("*.kml") )

#define mCreateWriter(typ,survnm) \
    const BufferString fnm( fnmfld_->fileName() ); \
    if ( fnm.isEmpty() ) \
	{ uiMSG().error("please enter the output file name"); return false; } \
 \
    ODGoogle::XMLWriter wrr( typ, fnm, survnm ); \
    if ( !wrr.isOK() ) \
	{ uiMSG().error(wrr.errMsg()); return false; }


#endif
