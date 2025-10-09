/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabexport.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabsequence.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "od_helpids.h"


uiColTabExport::uiColTabExport( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrExport(uiStrings::sColorTable(2)),
		       mODHelpKey(mColTabExportHelpID)))
{
    setOkText( uiStrings::sExport() );

    choicefld_ = new uiButtonGroup( this, "export_choice", OD::Horizontal );
    new uiRadioButton( choicefld_, tr("OpendTect") );
    new uiRadioButton( choicefld_, tr("Petrel") );
    new uiRadioButton( choicefld_, tr("CSV") );
    new uiLabel( this, tr("Export format"), choicefld_ );
    choicefld_->selectButton( 0 );
    mAttachCB( choicefld_->valueChanged, uiColTabExport::choiceCB );

    uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Color table(s) to export") );
    su.lblpos( uiListBox::LeftTop );
    listfld_ = new uiListBox( this, su, "colortables" );
    listfld_->attach( alignedBelow, choicefld_ );
    fillList();

    uiFileInput::Setup fisu;
    fisu.forread( false ).objtype( tr("ColorTable") );
    fisu.defseldir( GetSurveyExportDir() );
    dirfld_ = new uiFileInput( this, tr("Output folder"), fisu );
    dirfld_->setDefaultSelectionDir( GetExportToDir() );
    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    dirfld_->attach( alignedBelow, listfld_ );
    dirfld_->setStretch( 2, 0 );

    mAttachCB( postFinalize(), uiColTabExport::choiceCB );
}


uiColTabExport::~uiColTabExport()
{
}


void uiColTabExport::choiceCB( CallBacker* )
{
    const int exporttype = choicefld_->selectedId();
    if ( exporttype==0 )
    {
	dirfld_->setSelectMode( uiFileDialog::AnyFile );
	FilePath fp( GetExportToDir(), "opendtect_colortables" );
	fp.setExtension( "odct" );
	dirfld_->setFileName( fp.fullPath() );
	dirfld_->setDefaultExtension( "odct" );
	dirfld_->setTitleText( uiStrings::sOutputFile() );
    }
    else
    {
	dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
	dirfld_->setFileName( GetExportToDir() );
	dirfld_->setTitleText( tr("Output folder") );
    }
}


void uiColTabExport::fillList()
{
    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    for ( const auto* nm : allctnms )
    {
	ColTab::Sequence seq;
	const bool res = ColTab::SM().get( nm->buf(), seq );
	if ( !res )
	    continue;

	uiPixmap pm( 16, 10 );
	pm.fill( seq, true );
	listfld_->addItem( toUiString(nm->buf()), pm );
    }

    listfld_->setCurrentItem( 0 );
    listfld_->scrollToTop();
    listfld_->resizeToContents();
}


void uiColTabExport::writeODFile( const ColTab::Sequence& seq,
				  od_ostream& strm )
{
    IOPar par;
    seq.fillPar( par );
    par.write( strm, "OpendTect ColorTables" );
}


void uiColTabExport::writeAlutFile( const ColTab::Sequence& seq,
				    od_ostream& strm )
{
    for ( int idx=0; idx<256; idx++ )
    {
	const float pos = float(idx) / 255.f;
	const OD::Color col = seq.color( pos );
	strm << int(col.r()) << ","
	     << int(col.g()) << ","
	     << int(col.b()) << ","
	     << 255-col.t() << "\n";
    }
}


void uiColTabExport::writeCSVFile( const ColTab::Sequence& seq,
				   od_ostream& strm )
{
    for ( int idx=0; idx<seq.size(); idx++ )
    {
	const float pos = seq.position( idx );
	const float transparency = seq.transparencyAt( pos );
	strm << pos << ","
	     << int(seq.r(idx)) << ","
	     << int(seq.g(idx)) << ","
	     << int(seq.b(idx)) << ","
	     << transparency << "\n";
    }
}


bool uiColTabExport::acceptOK( CallBacker* )
{
    BufferStringSet chosen;
    listfld_->getChosen( chosen );
    if ( chosen.isEmpty() )
    {
	uiMSG().error( tr("Select at least 1 color table for export") );
	return false;
    }

    const int exportmode = choicefld_->selectedId();
    if ( exportmode==0 )
    {
	const FilePath fp( dirfld_->fileName() );
	od_ostream strm( fp.fullPath() );
	if ( !strm.isOK() )
	    return false;

	IOPar allpars;
	int coltabidx = 0;
	for ( int idx=0; idx<chosen.size(); idx++ )
	{
	    const BufferString& nm = chosen.get( idx );
	    ColTab::Sequence seq;
	    if ( !ColTab::SM().get(nm.buf(),seq) )
		continue;

	    IOPar par;
	    seq.fillPar( par );
	    allpars.mergeComp( par, toString(coltabidx) );
	    coltabidx++;
	}

	const bool res = allpars.write( strm, "OpendTect ColorTables" );
	if ( !res )
	{
	    uiMSG().error( tr("Could not write Color Tables to\n%1")
			   .arg(strm.fileName()) );
	    return false;
	}
    }
    else
    {
	const char* extension = exportmode==1 ? "alut" : "csv";
	int nrwritten = 0;
	for ( const auto* nm : chosen )
	{
	    ColTab::Sequence seq;
	    if ( !ColTab::SM().get(nm->buf(),seq) )
		continue;

	    BufferString fnm( nm->buf() );
	    fnm.trimBlanks();
	    fnm.clean( BufferString::NoFileSeps );
	    FilePath fp( dirfld_->fileName(), fnm.buf() );
	    fp.setExtension( extension );

	    od_ostream strm( fp.fullPath() );
	    if ( !strm.isOK() )
		continue;

	    if ( exportmode==1 )
		writeAlutFile( seq, strm );
	    else if ( exportmode==2 )
		writeCSVFile( seq, strm );

	    strm.close();

	    nrwritten++;
	}

	if ( nrwritten==0 )
	{
	    uiMSG().error( tr("No color tables have been exported. "
			      "Please check permissions.") );
	    return false;
	}
    }

    const uiString msg = tr( "Color Table(s) successfully exported."
			    "\n\nDo you want to export more Color Tables?" );
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
					   tr("No, close window") );
    return !ret;
}
