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
#include "oddirs.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "tabledef.h"
#include "velocityfunctionascio.h"
#include "velocityfunctionstored.h"

#include "uifileinput.h"
#include "uigeom2dsel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uiveldesc.h"

#include "hiddenparam.h"


static HiddenParam<Vel::uiImportVelFunc,uiGeom2DSel*> hp_geom2dfld( nullptr );

namespace Vel
{

uiImportVelFunc::uiImportVelFunc( uiParent* p )
    : uiImportVelFunc(p,false)
{}


uiImportVelFunc::uiImportVelFunc( uiParent* p, bool is2d )
    : uiDialog( p,uiDialog::Setup(tr("Import Velocity Function"),
				  mNoDlgTitle,
				  mODHelpKey(mImportVelFuncHelpID) )
			    .modal(false))
    , fd_(*FunctionAscIO::getDesc(is2d))
{
    setVideoKey( mODVideoKey(mImportVelFuncHelpID) );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    mAttachCB( inpfld_->valueChanged, uiImportVelFunc::inpSelCB );

    uiVelocityDesc::Setup su( nullptr, is2d, false );
    su.desc_.setType( OD::VelocityType::Interval );
    typefld_ = new uiVelocityDesc( this, su );
    typefld_->attach( alignedBelow, inpfld_ );
    mAttachCB( typefld_->typeChangeNotifier(),
	       uiImportVelFunc::velTypeChangeCB );

    auto* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, typefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
					mODHelpKey(mImportVelFuncParsHelpID) );
    dataselfld_->attach( alignedBelow, typefld_ );
    dataselfld_->attach( ensureBelow, sep );
    //TODO: set unit for velocities, copy back to typefld_

    hp_geom2dfld.setParam( this, nullptr );
    uiObject* attachobj = dataselfld_->attachObj();
    if ( is2d )
    {
	auto* geom2dfld = new uiGeom2DSel( this, true, uiStrings::sLineName() );
	geom2dfld->attach( alignedBelow, dataselfld_ );
	attachobj = geom2dfld->attachObj();
	hp_geom2dfld.setParam( this, geom2dfld );
    }

    IOObjContext ctxt = StoredFunctionSource::ioContext();
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			     uiStrings::phrOutput(tr("Velocity Function")) );
    outfld_->attach( alignedBelow, attachobj );
    outfld_->attach( ensureBelow, sep );

    mAttachCB( postFinalize(), uiImportVelFunc::initDlgCB );
}


uiImportVelFunc::~uiImportVelFunc()
{
    detachAllNotifiers();
    delete &fd_;
    hp_geom2dfld.removeParam( this );
}


void uiImportVelFunc::initDlgCB( CallBacker* )
{
    velTypeChangeCB( nullptr );
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
    fd_.bodyinfos_[mVel]->setName( toString(desc.getType()) );
}


void uiImportVelFunc::formatSel( CallBacker* )
{
    const bool is2d = hp_geom2dfld.getParam( this );
    FunctionAscIO::updateDesc( fd_, is2d );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportVelFunc::acceptOK( CallBacker* )
{
    BinIDValueSet bidvalset( 0, true);
    VelocityDesc desc;
    if ( !typefld_->get(desc,true) )
	return false;

    if ( !*inpfld_->fileName() )
	mErrRet( uiStrings::phrSelect(uiStrings::phrInput(uiStrings::sFile())) )

    od_istream strm( inpfld_->fileName() );
    if ( !strm.isOK() )
	 mErrRet( uiStrings::sCantOpenInpFile() );

    auto* geom2dfld = hp_geom2dfld.getParam( this );
    Pos::GeomID geomid;
    if ( geom2dfld )
    {
	const IOObj* ioobj = geom2dfld->ioobj();
	if ( !ioobj )
	    return false;

	const MultiID key = ioobj->key();
	geomid.set( key.objectID() );
    }
    else
	geomid = Survey::default3DGeomID();

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
	mErrRet( uiStrings::phrCannotRead( ::toUiString(inpfld_->fileName())) );

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	mErrRet( outfld_->isEmpty() ? uiStrings::phrSelect(uiStrings::sOutput())
				    : uiStrings::sEmptyString() )

    const ZDomain::Info& zinfo = SI().zDomainInfo(); //TODO support more
    RefMan<StoredFunctionSource> functions = new StoredFunctionSource;
    functions->setData( bidvalset, desc, zinfo );
    if ( !functions->store(ioobj->key()) )
	mErrRet( tr("Cannot store velocity functions") );

    uiString msg = tr("Velocity Function successfully imported."
		      "\n\nDo you want to import more Velocity Functions?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

} // namespace Vel
