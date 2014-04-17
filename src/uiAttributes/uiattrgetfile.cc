/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrgetfile.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "seistrctr.h"

#include "uiattrsrchprocfiles.h"
#include "uibutton.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uisrchprocfiles.h"
#include "uistoredattrreplacer.h"
#include "uitextedit.h"


// uiAttrSrchProcFiles implementation at end of file

using namespace Attrib;

uiGetFileForAttrSet::uiGetFileForAttrSet( uiParent* p, bool isads, bool is2d )
    : uiDialog(p,uiDialog::Setup(
		isads ? "Get Attribute Set" : "Get attributes from job file",
		isads ? "Select file containing an attribute set"
		      : "Select job specification file",
		 "101.1.3"))
    , attrset_(*new DescSet(is2d))
    , isattrset_(isads)
{
    fileinpfld = new uiFileInput( this, "File name" );
    fileinpfld->setFilter( isattrset_ ? "AttributeSet files (*.attr)"
				    : "Job specifications (*.par)" );
    fileinpfld->setDefaultSelectionDir( isattrset_ ? GetBaseDataDir()
						 : GetProcFileName(0) );
    fileinpfld->valuechanged.notify( mCB(this,uiGetFileForAttrSet,selChg) );
    if ( !isattrset_ )
    {
	uiPushButton* but = new uiPushButton( this,
				"&Find from created cube",
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

    attrset_.removeAll( false ); attrset_.usePar( iop );
    const int nrgood = attrset_.nrDescs( false, false );
    BufferString txt( nrgood == 1  ? "Attribute: "
			: (nrgood ? "Attributes:\n"
				  : "No valid attributes present") );
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


bool uiGetFileForAttrSet::acceptOK( CallBacker* )
{
    fname_ = fileinpfld->fileName();
    if ( fname_.isEmpty() || !File::exists(fname_) )
    {
	uiMSG().error( "Please enter the filename" );
	return false;
    }
    selChg(0);
    return true;
}


uiAttrSrchProcFiles::uiAttrSrchProcFiles( uiParent* p, bool is2d )
    : uiSrchProcFiles(p,mkCtio(is2d),"Output.0.Seismic.ID")
{
    setHelpKey( "101.1.4" );
}


CtxtIOObj& uiAttrSrchProcFiles::mkCtio( bool is2d )
{
    ctioptr_ = uiSeisSel::mkCtxtIOObj(is2d?Seis::Line:Seis::Vol,true);
    ctioptr_->ctxt.forread = true;
    ctioptr_->ctxt.toselect.require_.set( sKey::Type(), sKey::Attribute() );
    return *ctioptr_;
}


uiAttrSrchProcFiles::~uiAttrSrchProcFiles()
{
    delete ctioptr_;
}


// uiImpAttrSet

static BufferString sImportDir;

uiImpAttrSet::uiImpAttrSet( uiParent* p )
    : uiDialog(p,Setup("Import Attribute Set",mNoDlgTitle,mTODOHelpKey)
		 .modal(false))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    if ( sImportDir.isEmpty() )
	sImportDir = GetDataDir();

    const char* fltr = "Attribute Sets (*.attr)";
    fileinpfld_ = new uiFileInput( this, "Select",
	uiFileInput::Setup().defseldir(sImportDir).forread(true).filter(fltr) );

    attrsetfld_ = new uiIOObjSel( this, mIOObjContext(AttribDescSet) );
    attrsetfld_->attach( alignedBelow, fileinpfld_ );
}


uiImpAttrSet::~uiImpAttrSet()
{}


bool uiImpAttrSet::acceptOK( CallBacker* )
{
    const char* fnm = fileinpfld_->fileName();
    if ( !File::exists(fnm) )
    {
	uiMSG().error( "Please select existing file.");
	return false;
    }

    const IOObj* ioobj = attrsetfld_->ioobj();
    if ( !ioobj ) return false;

    BufferString errmsg;
    Attrib::DescSet ds( false );
    bool res = AttribDescSetTranslator::retrieve( ds, fnm, errmsg );
    if ( !res )
    {
	uiMSG().error( errmsg );
	return false;
    }

    uiStoredAttribReplacer sar( this, &ds );
    sar.go();

    res = AttribDescSetTranslator::store( ds, ioobj, errmsg );
    if ( !res )
    {
	uiMSG().error( errmsg );
	return false;
    }

    res = uiMSG().askGoOn( "Attribute Set successfully imported.\n"
			   "Do you want to import more Attribute Sets?" );
    return !res;
}
