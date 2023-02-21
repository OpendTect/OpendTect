/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackimpmute.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"

#include "trckeysampling.h"
#include "oddirs.h"
#include "od_istream.h"
#include "prestackmuteasciio.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_helpids.h"


namespace PreStack
{

uiImportMute::uiImportMute( uiParent* p )
    : uiDialog( p,uiDialog::Setup(tr("Import Mute Function"),mNoDlgTitle,
				  mODHelpKey(mPreStackImportMuteHelpID)) )
    , fd_( *MuteAscIO::getDesc() )
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );

    inpfilehaveposfld_ = new uiGenInput( this, tr("File contains position"),
					 BoolInpSpec(true) );
    inpfilehaveposfld_->attach( alignedBelow, inpfld_ );
    inpfilehaveposfld_->valueChanged.notify(
				mCB(this,uiImportMute,changePrefPosInfo) );

    inlcrlfld_ = new uiGenInput( this, tr("Inl/Crl"),
				 PositionInpSpec(PositionInpSpec::Setup()) );
    inlcrlfld_->attach( alignedBelow, inpfilehaveposfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		      mODHelpKey(mPreStackImportMuteParsHelpID) );
    dataselfld_->attach( alignedBelow, inlcrlfld_ );

    IOObjContext ctxt = mIOObjContext( MuteDef );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt, tr("Mute Definition") );
    outfld_->attach( alignedBelow, dataselfld_ );

    postFinalize().notify( mCB(this,uiImportMute,formatSel) );
}


uiImportMute::~uiImportMute()
{
    delete &fd_;
}


void uiImportMute::formatSel( CallBacker* )
{
    inlcrlfld_->display( !haveInpPosData() );
    MuteAscIO::updateDesc( fd_, haveInpPosData() );
}


void uiImportMute::changePrefPosInfo( CallBacker* cb )
{
    BinID center( SI().inlRange(false).center(),
		  SI().crlRange(false).center() );
    SI().snap( center );
    inlcrlfld_->setValue( center );

    formatSel( cb );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportMute::acceptOK( CallBacker* )
{
    MuteDef mutedef;

    if ( !*inpfld_->fileName() )
	mErrRet( uiStrings::phrSelect(mJoinUiStrs(sInput(),sFile())) )

    od_istream strm( inpfld_->fileName() );
    if ( !strm.isOK() )
	mErrRet( uiStrings::phrCannotOpen(mJoinUiStrs(sInput(),sFile())) )

    MuteAscIO muteascio( fd_, strm );

    if ( haveInpPosData() )
    {
	if ( !muteascio.getMuteDef(mutedef) )
	    mErrRet( uiStrings::phrCannotRead(
				    toUiString( inpfld_->fileName() ) ) );
    }
    else
    {
	TrcKeySampling hs;

	if ( inlcrlfld_->getBinID() == BinID(mUdf(int),mUdf(int)) )
	    mErrRet( tr("Enter Inl/Crl") )

	else if ( !hs.includes(inlcrlfld_->getBinID()) )
	    mErrRet( tr("Enter Inl/Crl within survey range") )

	else if ( !muteascio.getMuteDef(mutedef, inlcrlfld_->getBinID()) )
	    mErrRet( uiStrings::phrCannotRead( toUiString(inpfld_->fileName())))
    }

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

   PtrMan<MuteDefTranslator> trans =
			sCast(MuteDefTranslator*,ioobj->createTranslator());
    if ( !trans ) return false;

    uiString str;
    const bool retval = trans->store( mutedef, ioobj, str );
    if ( !retval )
    {
	if ( str.isSet() )
	    uiMSG().error( str );
	return false;
	//TODO: integrate to mErrRet when all messages are uiString
	//should be part of another revision
    }

    uiMSG().message( tr("Import finished successfully") );
    return false;
}


bool uiImportMute::haveInpPosData() const
{
    return inpfilehaveposfld_->getBoolValue();
}

} // namespace PreStack
