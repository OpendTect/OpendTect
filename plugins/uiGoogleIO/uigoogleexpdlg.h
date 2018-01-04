#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "uidialog.h"
#include "filepath.h"
class uiFileSel;

uiString kmlFileuiString();
uiString sOutFileName();

#define mDecluiGoogleExpStd \
    uiFileSel*		fnmfld_; \
    bool		acceptOK()

#define mImplFileNameFld(nm) \
    BufferString deffnm( nm ); \
    deffnm.clean( BufferString::AllowDots ); \
    File::Path deffp( GetDataDir() ); deffp.add( deffnm ).setExtension( "kml" ); \
    uiFileSel::Setup fssu( OD::HtmlContent, deffp.fullPath() ); \
    fssu.setForWrite().setFormat( kmlFileuiString(), "kml" ); \
    fnmfld_ = new uiFileSel( this, uiStrings::sOutputFile(), fssu );


#define mCreateWriter(typ,survnm) \
    const BufferString fnm( fnmfld_->fileName() ); \
    if ( fnm.isEmpty() ) \
	{ uiMSG().error(uiStrings::phrEnter(sOutFileName())); \
	  return false; } \
 \
    ODGoogle::XMLWriter wrr( typ, fnm, survnm ); \
    if ( !wrr.isOK() ) \
	{ uiMSG().error(wrr.errMsg()); return false; }
