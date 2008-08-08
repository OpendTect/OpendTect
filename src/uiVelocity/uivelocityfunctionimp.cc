
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: uivelocityfunctionimp.cc,v 1.1 2008-08-08 10:15:25 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uivelocityfunctionimp.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "picksettr.h"
#include "strmprov.h"
#include "tabledef.h"
#include "velocityasciio.h"
#include "velocityasctransl.h"
#include "velocityascdata.h"

namespace Vel
{


uiImportVelFunc::uiImportVelFunc( uiParent* p )
    : uiDialog( p,uiDialog::Setup("Import Velocity","Specify Parameters",0) )
    , ctio_( *mMkCtxtIOObj(VelocityAscData) )  
    , fd_( *VelocityAscIO::getDesc() )
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    		       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 0 );
    dataselfld_->attach( alignedBelow, inpfld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Velocity" );
    outfld_->attach( alignedBelow, dataselfld_ );
    outfld_->attach( ensureBelow, sep );

    finaliseDone.notify( mCB(this,uiImportVelFunc,formatSel) );
}	


uiImportVelFunc::~uiImportVelFunc()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


void uiImportVelFunc::formatSel( CallBacker* )
{
    VelocityAscIO::updateDesc( fd_ );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportVelFunc::acceptOK( CallBacker* )
{
    BinIDValueSet bidvalset( 2, true);
    VelocityAscData veldata( bidvalset );

    if ( !*inpfld_->fileName() )
	mErrRet( "Please select the input file" );

    StreamData sd = StreamProvider( inpfld_->fileName() ).makeIStream();
    if ( !sd.usable() )
	 mErrRet( "Cannot open input file" )

    VelocityAscIO velascio( fd_, *sd.istrm );

    if ( !velascio.getVelocityAscData(veldata) )    
	mErrRet( "Failed to convert into compatible data" );

    sd.close();

    if ( !outfld_->commitInput(true) )
	mErrRet( "Please select the output" )

    VelocityAscDataTranslator* tr = 
		(VelocityAscDataTranslator*)ctio_.ioobj->getTranslator();
    if ( !tr ) return false;

    BufferString bs;
    const bool retval = tr->store( veldata, ctio_.ioobj, bs );

    if ( !retval )
        mErrRet( bs.buf() );	

    uiMSG().message( "Import finished successfully" );
    return false;
}

} // namespace Vel
