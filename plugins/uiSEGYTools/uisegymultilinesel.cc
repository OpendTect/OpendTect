/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    int nrrows = filespec_.nrFiles();
    if ( needwcsel )
	nrrows++;

    const int nrcols = needwcsel ? nrwc_ + 3 : 3;
    tbl_ = new uiTable( this, uiTable::Setup(nrrows,nrcols).defrowlbl(true)
				.rowdesc(uiStrings::sFile())
				.defrowstartidx(needwcsel? 0 : 1),
				"File Table" );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidthInChar( 120 );
    tbl_->setPrefHeightInRows( mMIN(nrrows,15) );
    tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->setColumnStretchable( nrcols-2, true );
    tbl_->setColumnLabel( 0, uiStrings::sFileName() );
    tbl_->setColumnLabel( nrcols-1, tr("Line Status") );
    tbl_->setColumnToolTip( nrcols-1,
	    tr("Tells if line geometry already exists in the survey") );
    tbl_->setColumnLabel( nrcols-2, needwcsel ? tr("Final Line name")
					      : uiStrings::sLineName() );

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

	tbl_->setRowReadOnly( 0, true );
	tbl_->setRowLabel( 0, uiString::empty() );
    }

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
	const int rowidx = needwcsel ? idx + 1 : idx;
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

    tbl_->resizeColumnsToContents();
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
    const int linenmcol = tbl_->nrCols() - 2;
    tbl_->resizeColumnsToContents();
    tbl_->setColumnStretchable( linenmcol, true );
    if ( rc.col() == linenmcol )
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
    RowCol rc( rowidx, linecol );
    const BufferString linenm = tbl_->text( rc );
    const Survey::Geometry* geom = Survey::GM().getGeometry( linenm.buf() );
    rc.col() = statuscol;
    if ( geom && geom->is2D() )
    {
	tbl_->setText( rc, tr("Exists") );
	tbl_->setColor( rc, OD::Color(180,255,180) );
    }
    else
    {
	tbl_->setText( rc, uiStrings::sNew() );
	tbl_->setColor( rc, OD::Color(180,240,255) );
    }

    tbl_->setCellReadOnly( rc, true );
}


bool uiSEGYMultiLineSel::acceptOK( CallBacker* )
{
    linenames_.setEmpty();
    const int linecol = tbl_->nrCols() - 2;
    const bool haswcselrow = nrwc_ > 1;
    for ( int idx=0; idx<filespec_.nrFiles(); idx++ )
    {
	const int rowidx = haswcselrow ? idx+1 : idx;
	linenames_.add( tbl_->text(RowCol(rowidx,linecol)) );
    }

    selwcidx_ = curwcidx_;
    return true;
}
