
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uivelocityfunctionimp.h"

#include "file.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "uicombobox.h"
#include "uitaskrunner.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "picksettr.h"
#include "strmprov.h"
#include "tabledef.h"
#include "velocityfunctionascio.h"
#include "velocityfunctionstored.h"
#include "uiveldesc.h"

namespace Vel
{

uiImportVelFunc::uiImportVelFunc( uiParent* p )
    : uiDialog( p,uiDialog::Setup( "Import Velocity Function",
				   "Specify Parameters","103.2.8") )
    , ctio_( *new CtxtIOObj( StoredFunctionSource::ioContext() ) )
    , fd_( *FunctionAscIO::getDesc() )
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    		       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    uiVelocityDesc::Setup su;
    su.desc_.type_ = VelocityDesc::Interval;
    typefld_ = new uiVelocityDesc( this, &su );
    typefld_->attach( alignedBelow, inpfld_ );
    typefld_->typeChangeNotifier().notify(
	    mCB(this,uiImportVelFunc,velTypeChangeCB) );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, typefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "103.2.9" );
    dataselfld_->attach( alignedBelow, typefld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Velocity" );
    outfld_->attach( alignedBelow, dataselfld_ );
    outfld_->attach( ensureBelow, sep );

    postFinalise().notify( mCB(this,uiImportVelFunc,formatSel) );
    velTypeChangeCB( 0 );
}	


uiImportVelFunc::~uiImportVelFunc()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


#define mVel 2

void uiImportVelFunc::velTypeChangeCB( CallBacker* )
{
    VelocityDesc desc;
    typefld_->get( desc, false );
    fd_.bodyinfos_[mVel]->setName( VelocityDesc::getTypeString(desc.type_) );
}


void uiImportVelFunc::formatSel( CallBacker* )
{
    FunctionAscIO::updateDesc( fd_ );
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiImportVelFunc::acceptOK( CallBacker* )
{
    BinIDValueSet bidvalset( 0, true);
    VelocityDesc desc;
    if ( !typefld_->get( desc, true ) )
	return false;

    if ( !*inpfld_->fileName() )
	mErrRet( "Please select the input file" );

    StreamData sd = StreamProvider( inpfld_->fileName() ).makeIStream();
    if ( !sd.usable() )
	 mErrRet( "Cannot open input file" );

    const od_int64 filesize = File::getKbSize( inpfld_->fileName() );
    FunctionAscIO velascio( fd_, *sd.istrm, filesize ? filesize : -1 );

    velascio.setOutput( bidvalset );
    bool success;
    if ( filesize>2 )
    {
    	uiTaskRunner tr( this );
	success = tr.execute( velascio );
    }
    else
        success = velascio.execute();

    
    if ( !success )
	mErrRet( "Failed to convert into compatible data" );

    sd.close();

    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please select the output" : 0)

    RefMan<StoredFunctionSource> functions = new StoredFunctionSource;
    functions->setData( bidvalset, desc, true ); //set ZisT
    if ( !functions->store( ctio_.ioobj->key() ) )
	mErrRet( "Cannot store velocity functions" );

    uiMSG().message( "Import succeeded" );
    return false;
}

} // namespace Vel
