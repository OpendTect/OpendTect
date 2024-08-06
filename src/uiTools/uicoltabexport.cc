/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabexport.h"

#include "uifileinput.h"
#include "uigeninput.h"
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
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(
						uiStrings::sColorTable(2)),
				  mNoDlgTitle, mODHelpKey(mColTabExportHelpID)))
{
    setOkText( uiStrings::sExport() );
    choicefld_ = new uiGenInput( this, tr("Export format"),
	BoolInpSpec(true,tr("OpendTect"),tr("Petrel")) );

    uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Color table(s) to export") );
    su.lblpos( uiListBox::LeftTop );
    listfld_ = new uiListBox( this, su );
    listfld_->attach( alignedBelow, choicefld_ );
    fillList();

    uiFileInput::Setup fisu;
    fisu.forread( false ).objtype( tr("ColorTable") );
    fisu.defseldir( GetSurveyExportDir() );
    dirfld_ = new uiFileInput( this,
			       uiStrings::phrOutput(tr("location")), fisu );
    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
    dirfld_->attach( alignedBelow, listfld_ );
    dirfld_->setStretch( 2, 0 );
}


uiColTabExport::~uiColTabExport()
{
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


void uiColTabExport::writeODFile( const ColTab::Sequence& seq,
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
	     << 255-transparency << "\n";
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

    const bool asod = choicefld_->getBoolValue();
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
	fp.setExtension( asod ? "odct" : "alut" );

	od_ostream strm( fp.fullPath() );
	if ( !strm.isOK() )
	    continue;

	asod ? writeODFile( seq, strm ) : writeAlutFile( seq, strm );
	strm.close();

	nrwritten++;
    }

    if ( nrwritten==0 )
    {
	uiMSG().error( tr("No color tables have been exported. "
			  "Please check permissions.") );
	return false;
    }

    uiString msg = tr( "Color table(s) successfully exported."
		      "\n\nDo you want to export more Color Tables?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}
