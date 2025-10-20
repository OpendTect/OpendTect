/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabimport.h"

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
#include "od_istream.h"
#include "od_helpids.h"
#include "separstr.h"
#include "settings.h"


mDefineEnumUtils( uiColTabImport, ImportType, "Import type" )
{ "Other user", "OpendTect ColorTable file", "Petrel *.alut", "CSV", nullptr };

template <>
void EnumDefImpl<uiColTabImport::ImportType>::init()
{
    uistrings_ += uiStrings::sOtherUser();
    uistrings_ += uiStrings::sODTColTab();
    uistrings_ += uiStrings::sPetrelAlut();
    uistrings_ += ::toUiString("ASCII CSV");
}

static BufferString sHomePath;
static BufferString sFilePath;

uiColTabImport::uiColTabImport( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrImport(uiStrings::sColorTable(mPlural)),
		       mODHelpKey(mColTabImportHelpID)))
{
    setOkText( uiStrings::sImport() );
    choicefld_ = new uiGenInput( this, tr("Import from"),
				 StringListInpSpec(ImportTypeDef()) );
    choicefld_->valueChanged.notify( mCB(this,uiColTabImport,choiceSel) );

    sHomePath = sFilePath = GetPersonalDir();
    dirfld_ = new uiFileInput( this, tr("User's HOME folder"),
			       uiFileInput::Setup(sHomePath)
			       .directories(true).withexamine(true) );
    dirfld_->setReadOnly();
    dirfld_->valueChanged.notify( mCB(this,uiColTabImport,usrSel) );
    dirfld_->attach( alignedBelow, choicefld_ );

    dtectusrfld_ = new uiGenInput( this, tr("DTECT_USER (if any)") );
    dtectusrfld_->attach( alignedBelow, dirfld_ );
    dtectusrfld_->updateRequested.notify( mCB(this,uiColTabImport,usrSel) );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Color table(s) to add") );
    su.lblpos( uiListBox::LeftTop );
    listfld_ = new uiListBox( this, su, "colortables" );
    listfld_->attach( alignedBelow, dtectusrfld_ );

    messagelbl_ = new uiLabel( this, uiString::emptyString() );
    messagelbl_->setTextColor( OD::Color::Red() );
    messagelbl_->setHSzPol( uiObject::Wide );
    messagelbl_->attach( alignedBelow, dtectusrfld_ );
    messagelbl_->display( false );

    choiceSel( nullptr );
}


uiColTabImport::~uiColTabImport()
{
    deepErase( seqs_ );
}


const char* uiColTabImport::getCurrentSelColTab() const
{
    return listfld_->getText();
}


void uiColTabImport::choiceSel( CallBacker* )
{
    const auto imptype = sCast(ImportType,choicefld_->getIntValue());
    switch( imptype )
    {
	case OtherUser:
	    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
	    dirfld_->setTitleText( tr("User's HOME folder") );
	    dirfld_->setFileName( sHomePath );
	    dirfld_->enableExamine( false );
	    dtectusrfld_->display( true );
	    break;
	case ODTColTab:
	    dirfld_->setSelectMode( uiFileDialog::ExistingFile );
	    dirfld_->setTitleText( uiStrings::sInputFile() );
	    dirfld_->setFileName( sFilePath );
	    dirfld_->enableExamine( true );
	    dtectusrfld_->display( false );
	    break;
	case PetrelAlut:
	    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
	    dirfld_->setTitleText( ::toUiString("Petrel (*.alut) folder") );
	    dirfld_->setFileName( sFilePath );
	    dirfld_->enableExamine( false );
	    dtectusrfld_->display( false );
	    break;
	case CSV:
	    dirfld_->setSelectMode( uiFileDialog::ExistingFiles );
	    dirfld_->setTitleText( ::toUiString("ASCII (CSV)") );
	    dirfld_->setFileName( sFilePath );
	    dirfld_->enableExamine( true );
	    dtectusrfld_->display( false );
	    break;
	default: break;
    }

    usrSel( nullptr );
}


#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColTabImport::usrSel( CallBacker* )
{
    PtrMan<IOPar> ctabiop = nullptr;
    listfld_->setEmpty();

    const auto imptype = sCast(ImportType,choicefld_->getIntValue());

    BufferStringSet fnms;
    dirfld_->getFileNames( fnms );

    if ( fnms.isEmpty() )
    {
	showMessage(tr("No files selected for import."));
	return;
    }

    FilePath fp = fnms.get( 0 );
    if ( !File::exists(fp.fullPath()) )
    {
	uiMSG().error(tr("Please select an existing %1")
		   .arg( imptype==ODTColTab
			 || imptype==CSV ? uiStrings::sFile().toLower()
					 : uiStrings::sFolder().toLower() ));
	return;
    }

    if ( imptype==OtherUser )
    {
	sHomePath = fp.fullPath();

	fp.add( ".od" );
	if ( !File::exists(fp.fullPath()) )
	{
	    showMessage( tr("No '.od' folder found in selected home folder.") );
	    return;
	}
	else
	    showList();

	BufferString settdir( fp.fullPath() );
	const char* dtusr = dtectusrfld_->text();
	ctabiop = Settings::fetchExternal( "coltabs", dtusr, settdir );
	if ( !ctabiop )
	{
	    showMessage( tr("No user-defined color tables found") );
	    return;
	}
	else
	    showList();

	getFromSettingsPar( *ctabiop );
    }
    else if ( imptype==ODTColTab )
    {
	const BufferString fnm = fp.fullPath();
	if ( File::isDirectory(fnm) )
	{
	    if ( listfld_->isEmpty() )
		showMessage( tr("No files detected to import") );
	    else
		showList();

	    return;
	}

	sFilePath = fnm;
	ctabiop = new IOPar;
	bool res = ctabiop->read( fnm, "Default settings" );
	if ( !res )
	{
	    res = ctabiop->read( fnm, 0 );
	    if ( !res )
	    {
		showMessage(uiStrings::phrCannotRead(uiStrings::phrJoinStrings
			   (uiStrings::sColorTable(), tr("from Selected"),
			    uiStrings::sFile())));
		return;
	    }
	}
	getFromSettingsPar( *ctabiop );
    }
    else if ( imptype==PetrelAlut )
    {
	BufferStringSet filenms;
	File::listDir( fp.fullPath(), File::DirListType::FilesInDir,
		       filenms, "*.alut" );

	if ( filenms.isEmpty() )
	{
	    showMessage( tr("No Petrel (*.alut) files detected in selected "
			    "folder.") );
	    return;
	}

	getFromAlutFiles( filenms );
    }
    else if ( imptype==CSV )
    {
	for ( auto const nm : fnms )
	{
	    if ( File::isDirectory(nm->str()) )
		fnms.remove( nm->buf() );
	}

	getFromCSVFiles( fnms );

	if ( fnms.isEmpty() || listfld_->isEmpty() )
	{
	    showMessage( tr("No CSV files detected among the ones selected.") );
	    return;
	}
    }
}


void uiColTabImport::getFromSettingsPar( const IOPar& par )
{
    deepErase( seqs_ );
    BufferStringSet seqnames;
    ObjectSet<ColTab::Sequence> seqlist;
    int nrinvalididx = 0;
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> subpar = par.subselect( idx );
	if ( !subpar || !subpar->size() )
	{
	    nrinvalididx++;
	    if ( nrinvalididx>1000 )
		break;
	    else
		continue;
	}

	ColTab::Sequence* seq = new ColTab::Sequence;
	if ( !seq->usePar(*subpar) )
	{
	    delete seq;
	    nrinvalididx++;
	    continue;
	}

	seqlist += seq;
	seqnames.add( seq->name() );
    }

    if ( seqnames.isEmpty() )
	showMessage(uiStrings::phrCannotRead(uiStrings::phrJoinStrings
		   (uiStrings::sColorTable(), tr("from Selected"),
		    uiStrings::sFile())));
    else
    {
	ConstArrPtrMan<int> sortindexes = seqnames.getSortIndexes();
	for ( int idx=0; idx<seqnames.size(); idx++ )
	{
	    const int sortedidx = sortindexes[idx];
	    auto* seq = seqlist[sortedidx];
	    seqs_ += seq;

	    uiPixmap coltabpix( 16, 10 );
	    coltabpix.fill( *seq, true );
	    listfld_->addItem( ::toUiString(seq->name()), coltabpix );
	}

	showList();
    }
}


void uiColTabImport::getFromAlutFiles( const BufferStringSet& filenms )
{
    deepErase( seqs_ );
    for ( const auto* filenm : filenms )
    {
	FilePath fp( dirfld_->fileName(), filenm->str() );
	od_istream strm( fp.fullPath() );
	if ( !strm.isOK() )
	    continue;

	auto* seq = new ColTab::Sequence;
	seq->setName( fp.baseName() );
	TypeSet<int> r, g, b, a;
	BufferString line;
	while ( strm.isOK() )
	{
	    strm.getLine( line );
	    if ( line.isEmpty() || strm.isBad() )
		break;

	    const SeparString ss( line );
	    if ( ss.size()!=4 )
		break;

	    r += std::clamp( ss.getIValue(0), 0, 255 );
	    g += std::clamp( ss.getIValue(1), 0, 255 );
	    b += std::clamp( ss.getIValue(2), 0, 255 );
	    a += std::clamp( ss.getIValue(3), 0, 255 );
	}

	if ( r.isEmpty() )
	{
	    delete seq;
	    continue;
	}

	StepInterval<float> si( 0.f, 1.f, 1.f/(r.size()-1) );
	for ( int idx=0; idx<r.size(); idx++ )
	{
	    const float pos = si.atIndex( idx );
	    seq->setColor( pos, r[idx], g[idx], b[idx] );
	    seq->setTransparency( Geom::Point2D<float>(pos,255-a[idx]) );
	}

	seq->simplify();
	seqs_ += seq;
	uiPixmap coltabpix( 16, 10 );
	coltabpix.fill( *seq, true );
	listfld_->addItem( ::toUiString(fp.baseName()), coltabpix );
    }

    if ( !listfld_->isEmpty() )
	showList();
}


void uiColTabImport::getFromCSVFiles( const BufferStringSet& filenms,
				      const char* sep )
{
    deepErase( seqs_ );
    uiStringSet skippedfiles;
    for ( const auto* filenm : filenms )
    {
	if ( File::isDirectory(filenm->str()) )
	    continue;

	const FilePath fp( filenm->str() );
	od_istream strm( fp.fullPath() );
	if ( !strm.isOK() )
	    continue;

	PtrMan<ColTab::Sequence> seq = new ColTab::Sequence;
	seq->setName( fp.baseName() );
	TypeSet<float> pos;
	TypeSet<int> r, g, b, t;
	BufferString line;

	while ( strm.isOK() )
	{
	    strm.getLine( line );
	    if ( line.isEmpty() || strm.isBad() )
		break;

	    if ( line.isEqual("Position, Red, Green, Blue, Transparency") )
	    {
		strm.getLine( line );
		line.remove( "## " ).remove( "#" );
		seq->setName( line.buf() );
		continue;
	    }

	    const SeparString ss( line, *sep );
	    const int numsize = ss.size();
	    if ( numsize<4 )
		break;

	    pos += std::clamp( ss.getFValue(0), 0.f, 1.f );
	    r += std::clamp( ss.getIValue(1), 0, 255 );
	    g += std::clamp( ss.getIValue(2), 0, 255 );
	    b += std::clamp( ss.getIValue(3), 0, 255 );

	    if ( numsize==5 )
		t += std::clamp( ss.getIValue(4), 0, 255 );
	    else
		t += 0;
	}

	if ( r.isEmpty() || r.size()>255 )
	{
	    const uiString sf = tr("%1: %2")
				    .arg( filenm->buf() )
				    .arg( r.isEmpty()
					? "No RGBT values provided"
					: "More than 255 RGBT values provided");
	    skippedfiles.add( sf );
	    continue;
	}

	for ( int idx=0; idx<r.size(); idx++ )
	{
	    seq->setColor( pos[idx], r[idx], g[idx], b[idx] );
	    seq->setTransparency( Geom::Point2D<float>(pos[idx],t[idx]) );
	}

	uiPixmap coltabpix( 16, 10 );
	coltabpix.fill( *seq, true );
	listfld_->addItem( ::toUiString(seq->getName()), coltabpix );
	seqs_ += seq.release();
    }

    if ( !listfld_->isEmpty() )
	showList();

    if ( !skippedfiles.isEmpty() )
    {
	const bool all = skippedfiles.size()==filenms.size();
	const uiString msg = tr("%1 of the selected files were skipped because "
				"they either do not adhere to the CSV format "
				"or exceed the size limit.\n\n(Hint: You can "
				"use the 'Examine' tool to see why the file "
				"might be invalid)")
				 .arg( all ? "All" : "Some" );
	skippedfiles.insert( 0, msg );
	uiMSG().errorWithDetails( skippedfiles );
    }

    return;
}


bool uiColTabImport::acceptOK( CallBacker* )
{
    bool oneadded = false;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    tobeadded += seqs_[idx];
    }

    for ( int idx=0; idx<tobeadded.size(); idx++ )
    {
	ColTab::Sequence seq( *tobeadded[idx] );
	bool doset = true;
	const int seqidx = ColTab::SM().indexOf( seq.name() );
	if ( seqidx >= 0 )
	{
	    const uiString msg = tr("User Color table '%1' "
				    "will replace the existing.\nOverwrite?")
					.arg(seq.name());
	    doset = uiMSG().askOverwrite( msg );
	}

	if ( doset )
	{
	    oneadded = true;
	    seq.setType( ColTab::Sequence::User );
	    ColTab::SM().set( seq );
	}
    }

    if ( oneadded )
	ColTab::SM().write( false );

    const uiString msg = tr( "Color table(s) successfully imported."
			     "\n\nDo you want to import more Color tables?" );

    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );
    return !ret;
}


void uiColTabImport::showMessage( const uiString& msg )
{
    messagelbl_->setText( msg );
    messagelbl_->display( true );
    listfld_->display( false );
}


void uiColTabImport::showList()
{
    messagelbl_->display( false );
    listfld_->display( true );
}
