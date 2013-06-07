/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uitextfile.h"
#include "uitextedit.h"
#include "uitable.h"
#include "uimenu.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "strmprov.h"
#include "filepath.h"
#include "tableconvimpl.h"

#define mTxtEd() (txted_ ? (uiTextEditBase*)txted_ : (uiTextEditBase*)txtbr_)


void uiTextFile::init( uiParent* p )
{
    txted_ = 0; txtbr_ = 0; tbl_ = 0;
    if ( setup_.table_ )
    {
	uiTable::Setup tsu;
	tsu.rowdesc("Row").coldesc("Col").fillrow(false).fillcol(true)
	    .defcollbl(true).defrowlbl(true);
	tbl_ = new uiTable( p, tsu, setup_.filename_ );
	tbl_->setTableReadOnly( setup_.readonly_ );
	tbl_->setStretch( 2, 2 );
	tbl_->setPrefHeight( 200 );
    }
    else if ( !setup_.readonly_ )
	txted_ = new uiTextEdit( p, setup_.filename_ );
    else
	txtbr_ = new uiTextBrowser( p, setup_.filename_, setup_.maxlines_,
				    true, setup_.logviewmode_ );

    BufferString nm( setup_.filename_ );
    setup_.filename_ = "";
    open( nm );
}


bool uiTextFile::isModified() const
{
    return setup_.readonly_ ? false : (mTxtEd() ? mTxtEd()->isModified()
					//TODO : tbl_->isModified());
						: false );
}


class uiTableExpHandler : public Table::ExportHandler
{
public:

uiTableExpHandler( uiTable* t, int ml )
    : Table::ExportHandler(std::cerr)
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
    return true;
}

void finish()
{
    tbl_->setCurrentCell( RowCol(0,0) );
}

const char* putRow( const BufferStringSet& bss )
{
    RowCol rc( tbl_->nrRows(), 0 );
    tbl_->insertRows( rc.row, 1 );
    if ( bss.size() >= tbl_->nrCols() )
	tbl_->insertColumns( tbl_->nrCols(), bss.size() - tbl_->nrCols() );

    for ( ; rc.col<bss.size(); rc.col++ )
	tbl_->setText( rc, bss.get(rc.col) );

    nrlines_++;
    if ( nrlines_ >= maxlines_ )
    {
	rc.row++; rc.col = 0;
	tbl_->insertRows( rc.row, 1 );
	tbl_->setText( rc, "[...]" );
	return "";
    }
    return 0;
}

    uiTable*	tbl_;
    int		maxlines_;
    int		nrlines_;
    BufferString msg_;

};


bool uiTextFile::open( const char* fnm )
{
    uiObj()->setName( fnm );
    if ( txted_ )
	txted_->readFromFile( fnm );
    else if ( txtbr_ )
	txtbr_->setSource( fnm );
    else
    {
	StreamData sd = StreamProvider(fnm).makeIStream();
	if ( !sd.usable() )
	    { sd.close(); return false; }

	Table::WSImportHandler imphndlr( *sd.istrm );
	uiTableExpHandler exphndlr( tbl_, setup_.maxlines_ );
	Table::Converter cnvrtr( imphndlr, exphndlr );
	cnvrtr.execute();
    }

    if ( setup_.filename_ != fnm )
    {
	setup_.filename_ = fnm;
	fileNmChg.trigger();
    }

    return true;
}


bool uiTextFile::reLoad()
{
    return open( setup_.filename_ );
}


bool uiTextFile::save()
{
    return saveAs( setup_.filename_ );
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
	StreamData sd = StreamProvider(fnm).makeOStream();
	if ( !sd.usable() )
	    { sd.close(); return false; }
	*sd.ostrm << text();
	if ( !sd.ostrm->good() )
	    { sd.close(); return false; }
	sd.close();
	// tbl_->setModified( false );
    }

    if ( setup_.filename_ != fnm )
    {
	setup_.filename_ = fnm;
	fileNmChg.trigger();
    }
    return true;
}


int uiTextFile::nrLines() const
{
    if ( txtbr_ )
	return txtbr_->nrLines();
    else if ( tbl_ )
	return tbl_->nrRows();
    else
	return 100; // TODO
}


void uiTextFile::toLine( int lnr )
{
    const int lastln = nrLines() - 1;
    if ( lastln < 0 ) return;
    if ( lnr > lastln ) lnr = lastln;

    if ( mTxtEd() )
    {
	//TODO implement correctly
	if ( txtbr_ && lnr > 1 )
	    txtbr_->scrollToBottom();
    }
    else
	tbl_->setCurrentCell( RowCol(lnr,0) );
}


const char* uiTextFile::text() const
{
    if ( mTxtEd() )
	return mTxtEd()->text();

    static BufferString ret; ret = "";
    BufferString linetxt;
    const int nrrows = tbl_->nrRows(); const int nrcols = tbl_->nrCols();
    for ( RowCol rc(0,0); rc.row<nrrows; rc.row++ )
    {
	linetxt = "";
	for ( rc.col=0; rc.col<nrcols; rc.col++ )
	{
	    const char* celltxt = tbl_->text( rc );
	    if ( !*celltxt ) break;
	    if ( rc.col ) linetxt += " ";
	    linetxt += celltxt;
	}
	if ( rc.row != nrrows-1 ) linetxt += "\n";
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


uiTextFileDlg::uiTextFileDlg( uiParent* p, bool rdonly, bool tbl,
			      const char* fnm )
	: uiDialog(p,Setup(fnm))
{
    Setup dlgsetup( fnm );
    dlgsetup.allowopen(!rdonly).allowsave(!rdonly);
    init( dlgsetup, uiTextFile::Setup(rdonly,tbl,fnm) );
}


uiTextFileDlg::uiTextFileDlg( uiParent* p, const Setup& dlgsetup )
	: uiDialog(p,dlgsetup)
{
    init( dlgsetup, uiTextFile::Setup(true,false,dlgsetup.wintitle_) );
}


void uiTextFileDlg::init( const uiTextFileDlg::Setup& dlgsetup,
			  const uiTextFile::Setup& tsetup )
{
    scroll2bottom_ = dlgsetup.scroll2bottom_;

    editor_ = new uiTextFile( this, tsetup );
    editor_->fileNmChg.notify( mCB(this,uiTextFileDlg,fileNmChgd) );

    uiPopupMenu* filemnu = new uiPopupMenu( this, "&File" );
    if ( dlgsetup.allowopen_ )
	filemnu->insertItem( new uiMenuItem("&Open ...",
		    	     mCB(this,uiTextFileDlg,open)) );
    if ( dlgsetup.allowsave_ )
    {
	filemnu->insertItem( new uiMenuItem("&Save ...",
		    	     mCB(this,uiTextFileDlg,save)) );
	filemnu->insertItem( new uiMenuItem("Save &As ...",
		    	     mCB(this,uiTextFileDlg,saveAs)) );
    }
    filemnu->insertItem( new uiMenuItem("&Quit",
			 mCB(this,uiTextFileDlg,dismiss)) );
    menuBar()->insertItem( filemnu );
}


void uiTextFileDlg::fileNmChgd( CallBacker* )
{
    FilePath fp( editor_->fileName() );
    setName( fp.fileName() );
    setCaption( fp.fullPath() );
    setTitleText( editor_->fileName() );
}


void uiTextFileDlg::open( CallBacker* )
{
    uiFileDialog dlg( this, uiFileDialog::ExistingFile,
		      editor_->fileName(), "", "Select file" );
    if ( dlg.go() )
    {
	editor_->open( dlg.fileName() );
	if ( scroll2bottom_ )
	    editor_->toLine( mUdf(int) );
    }
}


void uiTextFileDlg::save( CallBacker* )
{
    editor_->save();
}


void uiTextFileDlg::saveAs( CallBacker* )
{
    uiFileDialog dlg( this, uiFileDialog::AnyFile,
	    	      editor_->fileName(), "", "Select new file name" );
    if ( dlg.go() )
	editor_->saveAs( dlg.fileName() );
}


void uiTextFileDlg::dismiss( CallBacker* )
{
    accept( this );
}


bool uiTextFileDlg::rejectOK( CallBacker* cb )
{
    if ( !cancelpushed_ )
	return acceptOK( cb );

    if ( !okToExit() )
	return false;

    if ( !editor_->reLoad() )
	doMsg( "Cannot re-load file. Possibly the file no longer exists." );

    return false;
}


bool uiTextFileDlg::acceptOK( CallBacker* )
{
    return okToExit();
}


int uiTextFileDlg::doMsg( const char* msg, bool iserr )
{
    int ret = 0;

    uiMsgMainWinSetter setter( this );

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

    BufferString msg( "File:\n" );
    msg += editor_->fileName();
    msg += "\nwas modified. Save now?";
    int opt = doMsg( msg, false );
    if ( opt == 2 )
	return false;
    else if ( opt == 0 )
    {
	if ( !editor_->save() )
	{
	    doMsg( "Could not save file. Please try 'Save As'" );
	    return false;
	}
    }

    return true;
}
