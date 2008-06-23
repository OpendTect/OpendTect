
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
Date:		June 2008
 RCS:		$Id: uiprestackimpmute.cc,v 1.2 2008-06-23 10:18:45 cvsumesh Exp $
________________________________________________________________________

-*/
#include "uiprestackimpmute.h"
#include "prestackmutedeftransl.h"
#include "prestackmuteasciio.h"
#include "ctxtioobj.h"
#include "uiseparator.h"
#include "tabledef.h"
#include "uifileinput.h"
#include "oddirs.h"
#include "uitblimpexpdatasel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "prestackmutedef.h"
#include "strmprov.h"
#include "uimsg.h"

static const char* interpoltypes[] = { "Linear", "Poly", "Snap", 0 };

uiImportMute::uiImportMute( uiParent* p )
    :uiDialog( p,uiDialog::Setup( "Import Mute", "Specify Parameters",0) )
    ,ctio_( *mMkCtxtIOObj(MuteDef) )
    ,fd_( *PreStack::MuteAscIO::getDesc() )
{
    inpfld_ = new uiFileInput( this, "Input ASCII File", 
	                      uiFileInput::Setup().withexamine(true)
			      .defseldir( GetDataDir() ) );

    extpoltypefld_ = new uiGenInput( this, "Extrapolate Type",
	    				BoolInpSpec( true ) );
    extpoltypefld_->attach( alignedBelow, inpfld_ );

    intpoltypefld_ = new uiGenInput( this,"Interpolate Type",
	                          StringListInpSpec(interpoltypes) );
    intpoltypefld_->attach( alignedBelow, extpoltypefld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, intpoltypefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 0 );
    dataselfld_->attach( alignedBelow, intpoltypefld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Mute" );
    outfld_->attach( alignedBelow, inpfld_ );
    outfld_->attach( ensureBelow, sep );
}


uiImportMute::~uiImportMute()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportMute::acceptOK( CallBacker* )
{
    PreStack::MuteDef mutedef;

    if( !*inpfld_->fileName() )
    mErrRet( "Please select the input file" );

    StreamData sd = StreamProvider( inpfld_->fileName() ).makeIStream();
    PreStack::MuteAscIO muteascio( fd_, *sd.istrm );

    if ( !muteascio.getMuteDef( mutedef, extpoltypefld_->getBoolValue(),
	       			getInterpolType() ) )
	mErrRet( "Failed to convert into compatible data" );

    MuteDefTranslator* tr = 
	         (MuteDefTranslator*)ctio_.ioobj->getTranslator();
    
    if ( !tr ) return false;

    BufferString bs;
    bool retval = false;
    retval = tr->store( mutedef, ctio_.ioobj, bs );

    if ( !retval )
    mErrRet( bs.buf() );	

    return retval;
}


PointBasedMathFunction::InterpolType uiImportMute::getInterpolType()
{
    PointBasedMathFunction::InterpolType iptype = 
	                                       PointBasedMathFunction::Linear;
    
    const int typeval = intpoltypefld_->getIntValue();
     if ( typeval == 1 )
	 iptype = PointBasedMathFunction::Poly;
     if ( typeval == 2 )
	 iptype = PointBasedMathFunction::Snap;

      return iptype;
}
