/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2002
________________________________________________________________________

-*/

#include "uiseisimportodcube.h"

#include "ioobjctxt.h"
#include "dbman.h"
#include "dbdir.h"
#include "genc.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seisblockstr.h"
#include "seisblocksreader.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "survgeom3d.h"
#include "veldesc.h"
#include "velocitycalc.h"
#include "file.h"
#include "filepath.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseistransf.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiSeisImportODCube::uiSeisImportODCube( uiParent* p )
    : uiDialog(p,Setup(tr("Import OpendTect Cube"),mNoDlgTitle,
		       mODHelpKey(mSeisImpCBVSHelpID)).modal(false))
    , outioobj_(0)
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( dbdir )
	const_cast<DBKey&>(tmpid_) = dbdir->newTmpKey();
    setCtrlStyle( RunAndClose );

    uiFileSel::Setup fssu;
    fssu.setFormat( tr("OpendTect seismic files"), "blocks", "cbvs" );
    inpfld_ = new uiFileSel( this, tr("(First) file name"), fssu );
    inpfld_->newSelection.notify( mCB(this,uiSeisImportODCube,inpSel) );

    StringListInpSpec spec;
    spec.addString(
	    uiStrings::phrInput(uiStrings::phrData(uiStrings::sCube())) );
    spec.addString( tr("Generated Attribute Cube") );
    spec.addString( uiStrings::sSteeringCube() );
    typefld_ = new uiGenInput( this, tr("Cube type"), spec );
    typefld_->attach( alignedBelow, inpfld_ );
    typefld_->valuechanged.notify( mCB(this,uiSeisImportODCube,typeChg) );

    modefld_ = new uiGenInput( this, tr("Import mode"),
			       BoolInpSpec( false, tr("Copy the data"),
						   tr("Use in-place") ) );
    modefld_->attach( alignedBelow, typefld_ );
    modefld_->valuechanged.notify( mCB(this,uiSeisImportODCube,modeSel) );

    uiSeisTransfer::Setup sts( Seis::Vol );
    sts.withnullfill(false).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, modefld_ );

    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( Seis::InclSteer );
    sssu.enabotherdomain( true );
    sssu.withwriteopts( false );
    IOObjContext outctxt( uiSeisSel::ioContext( Seis::Vol, false ) );
    outfld_ = new uiSeisSel( this, outctxt, sssu );
    outfld_->attach( alignedBelow, transffld_ );

    postFinalise().notify( mCB(this,uiSeisImportODCube,typeChg) );
    postFinalise().notify( mCB(this,uiSeisImportODCube,modeSel) );
}


uiSeisImportODCube::~uiSeisImportODCube()
{
    delete outioobj_;
}


static bool isCBVS( const char* fnm )
{
    const BufferString ext( File::Path(fnm).extension() );
    return ext == "cbvs";
}


static BufferString getTranslKey( bool iscbvs )
{
    return iscbvs ? CBVSSeisTrcTranslator::translKey()
		  : BlocksSeisTrcTranslator::translKey();
}


IOObj* uiSeisImportODCube::getInpIOObj( const char* inp ) const
{
    File::Path fp( inp );
    IOStream* iostrm = new IOStream( "_tmp", tmpid_ );
    iostrm->setGroup( mTranslGroupName(SeisTrc) );
    iostrm->setTranslator( getTranslKey(isCBVS(inp)) );
    iostrm->setDirName( sSeismicSubDir() );
    iostrm->fileSpec().setFileName( inp );
    return iostrm;
}


void uiSeisImportODCube::modeSel( CallBacker* )
{
    transffld_->display( modefld_->getBoolValue() );
}


void uiSeisImportODCube::typeChg( CallBacker* )
{
    transffld_->setIsSteering( typefld_->getIntValue() == 2 );
}


void uiSeisImportODCube::inpSel( CallBacker* )
{
    BufferString inp = inpfld_->text();
    if ( inp.isEmpty() )
	return;

    if ( !File::isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getInpIOObj( inp );
	transffld_->updateFrom( *ioobj );
    }

    File::Path fp( inp );
    fp.setExtension( 0 );
    inp = fp.fileName();
    inp.replace( '_', ' ' );

    outfld_->setInputText( inp );
}


#define rmTmpIOObj() DBM().removeEntry( tmpid_ )
#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiSeisImportODCube::acceptOK()
{
    const IOObj* selioobj = outfld_->ioobj();
    if ( !selioobj )
	return false;

    outioobj_ = selioobj->clone();
    mDynamicCastGet(IOStream*,iostrm,outioobj_);
    const bool dolink = modefld_ && !modefld_->getBoolValue();

    BufferString fname = inpfld_->text();
    if ( fname.isEmpty() )
	mErrRet( uiStrings::phrSelect(tr("the input filename")) );

    const bool iscbvs = isCBVS( fname );
    if ( !iscbvs )
    {
	Seis::Blocks::Reader rdr( fname );
	if ( rdr.state().isError() )
	    mErrRet( rdr.state() )
	if ( !rdr.hGeom().isCompatibleWith(SurvGeom::get3D()) )
	    mErrRet( tr("The selected cube is not usable in this survey") )

    }
    const BufferString translkey = getTranslKey( iscbvs );

    if ( dolink )
    {
	if ( iostrm )
	{
		// Check if it's under the survey dir, then make path relative
	    File::Path inputfile( fname );
	    inputfile.makeCanonical();
	    File::Path seismicsdir( iostrm->fileSpec().fullDirName() );
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

    outioobj_->setTranslator( translkey );
    PtrMan<IOObj> inioobj = 0;
    if ( dolink )
	iostrm->fileSpec().setFileName( fname );
    else
    {
	inioobj = getInpIOObj( fname );
	const auto inuirv = inioobj->commitChanges();
	if ( !inuirv.isOK() )
	    { uiMSG().error( inuirv ); return false; }
    }

    const auto outuirv = outioobj_->commitChanges();
    if ( !outuirv.isOK() )
	{ uiMSG().error( outuirv ); return false; }

    if ( dolink )
    {
	 uiString msg = tr("%1 cube successfully imported."
		      "\n\nDo you want to import more cubes?").arg(translkey);
	 bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
	 return !ret;
    }

    uiSeisIOObjInfo ioobjinfo( this, *outioobj_ );
    if ( !ioobjinfo.checkSpaceLeft(transffld_->spaceInfo(),true) )
	{ rmTmpIOObj(); return false; }

    PtrMan<Executor> exec = transffld_->getTrcProc( *inioobj, *outioobj_,
		    "Importing OpendTect seismic cube", tr("Loading data") );
    if ( !exec )
	{ rmTmpIOObj(); return false; }

    uiTaskRunner dlg( this );
    const bool allok = dlg.execute( *exec ) ;
    if ( allok && !ioobjinfo.is2D() )
	ioobjinfo.provideUserInfo();

    rmTmpIOObj();
    return allok;
}
