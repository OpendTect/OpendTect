/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sep 2020
________________________________________________________________________

-*/

#include "uisegymultilinesel.h"

#include "uibutton.h"
#include "uipixmap.h"
#include "uitable.h"

#include "filepath.h"
#include "segyfiledef.h"
#include "survgeom.h"


uiSEGYMultiLineSel::uiSEGYMultiLineSel( uiParent* p, const SEGY::FileSpec& fs,
					int& wcidx, BufferStringSet& linenames )
    : uiDialog(p,uiDialog::Setup(tr("Select Line names"),
				 tr("Review/Edit Line names"),
				 mNoHelpKey))
    , filespec_(fs)
    , linenames_(linenames)
    , selwcidx_(wcidx)
    , curwcidx_(wcidx)
{
    BufferString userfilenm = FilePath( filespec_.usrStr() ).fileName();
    nrwc_ = userfilenm.count( '*' );
    const bool needwcsel = nrwc_ > 1;

    const int nrrows = filespec_.nrFiles() + 1;
    const int nrcols = needwcsel ? nrwc_ + 3 : 3;
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,nrcols), "File Table" );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidthInChar( 20*nrcols );
    tbl_->setPrefHeightInRows( nrrows );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setRowLabel( 0, uiString::empty() );
    tbl_->setColumnLabel( 0, uiStrings::sFileName() );
    tbl_->setColumnLabel( nrcols-1, tr("Line exists") );
    tbl_->setColumnLabel( nrcols-2, tr("Final Line name") );

    if ( needwcsel )
    {
	for ( int idx=0; idx<nrwc_; idx++ )
	{
	    tbl_->setColumnLabel( idx+1, tr("Wild card %1").arg(idx+1) );
	    auto uicb = new uiCheckBox( 0, tr("Use as Line name"),
				    mCB(this,uiSEGYMultiLineSel,checkCB) );
	    tbl_->setCellObject( RowCol(0,idx+1), uicb );
	    checkboxes_ += uicb;
	    tbl_->setColumnReadOnly( idx+1, true );
	}
    }

    tbl_->setRowReadOnly( 0, true );
    tbl_->setColumnReadOnly( 0, true );
    tbl_->setColumnReadOnly( nrcols-1, true );
    tbl_->valueChanged.notify( mCB(this,uiSEGYMultiLineSel,lineEditCB) );

    initTable();
}


uiSEGYMultiLineSel::~uiSEGYMultiLineSel()
{
}


void uiSEGYMultiLineSel::initTable()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    BufferString userfilenm = FilePath( filespec_.usrStr() ).fileName();
    char* lastdot = userfilenm.findLast( '.' );
    if ( lastdot )
	*lastdot = '\0';

    BufferStringSet userfnmparts;
    BufferString filebuf( userfilenm );
    for ( int idx=0; idx<nrwc_; idx++ )
    {
	BufferString str( filebuf );
	char* wc = str.find( '*' );
	if ( wc )
	{
	    filebuf = wc + 1;
	    *wc = '\0';
	}

	if ( !str.isEmpty() )
	    userfnmparts.add( str );

	userfnmparts.add( "*" );
    }

    if ( !filebuf.isEmpty() )
	userfnmparts.add( filebuf );

    const bool needwcsel = nrwc_ > 1;
    const int linecol = tbl_->nrCols() - 2;
    for ( int idx=0; idx<filespec_.nrFiles(); idx++ )
    {
	const int rowidx = idx + 1;
	tbl_->setRowLabel( rowidx, toUiString(rowidx) );
	BufferString filenm( FilePath(filespec_.fileName(idx)).fileName() );
	tbl_->setText( RowCol(rowidx,0), filenm );
	lastdot = filenm.findLast( '.' );
	if ( lastdot )
	    *lastdot = '\0';

	BufferString prevpart;
	int colidx = 1;
	for ( int idy=0; idy<userfnmparts.size(); idy++ )
	{
	    const BufferString nmpart = userfnmparts.get( idy );
	    if ( nmpart == "*" )
	    {
		if ( idy == userfnmparts.size()-1 ) // Trailing *
		{
		    if ( needwcsel )
			tbl_->setText( RowCol(rowidx,colidx), filenm );
		    else
			tbl_->setText( RowCol(rowidx,linecol), filenm );
		}
	    }
	    else
	    {
		prevpart = filenm;
		char* str = prevpart.find( nmpart );
		if ( str )
		{
		    filenm = str + nmpart.size();
		    *str = '\0';
		}

		if ( prevpart.isEmpty() )
		    continue;

		if ( needwcsel )
		    tbl_->setText( RowCol(rowidx,colidx++), prevpart );
		else if ( idy == userfnmparts.size()-1 )
		    tbl_->setText( RowCol(rowidx,linecol), prevpart );
	    }
	}

	if ( linenames_.validIdx(idx) )
	    tbl_->setText( RowCol(rowidx,linecol), linenames_.get(idx) );
    }

    if ( !checkboxes_.isEmpty() )
    {
	if ( curwcidx_ < 0 )
	    curwcidx_ = guessWCIdx();

	if ( linenames_.isEmpty() )
	    checkboxes_[curwcidx_]->setChecked( true );
	else
	{
	    NotifyStopper ns( checkboxes_[curwcidx_]->activated );
	    checkboxes_[curwcidx_]->setChecked( true );
	}
    }

    tbl_->setColumnStretchable( linecol, true );
}


void uiSEGYMultiLineSel::checkCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,box,cb)
    if ( !cb )
	return;

    if ( !box->isChecked() )
    {
	NotifyStopper ns( box->activated );
	box->setChecked( true );
	return;
    }

    curwcidx_ = checkboxes_.indexOf( box );
    if ( curwcidx_ < 0 )
	return;

    for ( int idx=0; idx<checkboxes_.size(); idx++ )
    {
	if ( idx == curwcidx_ )
	    continue;

	NotifyStopper ns( checkboxes_[idx]->activated );
	checkboxes_[idx]->setChecked( false );
    }

    const int selcolidx = curwcidx_ + 1;
    const int linecol = nrwc_ + 1;
    for ( int rowidx=1; rowidx<tbl_->nrRows(); rowidx++ )
    {
	const BufferString sellinenm = tbl_->text( RowCol(rowidx,selcolidx) );
	tbl_->setText( RowCol(rowidx,linecol), sellinenm );
	updateLineAvailability( rowidx );
    }

}


void uiSEGYMultiLineSel::lineEditCB( CallBacker* )
{
    const RowCol& rc = tbl_->notifiedCell();
    if ( rc.col() == tbl_->nrCols()-2 )
	updateLineAvailability( rc.row() );
}


int uiSEGYMultiLineSel::guessWCIdx() const
{
    int retidx = -1;
    int maxscore = -1;
    for ( int idx=0; idx<nrwc_; idx++ )
    {
	int score = 0;
	for ( int rowidx=1; rowidx<tbl_->nrRows(); rowidx++ )
	{
	    const BufferString linenm = tbl_->text( RowCol(rowidx,idx+1) );
	    const Survey::Geometry* geom =
			Survey::GM().getGeometry( linenm.buf() );
	    if ( geom && geom->is2D() )
		score++;
	}

	if ( score > maxscore )
	{
	    retidx = idx;
	    maxscore = score;
	}
    }

    return retidx;
}


void uiSEGYMultiLineSel::updateLineAvailability( int rowidx )
{
    const int linecol = tbl_->nrCols() - 2;
    const int statuscol = tbl_->nrCols() - 1;
    if ( rowidx > 0 )
    {
	const BufferString linenm = tbl_->text( RowCol(rowidx,linecol) );
	const Survey::Geometry* geom = Survey::GM().getGeometry( linenm.buf() );
	if ( geom && geom->is2D() )
	    tbl_->setPixmap( RowCol(rowidx,statuscol), uiPixmap("checkgreen") );
	else
	    tbl_->clearCell( RowCol(rowidx,statuscol) );
    }
}


bool uiSEGYMultiLineSel::acceptOK( CallBacker* )
{
    linenames_.setEmpty();
    const int linecol = tbl_->nrCols() - 2;
    for ( int idx=1; idx<tbl_->nrRows(); idx++ )
	linenames_.add( tbl_->text(RowCol(idx,linecol)) );

    selwcidx_ = curwcidx_;
    return true;
}
