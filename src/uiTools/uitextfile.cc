/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2002
________________________________________________________________________

-*/

#include "uitextfile.h"

#include "uibutton.h"
#include "uifileselector.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitextedit.h"

#include "filepath.h"
#include "od_iostream.h"
#include "staticstring.h"
#include "tableconvimpl.h"


class uiTableExpHandler : public Table::ExportHandler
{ mODTextTranslationClass(uiTableExpHandler);
public:

uiTableExpHandler( uiTable* t, int ml )
    : Table::ExportHandler(od_ostream::nullStream())
    , tbl_(t)
    , nrlines_(0)
    , maxlines_(ml)
{
}

bool init()
{
    tbl_->clearTable();
    tbl_->setNrRows(0);
    tbl_->setNrCols(0);
    maxcharincols_.erase();
    return true;
}

void finish()
{
    tbl_->setCurrentCell( RowCol(0,0) );
    const int maxlineswithoutscrolling = 20;
    tbl_->setPrefHeightInRows( mMIN(nrlines_,maxlineswithoutscrolling) );

    int charsum = 0;
    for ( int col=0; col<maxcharincols_.size(); col++ )
	charsum += maxcharincols_[col] + 1;
    tbl_->setPrefWidthInChars( charsum );
}

bool putRow( const BufferStringSet& bss, uiString& msg )
{
    RowCol rc( tbl_->nrRows(), 0 );
    tbl_->insertRows( rc.row(), 1 );
    if ( bss.size() >= tbl_->nrCols() )
	tbl_->insertColumns( tbl_->nrCols(), bss.size() - tbl_->nrCols() );

    for ( ; rc.col()<bss.size(); rc.col()++ )
    {
	const BufferString txt = bss.get( rc.col() );
	tbl_->setText( rc, txt );
	const int txtlen = strLength( txt );

	if ( maxcharincols_.validIdx(rc.col()) )
	    maxcharincols_[rc.col()] = mMAX(txtlen,maxcharincols_[rc.col()]);
	else
	    maxcharincols_ += txtlen;
    }

    nrlines_++;
    if ( nrlines_ >= maxlines_ )
    {
	rc.row()++; rc.col() = 0;
	tbl_->insertRows( rc.row(), 1 );
	tbl_->setText( rc, "[...]" );
	return false;
    }

    return true;
}

    uiTable*	tbl_;
    int		maxlines_;
    int		nrlines_;

    TypeSet<int> maxcharincols_;
};


//=============================================================================


#define mTxtEd() (txted_ ? (uiTextEditBase*)txted_ : (uiTextEditBase*)txtbr_)

void uiTextFile::init( uiParent* p )
{
    txted_ = 0; txtbr_ = 0; tbl_ = 0;
    const CallBack modifcb( mCB(this,uiTextFile,valChg) );

    if ( setup_.style_ == File::Table )
    {
	uiTable::Setup tsu;
	tsu.rowdesc(uiStrings::sRow()).coldesc(uiStrings::sColumn())
		.fillrow(false).fillcol(true).defcollbl(true).defrowlbl(true);
	tbl_ = new uiTable( p, tsu, filename_ );
	tbl_->setTableReadOnly( !setup_.editable_ );
	tbl_->valueChanged.notify( modifcb );
	tbl_->setStretch( 2, 2 );
	tbl_->setPrefHeight( 200 );
    }
    else if ( !setup_.editable_ || setup_.style_ == File::Bin )
	txtbr_ = new uiTextBrowser( p, filename_, setup_.maxnrlines_,
				    true, setup_.style_ == File::Log );
    else
    {
	txted_ = new uiTextEdit( p, filename_ );
	txted_->textChanged.notify( modifcb );
    }

    open( filename_ );
    ismodified_ = false;
}


void uiTextFile::valChg( CallBacker* )
{
    ismodified_ = true;
}


bool uiTextFile::open( const char* fnm )
{
    uiObj()->setName( fnm );
    if ( txted_ )
	txted_->readFromFile( fnm );
    else if ( txtbr_ )
	txtbr_->setSource( fnm );
    else
    {
	od_istream strm( fnm );
	if ( !strm.isOK() )
	    return false;

	Table::WSImportHandler imphndlr( strm );
	uiTableExpHandler exphndlr( tbl_, setup_.maxnrlines_ );
	Table::Converter cnvrtr( imphndlr, exphndlr );
	cnvrtr.execute();

	tbl_->resizeColumnsToContents();
    }


    setFileName( fnm );
    return true;
}


void uiTextFile::setFileName( const char* fnm )
{
    if ( filename_ != fnm )
	{ filename_ = fnm; fileNmChg.trigger(); }
}


bool uiTextFile::reLoad()
{
    return open( filename_ );
}


bool uiTextFile::save()
{
    return saveAs( filename_ );
}


bool uiTextFile::saveAs( const char* fnm )
{
    if ( mTxtEd() )
    {
	if ( !mTxtEd()->saveToFile( fnm ) )
	    return false;
    }
    else
    {
	od_ostream strm( fnm );
	if ( !strm.isOK() )
	    return false;
	strm << text();
	if ( !strm.isOK() )
	    return false;
    }

    setFileName( fnm );
    ismodified_ = false;
    return true;
}


int uiTextFile::nrLines() const
{
    if ( txtbr_ )
	return txtbr_->nrLines();
    else if ( tbl_ )
	return tbl_->nrRows();
    else
    {
	BufferString txt( text() );
	return txt.isEmpty() ? 0 : txt.count( '\n' );
    }
}


void uiTextFile::toLine( int lnr )
{
    const int lastln = nrLines() - 1;
    if ( lastln < 0 ) return;
    if ( lnr > lastln ) lnr = lastln;

    if ( mTxtEd() )
    {
	//TODO implement correctly
	if ( lnr > 1 )
	    mTxtEd()->scrollToBottom();
    }
    else
	tbl_->setCurrentCell( RowCol(lnr,0) );
}


const char* uiTextFile::text() const
{
    if ( mTxtEd() )
	return mTxtEd()->text();

    mDeclStaticString( ret ); ret = "";
    BufferString linetxt;
    const int nrrows = tbl_->nrRows(); const int nrcols = tbl_->nrCols();
    for ( RowCol rc(0,0); rc.row()<nrrows; rc.row()++ )
    {
	linetxt = "";
	for ( rc.col()=0; rc.col()<nrcols; rc.col()++ )
	{
	    const char* celltxt = tbl_->text( rc );
	    if ( !*celltxt ) break;
	    if ( rc.col() ) linetxt += " ";
	    linetxt += celltxt;
	}
	if ( rc.row() != nrrows-1 ) linetxt += "\n";
	ret += linetxt;
    }

    return ret.buf();
}


uiObject* uiTextFile::uiObj()
{
    uiObject* ret = mTxtEd();
    if ( !ret ) ret = tbl_;
    return ret;
}


uiTextFileDlg::uiTextFileDlg( uiParent* p, const char* fnm, bool rdonly,
				bool tbl )
	: uiDialog(p,Setup(toUiString(fnm)))
{
    Setup dlgsetup( toUiString(fnm) );
    dlgsetup.allowopen(!rdonly).allowsave(!rdonly);
    uiTextFile::Setup tfsu( tbl ? File::Table : File::Text );
    tfsu.editable_ = !rdonly;
    init( dlgsetup, tfsu, fnm );
}


uiTextFileDlg::uiTextFileDlg( uiParent* p, const Setup& dlgsetup )
	: uiDialog(p,dlgsetup)
{
    init( dlgsetup, uiTextFile::Setup(), toString(dlgsetup.wintitle_) );
}


void uiTextFileDlg::init( const uiTextFileDlg::Setup& dlgsetup,
			  const uiTextFile::Setup& tsetup, const char* fnm )
{
    if ( caption().isEmpty() )
	setCaption( toUiString(fnm) );

    captionisfilename_ = caption().getString() == fnm;

    editor_ = new uiTextFile( this, fnm, tsetup );
    editor_->fileNmChg.notify( mCB(this,uiTextFileDlg,fileNmChgd) );

    uiMenu* filemnu = new uiMenu( this, uiStrings::sFile() );
    if ( dlgsetup.allowopen_ )
	filemnu->insertAction( new uiAction(m3Dots(uiStrings::sOpen()),
				mCB(this,uiTextFileDlg,open)) );
    if ( dlgsetup.allowsave_ )
    {
	if ( tsetup.editable_ )
	    filemnu->insertAction( new uiAction(uiStrings::sSave(),
					      mCB(this,uiTextFileDlg,save)) );
	filemnu->insertAction( new uiAction(m3Dots(uiStrings::sSaveAs()),
				mCB(this,uiTextFileDlg,saveAs)) );
    }

    filemnu->insertAction( new uiAction(uiStrings::sExit(),
			   mCB(this,uiTextFileDlg,dismiss)));
    menuBar()->addMenu( filemnu );

    postFinalise().notify( mCB(this,uiTextFileDlg,finalizeCB) );
}


void uiTextFileDlg::finalizeCB(CallBacker *)
{
    uiButton* cancelbut = button(CANCEL);
    if ( !cancelbut )
	return;

    if ( FixedString(setup().canceltext_.getOriginalString()) ==
		uiStrings::sReload().getOriginalString() )
	cancelbut->setIcon( "reload" );
}


void uiTextFileDlg::setFileName( const char* fnm )
{
    if ( captionisfilename_ )
	setCaption( toUiString(fnm) );
    editor_->open( fnm );
}


void uiTextFileDlg::fileNmChgd( CallBacker* )
{
    const uiString fnm = toUiString(editor_->fileName());
    File::Path fp( mFromUiStringTodo(fnm) );
    setName( fp.fileName() );
    if ( captionisfilename_ )
	setCaption( fnm );
}


void uiTextFileDlg::open( CallBacker* )
{
    uiFileSelector uifs( this, editor_->fileName() );
    if ( uifs.go() )
	editor_->open( uifs.fileName() );
}


void uiTextFileDlg::save( CallBacker* )
{
    editor_->save();
}


void uiTextFileDlg::saveAs( CallBacker* )
{
    uiFileSelector::Setup fssu( editor_->fileName() );
    fssu.setForWrite();
    uiFileSelector uifs( this, fssu );
    if ( uifs.go() )
	editor_->saveAs( uifs.fileName() );
}


void uiTextFileDlg::dismiss( CallBacker* )
{
    accept( this );
}


bool uiTextFileDlg::rejectOK()
{
    if ( !cancelpushed_ )
	return acceptOK();

    if ( !okToExit() )
	return false;

    if ( !editor_->reLoad() )
	doMsg( tr("Cannot re-load file. Possibly the file no longer exists.") );

    return false;
}


bool uiTextFileDlg::acceptOK()
{
    return okToExit();
}


int uiTextFileDlg::doMsg( const char* msg, bool iserr )
{
    int ret = 0;

    if ( iserr )
	uiMSG().error( toUiString(msg) );
    else
	ret = uiMSG().askGoOnAfter( toUiString(msg) );

    return ret;
}


int uiTextFileDlg::doMsg( const uiString& msg, bool iserr )
{
    int ret = 0;

    if ( iserr )
	uiMSG().error( msg );
    else
	ret = uiMSG().askGoOnAfter( msg );

    return ret;
}


bool uiTextFileDlg::okToExit()
{
    if ( !editor_->isModified() )
	return true;

    const uiString msg = uiStrings::phrIsNotSavedSaveNow(
				toUiString(editor_->fileName()).quote(true) );
    int opt = doMsg( msg, false );
    if ( opt == -1 )
	return false;
    else if ( opt == 1 && !editor_->save() )
	{ doMsg( uiStrings::phrCannotSave(tr(".\nPlease try 'Save As'")) );
								return false; }

    return true;
}
