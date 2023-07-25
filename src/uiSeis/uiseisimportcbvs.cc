/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisimportcbvs.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seisselection.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseistransf.h"
#include "uistrings.h"
#include "uitaskrunner.h"


uiSeisImportCBVS::uiSeisImportCBVS( uiParent* p )
    : uiDialog(p,Setup(tr("Import CBVS cube"),mNoDlgTitle,
		       mODHelpKey(mSeisImpCBVSHelpID)).modal(false))
    , outioobj_(0)
    , tmpid_("100010.",IOObj::tmpID())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter("CBVS (*.cbvs)").defseldir( GetBaseDataDir() );
    inpfld_ = new uiFileInput( this, tr("(First) CBVS file name"), fisu );
    inpfld_->valueChanged.notify( mCB(this,uiSeisImportCBVS,inpSel) );

    StringListInpSpec spec;
    spec.addString( uiStrings::phrInput(uiStrings::phrData(tr("Cube"))) );
    spec.addString( tr("Generated Attribute Cube") );
    spec.addString( tr("SteeringCube") );
    typefld_ = new uiGenInput( this, tr("Cube type"), spec );
    typefld_->attach( alignedBelow, inpfld_ );
    typefld_->valueChanged.notify( mCB(this,uiSeisImportCBVS,typeChg) );

    modefld_ = new uiGenInput( this, tr("Import mode"),
			       BoolInpSpec( false, tr("Copy the data"),
						   tr("Use in-place") ) );
    modefld_->attach( alignedBelow, typefld_ );
    modefld_->valueChanged.notify( mCB(this,uiSeisImportCBVS,modeSel) );

    uiSeisTransfer::Setup sts( Seis::Vol );
    sts.withnullfill(false).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, modefld_ );

    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    sssu.enabotherdomain( true );
    IOObjContext outctxt( uiSeisSel::ioContext( Seis::Vol, false ) );
    outctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    IOM().to( outctxt.getSelKey() );
    outfld_ = new uiSeisSel( this, outctxt, sssu );
    outfld_->attach( alignedBelow, transffld_ );

    postFinalize().notify( mCB(this,uiSeisImportCBVS,typeChg) );
    postFinalize().notify( mCB(this,uiSeisImportCBVS,modeSel) );
}


uiSeisImportCBVS::~uiSeisImportCBVS()
{
    delete outioobj_;
}


IOObj* uiSeisImportCBVS::getInpIOObj( const char* inp ) const
{
    IOStream* iostrm = new IOStream( "_tmp", tmpid_ );
    iostrm->setGroup( mTranslGroupName(SeisTrc) );
    iostrm->setTranslator( CBVSSeisTrcTranslator::translKey() );
    iostrm->setDirName( "Seismics" );
    iostrm->fileSpec().setFileName( inp );
    return iostrm;
}


void uiSeisImportCBVS::modeSel( CallBacker* )
{
    transffld_->display( modefld_->getBoolValue() );
}


void uiSeisImportCBVS::typeChg( CallBacker* )
{
    transffld_->setSteering( typefld_->getIntValue() == 2 );
}


void uiSeisImportCBVS::inpSel( CallBacker* )
{
    BufferString inp = inpfld_->text();
    if ( inp.isEmpty() )
	return;

    if ( !File::isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getInpIOObj( inp );
	transffld_->updateFrom( *ioobj );
    }

    FilePath fp( inp );
    fp.setExtension( 0 );
    inp = fp.fileName();
    inp.replace( '_', ' ' );

    outfld_->setInputText( inp );
}


#define rmTmpIOObj() IOM().permRemove( MultiID(tmpid_.buf()) );

bool uiSeisImportCBVS::acceptOK( CallBacker* )
{
    const IOObj* selioobj = outfld_->ioobj();
    if ( !selioobj )
	return false;

    outioobj_ = selioobj->clone();
    mDynamicCastGet(IOStream*,iostrm,outioobj_);
    const bool dolink = modefld_ && !modefld_->getBoolValue();

    BufferString fname = inpfld_->text();
    if ( !fname.str() )
    {
	uiMSG().error( tr("Please select the input filename") );
	return false;
    }

    if ( dolink )
    {
	if ( iostrm )
	{
		// Check if it's under the survey dir, then make path relative
	    FilePath inputfile( fname );
	    inputfile.makeCanonical();
	    FilePath seismicsdir( iostrm->fileSpec().fullDirName() );
	    seismicsdir.makeCanonical();
	    if ( inputfile.makeRelativeTo( seismicsdir ) )
		fname = inputfile.fullPath();
	}
    }

    const int seltyp = typefld_->getIntValue();
    if ( seltyp == 0 )
	outioobj_->pars().removeWithKey( "Type" );
    else
	outioobj_->pars().set( sKey::Type(), seltyp == 1 ?
				    sKey::Attribute() : sKey::Steering() );

    outioobj_->setTranslator( CBVSSeisTrcTranslator::translKey() );
    PtrMan<IOObj> inioobj = 0;
    if ( dolink )
	iostrm->fileSpec().setFileName( fname );
    else
    {
	inioobj = getInpIOObj( fname );
	if ( !IOM().commitChanges(*inioobj) )
	{
	    uiMSG().error(uiStrings::phrCannotWriteDBEntry(inioobj->uiName()));
	    return false;
	}
    }

    if ( !IOM().commitChanges(*outioobj_) )
    {
	uiMSG().error( uiStrings::phrCannotWriteDBEntry(outioobj_->uiName()) );
	return false;
    }

    if ( dolink )
    {
	 uiString msg = tr("CBVS cube successfully imported."
		      "\n\nDo you want to import more cubes?");
	 bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
	 return !ret;
    }

    uiSeisIOObjInfo ioobjinfo( *outioobj_, true );
    if ( !ioobjinfo.checkSpaceLeft(transffld_->spaceInfo()) )
    {
	rmTmpIOObj();
	return false;
    }

    PtrMan<Executor> exec = transffld_->getTrcProc( *inioobj, *outioobj_,
		    "Importing CBVS seismic cube", tr("Loading data") );
    if ( !exec )
    {
	rmTmpIOObj();
	return false;
    }

    uiTaskRunner dlg( this );
    const bool allok = dlg.execute( *exec ) ;
    if ( allok && !ioobjinfo.is2D() )
	ioobjinfo.provideUserInfo();

    rmTmpIOObj();
    return allok;
}
