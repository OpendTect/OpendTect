/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocityfunctionimp.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "tabledef.h"

#include "uifileinput.h"
#include "uigeom2dsel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uiveldesc.h"
#include "velocityfunctionascio.h"
#include "velocityfunctionstored.h"

namespace Vel
{

uiImportVelFunc::uiImportVelFunc( uiParent* p, bool is2d )
    : uiDialog( p,uiDialog::Setup(tr("Import Velocity Function"),
				  mNoDlgTitle,
				  mODHelpKey(mImportVelFuncHelpID) )
			    .modal(false))
    , fd_( *FunctionAscIO::getDesc(is2d) )
    , is2d_(is2d)
{
    setVideoKey( mODVideoKey(mImportVelFuncHelpID) );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    mAttachCB( inpfld_->valueChanged, uiImportVelFunc::inpSelCB );

    uiVelocityDesc::Setup su;
    su.desc_.type_ = VelocityDesc::Interval;
    typefld_ = new uiVelocityDesc( this, &su );
    typefld_->attach( alignedBelow, inpfld_ );
    mAttachCB( typefld_->typeChangeNotifier(),
	       uiImportVelFunc::velTypeChangeCB );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, typefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
					mODHelpKey(mImportVelFuncParsHelpID) );
    dataselfld_->attach( alignedBelow, typefld_ );
    dataselfld_->attach( ensureBelow, sep );

    uiObject* attachobj = dataselfld_->attachObj();
    if ( is2d )
    {
	geom2dfld_ = new uiGeom2DSel( this, true, uiStrings::sLineName() );
	geom2dfld_->attach( alignedBelow, dataselfld_ );
	attachobj = geom2dfld_->attachObj();
    }

    IOObjContext ctxt = StoredFunctionSource::ioContext();
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			     uiStrings::phrOutput(tr("Velocity Function")) );
    outfld_->attach( alignedBelow, attachobj );
    outfld_->attach( ensureBelow, sep );

    velTypeChangeCB( 0 );
}


uiImportVelFunc::~uiImportVelFunc()
{
    detachAllNotifiers();
    delete &fd_;
}


void uiImportVelFunc::inpSelCB( CallBacker* )
{
    const FilePath fp( inpfld_->fileName() );
    outfld_->setInputText( fp.baseName() );
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
    FunctionAscIO::updateDesc( fd_, is2d_ );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportVelFunc::acceptOK( CallBacker* )
{
    BinIDValueSet bidvalset( 0, true);
    VelocityDesc desc;
    if ( !typefld_->get( desc, true ) )
	return false;

    if ( !*inpfld_->fileName() )
	mErrRet( uiStrings::phrSelect(uiStrings::phrInput(uiStrings::sFile())) )

    od_istream strm( inpfld_->fileName() );
    if ( !strm.isOK() )
	 mErrRet( uiStrings::sCantOpenInpFile() );

    Pos::GeomID geomid;
    if ( is2d_ )
    {
	const IOObj* ioobj = geom2dfld_->ioobj();
	if ( !ioobj )
	    return false;

	const MultiID key = ioobj->key();
	geomid.set( key.objectID() );
    }

    const od_int64 filesize = File::getKbSize( inpfld_->fileName() );
    FunctionAscIO velascio( fd_, strm, geomid, filesize ? filesize : -1 );

    velascio.setOutput( bidvalset );
    bool success;
    if ( filesize>2 )
    {
	uiTaskRunner taskrunner( this );
	success = TaskRunner::execute( &taskrunner, velascio );
    }
    else
	success = velascio.execute();


    if ( !success )
	mErrRet( uiStrings::phrCannotRead( toUiString(inpfld_->fileName())) );

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	mErrRet( outfld_->isEmpty() ? uiStrings::phrSelect(uiStrings::sOutput())
				    : uiStrings::sEmptyString() )

    RefMan<StoredFunctionSource> functions = new StoredFunctionSource;
    functions->setData( bidvalset, desc, true ); //set ZisT
    if ( !functions->store( ioobj->key() ) )
	mErrRet( tr("Cannot store velocity functions") );

    uiString msg = tr("Velocity Function successfully imported."
		      "\n\nDo you want to import more Velocity Functions?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

} // namespace Vel
