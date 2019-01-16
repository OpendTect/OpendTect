/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiattrgetfile.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "seistrctr.h"
#include "od_helpids.h"

#include "uiattrsrchprocfiles.h"
#include "uibutton.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uisrchprocfiles.h"
#include "uistoredattrreplacer.h"
#include "uitextedit.h"


// uiAttrSrchProcFiles implementation at end of file

using namespace Attrib;

uiGetFileForAttrSet::uiGetFileForAttrSet( uiParent* p, bool isads, bool is2d )
    : uiDialog(p,uiDialog::Setup(
      isads ? tr("Get Attribute Set") : tr("Get attributes from job file"),
      isads ? tr("Select file containing an attribute set")
	    : tr("Select job specification file"),
	      mODHelpKey(mGetFileForAttrSetHelpID) ))
    , attrset_(*new DescSet(is2d))
    , isattrset_(isads)
{
    uiFileSel::Setup fssu( OD::TextContent );
    if ( isattrset_ )
	fssu.setFormat( File::Format( tr("AttributeSet files"), "attr" ) );
    else
	fssu.setFormat( File::Format( tr("Job specifications"),
					sParFileExtension() ) )
	    .initialselectiondir( GetProcFileName(0) );
    fileinpfld = new uiFileSel(this, uiStrings::sFileName(), fssu );
    fileinpfld->newSelection.notify( mCB(this,uiGetFileForAttrSet,selChg) );

    if ( !isattrset_ )
    {
	uiPushButton* but = new uiPushButton( this,
				tr("Find from created cube"),
				mCB(this,uiGetFileForAttrSet,srchDir), false );
	but->attach( rightOf, fileinpfld );
    }

    infofld = new uiTextEdit( this, "Attribute info", true );
    infofld->attach( ensureBelow, fileinpfld );
    infofld->attach( widthSameAs, fileinpfld );
    infofld->setPrefHeightInChar( 4 );
}


uiGetFileForAttrSet::~uiGetFileForAttrSet()
{
    delete &attrset_;
}


void uiGetFileForAttrSet::srchDir( CallBacker* )
{
    uiAttrSrchProcFiles dlg( this, attrset_.is2D() );
    if ( dlg.go() )
    {
	fileinpfld->setFileName( dlg.fileName() );
	selChg();
    }
}


void uiGetFileForAttrSet::selChg( CallBacker* )
{
    fname_ = fileinpfld->fileName();
    IOPar iop; iop.read( fname_, sKey::Pars() );

    if ( !isattrset_ )
    {
	PtrMan<IOPar> subpar = iop.subselect( "Attributes" );
	iop.setEmpty();
	if ( subpar ) iop = *subpar;
    }

    attrset_.setEmpty(); attrset_.usePar( iop );
    const int nrgood = attrset_.nrDescs( false, false );

    BufferString txt( nrgood == 1 ? "Attribute: "
                        : (nrgood ? "Attributes:\n"
                                  : "No valid attributes present" ) );

    int nrdone = 0;
    const int totalnrdescs = attrset_.size();
    for ( int idx=0; idx<totalnrdescs; idx++ )
    {
	Desc* desc = attrset_.desc( idx );
	if ( desc->isHidden() || desc->isStored() ) continue;

	nrdone++;
	txt += desc->userRef();
	txt += " ("; txt += desc->attribName(); txt += ")";
	if ( nrdone != nrgood )
	    txt += "\n";
    }

    infofld->setText( txt );
}


bool uiGetFileForAttrSet::acceptOK()
{
    fname_ = fileinpfld->fileName();
    if ( fname_.isEmpty() || !File::exists(fname_) )
    {
	uiMSG().error( uiStrings::phrEnter(tr("the filename")) );
	return false;
    }
    selChg(0);
    return true;
}


uiAttrSrchProcFiles::uiAttrSrchProcFiles( uiParent* p, bool is2d )
    : uiSrchProcFiles(p,mkCtio(is2d),"Output.0.Seismic.ID")
{
    setHelpKey( mODHelpKey(mAttrSrchProcFilesHelpID) );
}


CtxtIOObj& uiAttrSrchProcFiles::mkCtio( bool is2d )
{
    ctioptr_ = new CtxtIOObj(
		uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,true) );
    ctioptr_->ctxt_.forread_ = true;
    ctioptr_->ctxt_.toselect_.require_.set( sKey::Type(), sKey::Attribute() );
    return *ctioptr_;
}


uiAttrSrchProcFiles::~uiAttrSrchProcFiles()
{
    delete ctioptr_;
}


// uiImpAttrSet

static BufferString sImportDir;

uiImpAttrSet::uiImpAttrSet( uiParent* p )
    : uiDialog(p,Setup(tr("Import Attribute Set"),mNoDlgTitle,
		       mODHelpKey(mImpAttrSetHelpID)).modal(false))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    if ( sImportDir.isEmpty() )
	sImportDir = GetDataDir();

    uiFileSel::Setup fssu( OD::TextContent );
    fssu.initialselectiondir( sImportDir )
	.setFormat( tr("OpendTect Attribute Sets"), "attr" );
    fileinpfld_ = new uiFileSel( this, uiStrings::phrSelect(
					    uiStrings::sInputFile()), fssu );
    mAttachCB( fileinpfld_->newSelection, uiImpAttrSet::inpChgd );

    PtrMan<IOObjContext> ctxt = Attrib::DescSet::getIOObjContext( false );
    uiIOObjSel::Setup objsu(uiStrings::phrOutput(uiStrings::sAttributeSet()));
    objsu.withwriteopts(false);
    attrsetfld_ = new uiIOObjSel( this, *ctxt, objsu );
    attrsetfld_->attach( alignedBelow, fileinpfld_ );
}


uiImpAttrSet::~uiImpAttrSet()
{
    detachAllNotifiers();
}


void uiImpAttrSet::inpChgd( CallBacker* )
{
    const File::Path fp( fileinpfld_->fileName() );
    sImportDir = fp.pathOnly();
    attrsetfld_->setInputText( fp.baseName() );
}


bool uiImpAttrSet::acceptOK()
{
    const char* fnm = fileinpfld_->fileName();
    if ( !File::exists(fnm) )
    {
	uiMSG().error( uiStrings::phrSelect(tr("an existing file.")) );
	return false;
    }

    const IOObj* ioobj = attrsetfld_->ioobj();
    if ( !ioobj )
	return false;

    Attrib::DescSet ds( false );
    uiRetVal uirv = ds.load( fnm );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    uiStoredAttribReplacer sar( this, &ds );
    sar.go();

    uirv = ds.store( ioobj->key() );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    return !uiMSG().askGoOn( tr("Attribute Set successfully imported."
			   "\n\nDo you want to import more Attribute Sets?") );
}
