
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
Date:		June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiprestackimpmute.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "horsampling.h"
#include "oddirs.h"
#include "prestackmutedeftransl.h"
#include "prestackmuteasciio.h"
#include "prestackmutedef.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"


namespace PreStack
{


uiImportMute::uiImportMute( uiParent* p )
    : uiDialog( p,uiDialog::Setup("Import Mute","Specify Parameters","103.2.5"))
    , ctio_( *mMkCtxtIOObj(MuteDef) )
    , fd_( *MuteAscIO::getDesc() )
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File", 
	                       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    inpfilehaveposfld_ = new uiGenInput( this, "File contains position",
	   				 BoolInpSpec(true) );
    inpfilehaveposfld_->attach( alignedBelow, inpfld_ );
    inpfilehaveposfld_->valuechanged.notify(
	      			mCB(this,uiImportMute,changePrefPosInfo) );
  
    inlcrlfld_ = new uiGenInput( this, "Inl/Crl", 
				 PositionInpSpec(PositionInpSpec::Setup()) );
    inlcrlfld_->attach( alignedBelow, inpfilehaveposfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inlcrlfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "103.2.7" );
    dataselfld_->attach( alignedBelow, inlcrlfld_ ); 
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Mute" );
    outfld_->attach( alignedBelow, dataselfld_ );
    outfld_->attach( ensureBelow, sep );

    postFinalise().notify( mCB(this,uiImportMute,formatSel) );
}


uiImportMute::~uiImportMute()
{
    delete ctio_.ioobj; delete &ctio_;
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


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiImportMute::acceptOK( CallBacker* )
{
    MuteDef mutedef;

    if ( !*inpfld_->fileName() )
	mErrRet( "Please select the input file" );

    StreamData sd = StreamProvider( inpfld_->fileName() ).makeIStream();
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    MuteAscIO muteascio( fd_, *sd.istrm );

    if ( haveInpPosData() )
    {
	if ( !muteascio.getMuteDef(mutedef) )
	    mErrRet( "Failed to convert into compatible data" );
    }
    else
    {
	HorSampling hs;

	if ( inlcrlfld_->getBinID() == BinID(mUdf(int),mUdf(int)) )
	    mErrRet( "Please enter Inl/Crl" )

	else if ( !hs.includes(inlcrlfld_->getBinID()) )
	    mErrRet( "Please enter Inl/Crl within survey range" )

	else if ( !muteascio.getMuteDef(mutedef, inlcrlfld_->getBinID()) )
	    mErrRet( "Failed to convert into compatible data" )
    }

    sd.close();
    
    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please select the output" : 0 )
		    
    MuteDefTranslator* tr = (MuteDefTranslator*)ctio_.ioobj->getTranslator();
    if ( !tr ) return false;

    BufferString bs;
    const bool retval = tr->store( mutedef, ctio_.ioobj, bs );
    if ( !retval )
	mErrRet( bs.buf() );

    uiMSG().message( "Import finished successfully" );
    return false;
}


bool uiImportMute::haveInpPosData() const
{
    return inpfilehaveposfld_->getBoolValue();
}

};// namespace PreStack
