#ifndef uigoogleexpdlg_h
#define uigoogleexpdlg_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpdlg.h,v 1.4 2011/01/10 13:29:58 cvsbert Exp $
-*/

#include "uidialog.h"
#include "filepath.h"
class uiFileInput;

#define mDecluiGoogleExpStd \
    uiFileInput*	fnmfld_; \
    bool		acceptOK(CallBacker*)

#define mImplFileNameFld(nm) \
    BufferString deffnm( nm ); \
    cleanupString( deffnm.buf(), false, false, true ); \
    FilePath deffp( GetDataDir() ); deffp.add( deffnm ).setExtension( "kml" ); \
    uiFileInput::Setup fiinpsu( uiFileDialog::Gen, deffp.fullPath() ); \
    fiinpsu.forread( false ).filter( "*.kml" ); \
    fnmfld_ = new uiFileInput( this, "Output file", fiinpsu )
    

#define mCreateWriter(typ,survnm) \
    const BufferString fnm( fnmfld_->fileName() ); \
    if ( fnm.isEmpty() ) \
	{ uiMSG().error("please enter the output file name"); return false; } \
 \
    ODGoogle::XMLWriter wrr( typ, fnm, survnm ); \
    if ( !wrr.isOK() ) \
	{ uiMSG().error(wrr.errMsg()); return false; }


#endif
