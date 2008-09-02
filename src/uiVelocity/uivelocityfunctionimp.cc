
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: uivelocityfunctionimp.cc,v 1.4 2008-09-02 07:17:39 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uivelocityfunctionimp.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "uicombobox.h"

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

static const char* typeoptions[] = 
{
    "Unknown",
    "Interval",
    "RMO",
    "Avg",
    0
};    	


uiImportVelFunc::uiImportVelFunc( uiParent* p )
    : uiDialog( p,uiDialog::Setup( "Import Velocity",
				   "Specify Parameters","103.2.8") )
    , ctio_( *mMkCtxtIOObj(VelocityAscData) )  
    , fd_( *VelocityAscIO::getDesc() )
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    		       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Type" );
    lcb->attach( alignedBelow, inpfld_ );
    typefld_ = lcb->box();
    typefld_->addItems( typeoptions );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, lcb );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "103.2.9" );
    dataselfld_->attach( alignedBelow, lcb );
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
    veldata.setType( BufferString(
		         typefld_->textOfItem(typefld_->currentItem())) );

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
