/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tableviewcommands.h"
#include "cmdrecorder.h"

#include "cmddriverbasics.h"

#include "uiaction.h"
#include "uitableview.h"
#include <string.h>


namespace CmdDrive
{

#define mHdrText( mdl, hdror, idx ) \
( mdl->headerText(idx,hdror).getFullString().buf() )

static const char* tabletagstr[5] = { "row", "row-head", "cell", "col-head",
				      "column" };

bool TableViewCmd::parTableSelPre( const char* prefix, TableViewTag tag,
		    const uiTableView* table, const BufferString& itemstr,
		    int itemnr, TypeSet<RowCol>& itemrcs, bool ambicheck )
{
    const TableModel* model = table->getModel();
    if ( !mIsUdf(itemnr) )
    {
	BufferString wildcardchecktxt;
	int nrfound = 0;
	const bool matchall = mSearchKey(itemstr).isMatching( "\a" );

	const int outersz = tag<CellTag ? model->nrCols() : model->nrRows();
	const int innersz = tag<CellTag ? model->nrRows() : model->nrCols();
	for ( int fidx=(tag==CellTag ? 0 : -1); fidx<outersz; fidx++ )
	{
	    for ( int fidy=0; fidy<innersz; fidy++ )
	    {
		const int idx = itemnr<0 && tag==CellTag ?
					   outersz-fidx-1 : fidx;
		const int idy = itemnr<0 ? innersz-fidy-1 : fidy;

		if ( idx<0 && tag<CellTag &&
		     !table->isHeaderVisible(OD::Vertical) )
		    break;
		if ( idx<0 && tag>CellTag &&
		     !table->isHeaderVisible(OD::Horizontal) )
		    break;

		RowCol rc = tag<CellTag ? RowCol(idy,idx) : RowCol(idx,idy);
		if ( rc.row()>=0 && table->isRowHidden(rc.row()) )
		    continue;
		if ( rc.col()>=0 && table->isColumnHidden(rc.col()) )
		    continue;

		BufferString itemtxt =
		    idx>=0 ? model->getCellData(rc.row(),rc.col()).text()
		     : ( tag<CellTag ? mHdrText(model,OD::Vertical,idy)
				      : mHdrText(model,OD::Horizontal,idy) );
		if ( tag==RowTag )
		    rc.col() = -1;
		if ( tag==ColTag )
		    rc.row() = -1;
		if ( mSearchKey(itemstr).isMatching(itemtxt) )
		{
		    nrfound++;
		    if ( abs(itemnr)==nrfound )
		    {
			itemrcs += rc;
			wildcardchecktxt = itemtxt;
		    }
		    else if ( !itemnr && nrfound==1 )
		    {
			itemrcs += matchall ? RowCol(-1,-1) : rc;
			wildcardchecktxt = itemtxt;
		    }
		    else if ( !itemnr && !ambicheck && !matchall )
			itemrcs += rc;
		}
	    }
	    if ( tag==CellTag )
		continue;
	    if ( tag==RowHead || tag==ColHead || nrfound )
		break;
	}
	mParStrErrRet( BufferString(prefix,tabletagstr[tag]), nrfound, 0,
		       itemstr, itemnr, "string", ambicheck, false );
	wildcardMan().check( mSearchKey(itemstr), wildcardchecktxt );
    }
    return true;
}


bool TableViewCmd::isSelected( const uiTableView* table, const RowCol& rc ) const
{
    const TableModel* model = table->getModel();
    if ( rc.row()>=0 && rc.col()>=0 )
	return table->isCellSelected( rc );

    bool res = false;
    if ( rc.row()<0 && rc.col()>=0 )
    {
	for ( int row=0; row<model->nrRows(); row++ )
	{
	    if ( table->isRowHidden(row) )
	       continue;
	    if ( !table->isCellSelected(RowCol(row,rc.col())) )
		return false;

	    res = true;
	}
	return res;
    }

    if ( rc.row()>=0 && rc.col()<0 )
    {
	for ( int col=0; col<model->nrCols(); col++ )
	{
	    if ( table->isColumnHidden(col) )
	       continue;
	    if ( !table->isCellSelected(RowCol(rc.row(),col)) )
		return false;

	    res = true;
	}
	return res;
    }

    return res;
}


RowCol TableViewCmd::singleSelected( const uiTableView* table ) const
{
    const TableModel* model = table->getModel();
    int nrselected = 0;
    RowCol selrc;
    for ( int row=0; row<model->nrRows(); row++ )
    {
	for ( int col=0; col<model->nrCols(); col++ )
	{
	    if ( isSelected(table, RowCol(row,col)) )
	    {
		nrselected++;
		selrc = RowCol( row, col );
	    }
	}
    }

    if ( nrselected < 2 )
	return nrselected ? selrc : RowCol(-1,-1);

    for ( int row=0; row<model->nrRows(); row++ )
    {
	if ( isSelected(table, RowCol(row,-1)) != (row==selrc.row()) )
	{
	    for ( int col=0; col<model->nrCols(); col++ )
	    {
		if ( isSelected(table, RowCol(-1,col)) != (col==selrc.col()) )
		    return RowCol( -1, -1 );
	    }
	    return RowCol( -1, selrc.col() );
	}
    }

    return RowCol( selrc.row(), -1 );
}


#define mParTableSelPre( prefix,tag,table,itemstr,itemnr,itemrcs,ambicheck ) \
\
    TypeSet<RowCol> itemrcs; \
    if ( !parTableSelPre(prefix,tag,table,itemstr,itemnr,itemrcs,ambicheck) ) \
	return false;


#define mParTableTag( parstr, parnext, tabletag, cellonly ) \
\
    TableViewTag tabletag = RowTag; \
    BufferString tagstr; \
    const char* parnext = getNextNonBlanks( parstr, tagstr.getCStr() ); \
\
    if ( mMatchCI(tagstr,"RowHead") || mMatchCI(tagstr,"ColHead") ) \
    { \
	if ( cellonly ) \
	{ \
	    mParseErrStrm << "This command does not support header selection" \
			  << od_endl; \
	    return false; \
	} \
	tabletag = mMatchCI(tagstr,"RowHead") ? RowHead : ColHead; \
    } \
    else if ( mMatchCI(tagstr,"Cell") ) \
	tabletag = CellTag; \
    else if ( !iswalpha(tagstr[0]) ) \
	parnext = parstr; \
    else \
    { \
	mParseErrStrm << "Selection option not in " \
		      << (cellonly ? "[Cell]" : "[RowHead, ColHead, Cell}") \
		      << od_endl; \
	return false; \
    }


#define mRowColCheck( tag, itemnr, parstr, parnext ) \
\
    if ( tag != RowTag ) \
    { \
	itemnr = mUdf(int); \
	parnext = parstr; \
    }

bool TableViewClickCmd::act( const char* parstr )
{
    mParKeyStrInit( "table", parstr, parnext, keys, selnr );
    mParTableTag( parnext, parnexxt, tag, false );
    mParItemSelInit( tabletagstr[tag], parnexxt, parnexxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxt, parnexxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxt, parnexxxxt );
    mParMouse( parnexxxxt, partail, clicktags, "Left" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    mActivate( TableView, Activator(*uitable, rc, clicktags) );
    return true;
}


TableViewActivator::TableViewActivator( const uiTableView& uitable, const RowCol& rc,
				const BufferStringSet& clicktags )
    : acttable_( const_cast<uiTableView&>(uitable) )
    , actrc_( rc )
    , actclicktags_( clicktags )
{}


#define mSelectBlock( lowrc, highrc, toggle ) \
{ \
    acttable_.selectionChanged.disable(); \
    if ( !toggle ) \
	acttable_.clearSelection(); \
    RowCol rc; \
    for ( rc.row()=lowrc.row(); rc.row()<=highrc.row(); rc.row()++ ) \
    { \
	for ( rc.col()=lowrc.col(); rc.col()<=highrc.col(); rc.col()++ ) \
	{ \
	    acttable_.setCellSelected( rc, !toggle || !acttable_.isCellSelected(rc) ); \
	} \
    } \
    acttable_.setCurrentCell( lowrc ); \
    acttable_.selectionChanged.enable(); \
    acttable_.selectionChanged.trigger(); \
}


void TableViewActivator::actCB( CallBacker* )
{
    const TableModel* model = acttable_.getModel();
    if ( actrc_.row()<model->nrRows() && actrc_.col()<model->nrCols() )
    {
	bool ctrlclicked = actclicktags_.isPresent( "Ctrl" );

	if ( actrc_.row()==-1 && actrc_.col()>=0 )
	{
	    if ( acttable_.maxNrOfSelections()>0 &&
		 acttable_.getSelectionBehavior()!=uiTableView::SelectRows )
	    {
		mSelectBlock( RowCol(0, actrc_.col()),
			     RowCol(model->nrRows()-1, actrc_.col()),
			      ctrlclicked );
	    }
	    acttable_.columnClicked.trigger( actrc_.col() );
	}
	else if ( actrc_.row()>=0 && actrc_.col()==-1 )
	{
	    if ( acttable_.maxNrOfSelections()>0 &&
		 acttable_.getSelectionBehavior()!=uiTableView::SelectColumns )
	    {
		mSelectBlock( RowCol(actrc_.row(), 0),
			     RowCol(actrc_.row(), model->nrCols()-1),
			      ctrlclicked );
	    }
	    acttable_.rowClicked.trigger( actrc_.row() );
	}
	else if ( actrc_.row()>=0 && actrc_.col()>=0 )
	{
	    if ( acttable_.maxNrOfSelections() > 0 )
	    {
		if ( !ctrlclicked && actclicktags_.isPresent("Right") &&
		     acttable_.isCellSelected(actrc_) )
		{
		    acttable_.setCurrentCell( actrc_ );
		}
		else
		    mSelectBlock( actrc_, actrc_, ctrlclicked );
	    }
//	    acttable_.setNotifiedCell( actrc_ );

/*
	    if ( actclicktags_.isPresent("Left") )
		acttable_.leftClicked.trigger();
	    if ( actclicktags_.isPresent("Right") )
		acttable_.rightClicked.trigger();
*/

	    if ( actclicktags_.isPresent("Double") )
		acttable_.doubleClicked.trigger();

	}
    }
}


bool TableViewFillCmd::act( const char* parstr )
{
    mParKeyStrInit( "table", parstr, parnext, keys, selnr );
    mParTableTag( parnext, parnexxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxt, parnexxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxt, parnexxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxt, parnexxxxt );
    mParInputStr( "table cell", parnexxxxt, partail, inpstr, false );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];
    mActivate( TableViewFill, Activator(*uitable, rc, inpstr) );
    return true;
}


TableViewFillActivator::TableViewFillActivator( const uiTableView& uitable,
					const RowCol& rc,
					const char* txt )
    : acttable_( const_cast<uiTableView&>(uitable) )
    , actrc_( rc )
    , acttxt_( txt )
{}


void TableViewFillActivator::actCB( CallBacker* )
{
    TableModel* model = acttable_.getModel();
    if ( actrc_.row()>=0 && actrc_.row()<model->nrRows() &&
	 actrc_.col()>=0 && actrc_.col()<model->nrCols() )
    {
	model->setCellData( actrc_.row(), actrc_.col(), acttxt_.buf() );
    }
}


bool TableViewExecCmd::act( const char* parstr )
{
    mParKeyStrInit( "table", parstr, parnext, keys, selnr );
    mParTableTag( parnext, parnexxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxt, parnexxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxt, parnexxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxt, parnexxxxt );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    uiObject* localsearchenv = nullptr;
    if ( !localsearchenv )
    {
	mWinErrStrm << "No UI-objects found in table cell" << od_endl;
	return false;
    }

    return doLocalAction( localsearchenv, parnexxxxt );
}


#define mSelBehavior( uitable, itemarg, rowarg, colarg ) \
    ( uitable->getSelectionBehavior()==uiTableView::SelectRows ? rowarg : \
    ( uitable->getSelectionBehavior()==uiTableView::SelectColumns ? colarg \
								  : itemarg ) )

bool TableViewSelectCmd::act( const char* parstr )
{
    mParKeyStrInit( "table", parstr, parnext, keys, selnr );
    mParTableTag( parnext, parnexxt, tag1, false );
    mParItemSelInit( BufferString("first ",tabletagstr[tag1]), parnexxt,
		     parnexxxt, itemstr11, itemnr11, false );
    mParItemSelInit( BufferString("first ",tabletagstr[ColTag]), parnexxxt,
		     parnexxxxt, itemstr12, itemnr12, tag1!=RowTag );
    mRowColCheck( tag1, itemnr12, parnexxxt, parnexxxxt );

    bool newcelltag = false;
    TableViewTag tag2 = tag1;
    if ( tag1==RowTag || tag1==CellTag )
    {
	BufferString argword;
	const char* extraparstr = getNextNonBlanks( parnexxxxt,
						    argword.getCStr() );
	if ( mMatchCI(argword,"Cell") )
	{
	    newcelltag = true;
	    tag2 = CellTag;
	    mSkipBlanks( extraparstr );
	    parnexxxxt = extraparstr;
	}
    }
    mParItemSelInit( BufferString("last ",tabletagstr[tag2]), parnexxxxt,
		     parnexxxxxt, itemstr21, itemnr21, !newcelltag );

    const bool optional = tag2!=RowTag || parnexxxxxt==parnexxxxt;
    mParItemSelInit( BufferString("last ",tabletagstr[ColTag]), parnexxxxxt,
		     parnexxxxxxt, itemstr22, itemnr22, optional );
    if ( tag1==CellTag && !newcelltag && parnexxxxxxt!=parnexxxxxt )
	tag2 = RowTag;

    mRowColCheck( tag2, itemnr22, parnexxxxxt, parnexxxxxxt );
    mParOnOffInit( parnexxxxxxt, partail, onoff );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    if ( uitable->maxNrOfSelections() == 0 )
    {
	mWinErrStrm << "This table allows no selection at all" << od_endl;
	return false;
    }

    if ( !mIsUdf(itemnr21) )
    {
	if ( !itemnr11 ) itemnr11 = 1;
	if ( !itemnr12 ) itemnr12 = 1;
	if ( !itemnr21 ) itemnr21 = -1;
	if ( !itemnr22 ) itemnr22 = -1;
    }

    mParTableSelPre("first ", tag1, uitable,itemstr11,itemnr11,itemrcs11,false);
    mParTableSelPre("first ",ColTag,uitable,itemstr12,itemnr12,itemrcs12,false);
    mParTableSelPre("last ",  tag2, uitable,itemstr21,itemnr21,itemrcs21,false);
    mParTableSelPre("last ", ColTag,uitable,itemstr22,itemnr22,itemrcs22,false);

    const TableModel* model = uitable->getModel();
    const int nrrows = model->nrRows();
    const int nrcols = model->nrCols();
    TypeSet<int> selcount( mSelBehavior(uitable,1,nrrows,nrcols) , 0 );
    TypeSet<int> unselcount( mSelBehavior(uitable,1,nrrows,nrcols) , 0 );

    TableViewState selset( uitable );
    for ( int col=nrcols-1; col>=0; col-- )
    {
	for ( int row=nrrows-1; row>=0; row-- )
	{
	    const RowCol rc( row, col );
	    bool specified;

	    if ( mIsUdf(itemnr21) )
	    {
		const RowCol itemrc( tag1==ColHead ? -1 : row,
				     tag1<CellTag  ? -1 : col );

		specified = tag1!=RowTag || itemrcs12[0]==RowCol(-1,-1) ||
			    itemrcs12.isPresent(RowCol(-1,col));
		specified = specified && ( itemrcs11[0]==RowCol(-1,-1) ||
					   itemrcs11.isPresent(itemrc) );
	    }
	    else
	    {
		const int firstrow = tag1==ColHead ? 0 : itemrcs11[0].row();
		const int firstcol = tag2==RowHead ? 0 :
				   ( tag2==RowTag  ? itemrcs12[0].col() :
						     itemrcs11[0].col() );

		const int lastrow = tag2==ColHead ? nrrows-1
						  : itemrcs21[0].row();
		const int lastcol = tag2==RowHead ? nrcols-1 :
				  ( tag2==RowTag  ? itemrcs22[0].col() :
						    itemrcs21[0].col() );

		if ( firstrow <= lastrow )
		    specified = row>=firstrow && row<=lastrow;
		else
		    specified = row>=firstrow || row<=lastrow;

		if ( firstcol <= lastcol )
		    specified = specified &&  col>=firstcol && col<=lastcol;
		else
		    specified = specified && (col>=firstcol || col<=lastcol);
	    }

	    if ( !uitable->isRowHidden(row) && !uitable->isColumnHidden(col) )
		unselcount[ mSelBehavior(uitable,0,row,col) ]++;

	    if ( !onoff    && !specified )
		continue;
	    if ( onoff==-1 && (specified || !uitable->isCellSelected(rc)) )
		continue;
	    if ( onoff==1  && !specified && !uitable->isCellSelected(rc)  )
		continue;

	    selset.headInsert( rc );

	    if ( !uitable->isRowHidden(row) && !uitable->isColumnHidden(col) )
	    {
		unselcount[ mSelBehavior(uitable,0,row,col) ]--;
		selcount[   mSelBehavior(uitable,0,row,col) ]++;
	    }
	}
    }

    int nrselected = 0;
    if ( mSelBehavior(uitable,false,true,true) )
    {
	for ( int idx=0; idx<selcount.size(); idx++ )
	{
	    if ( selcount[idx] && unselcount[idx] )
	    {
		mWinErrStrm << "This table only allows selection of whole "
			    << mSelBehavior( uitable, "", "rows", "columns" )
			    << od_endl;
		return false;
	    }
	    if ( selcount[idx] > 0 )
		nrselected++;
	}
    }
    else
	nrselected = selcount[0];

    if ( uitable->maxNrOfSelections()==1 && nrselected!=1 )
    {
	mWinErrStrm << "This single-selection table does not allow "
		    << nrselected << " selected "
		    << mSelBehavior( uitable, "cells", "rows", "columns" )
		    << od_endl;
	return false;
    }


    if ( selset.equalToCurItemSel() )
    {
	mWinWarnStrm << "Table already showed the specified selection"
		     << od_endl;
    }

    mActivate( TableViewSelect, Activator(*uitable, selset.getSet()) );

    if ( model->nrRows()==nrrows && model->nrCols()==nrcols
				   && !selset.equalToCurItemSel() )
    {
	mWinWarnStrm << "Specified selection has been overruled" << od_endl;
    }

    return true;
}


TableViewSelectActivator::TableViewSelectActivator( const uiTableView& uitable,
					    const TypeSet<RowCol>& selset )
    : acttable_( const_cast<uiTableView&>(uitable) )
    , actselset_( selset )
{}


void TableViewSelectActivator::actCB( CallBacker* )
{
    if ( acttable_.maxNrOfSelections()>0 )
    {
	acttable_.selectionChanged.disable();
	acttable_.clearSelection();
	int idx = 0;
	while ( idx < actselset_.size() )
	{
	    if ( actselset_[idx].col() < 0 )
	    {
		for ( int row=actselset_[idx].row();
		      row<=actselset_[idx+1].row();
		      row-=actselset_[idx].col() )
		{
		    RowCol rc( row, actselset_[idx+1].col() );
		    acttable_.setCellSelected( rc, true );
		}
		idx++;
	    }
	    else
		acttable_.setCellSelected( actselset_[idx], true );

	    idx++;
	}
	acttable_.selectionChanged.enable();
	acttable_.selectionChanged.trigger();
    }
}


bool TableViewMenuCmd::act( const char* parstr )
{
    mParKeyStrInit( "table", parstr, parnext, keys, selnr );
    mParTableTag( parnext, parnexxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxt, parnexxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxt, parnexxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxt, parnexxxxt );
    mParMouse( parnexxxxt, parnexxxxxt, clicktags, "Right" );
    mParPathStrInit( "menu", parnexxxxxt, parnexxxxxxt, menupath );
    mParOnOffInit( parnexxxxxxt, partail, onoff );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    prepareIntercept( menupath, onoff );

    mActivate( TableView, Activator(*uitable, rc, clicktags) );

    BufferString objnm = "Table cell (";
    objnm += rc.row()+1; objnm += ","; objnm += rc.col()+1; objnm += ")";
    return didInterceptSucceed( objnm );
}


#define mCountRowsOrCols( funcname, sizefunc, ishiddenfunc ) \
\
static int funcname( const uiTableView* uitable, int rcidx=mUdf(int) ) \
{ \
    const TableModel* model = uitable->getModel(); \
    if ( !mIsUdf(rcidx) && (rcidx<0 || rcidx>=model->sizefunc()) ) \
	return 0; \
\
    int count = 0; \
    for ( int idx=0; idx<model->sizefunc(); idx++ ) \
    { \
	if ( uitable->ishiddenfunc(idx) ) continue; \
\
	count++; \
	if ( idx == rcidx ) break; \
    } \
    return count; \
}

mCountRowsOrCols( countRows, nrRows, isRowHidden );
mCountRowsOrCols( countCols, nrCols, isColumnHidden );


bool NrTableViewRowsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, partail, keys, selnr );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParIdentPost( identname, countRows(uitable), parnext );
    return true;
}


bool NrTableViewColsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, partail, keys, selnr );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParIdentPost( identname, countCols(uitable), parnext );
    return true;
}


bool IsTableViewItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, false );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, partail,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, partail );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];
    const int ison = uitable->maxNrOfSelections()<=0  ? -1 :
		     isSelected(uitable, rc) ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


bool CurTableViewRowCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParFramed( parnexxt, parnexxxt, framed );
    mParExtraFormInit( parnexxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    const TableModel* model = uitable->getModel();
    const RowCol rc = framed ? uitable->currentCell() : singleSelected(uitable);
    const BufferString text =
	!uitable->isHeaderVisible(OD::Vertical) || rc.row()<0 ? "" :
				mHdrText( model, OD::Vertical, rc.row() );
//    const OD::Color color = uitable->getHeaderBackground( rc.col(), true );
//    mGetColorString( color, rc.col()>=0, colorstr );
    mParForm( answer, form, text, countRows(uitable,rc.row()) );
//    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


bool CurTableViewColCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParFramed( parnexxt, parnexxxt, framed );
    mParExtraFormInit( parnexxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    const TableModel* model = uitable->getModel();
    const RowCol rc = framed ? uitable->currentCell() : singleSelected(uitable);
    const BufferString text =
	uitable->isHeaderVisible(OD::Horizontal) || rc.col()<0 ? "" :
				mHdrText( model, OD::Horizontal, rc.col() );
//    const OD::Color color = uitable->getHeaderBackground( rc.col(), false );
//    mGetColorString( color, rc.col()>=0, colorstr );
    mParForm( answer, form, text, countCols(uitable,rc.col()) );
//    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


#define mGetTableItemNr( uitable, rc, itemnr ) \
\
    const int rownr = countRows( uitable, rc.row() ); \
    const int colnr = countCols( uitable, rc.col() ); \
    const int itemnr = (!rownr || !colnr) ? 0 : \
		       (rownr-1) * countCols(uitable) + colnr;

bool CurTableViewItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParFramed( parnexxt, parnexxxt, framed );
    mParExtraFormInit( parnexxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    RowCol selrc = singleSelected( uitable );
    if ( selrc.row()<0 || selrc.col()<0 )
	selrc = RowCol( -1, -1 );

    const RowCol rc = framed ? uitable->currentCell() : selrc;
    const TableModel* model = uitable->getModel();
    const TableModel::CellData cd = model->getCellData( rc.row(), rc.col() );
    mGetTableItemNr( uitable, rc, itemnr );
    mGetColorString( model->cellColor(rc.row(),rc.col()), rc.col()>=0,
		     colorstr );
    mParForm( answer, form, cd.text(), itemnr );
    mParExtraForm( answer, form, Color, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


bool GetTableViewRowCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, false );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParExtraFormInit( parnexxxxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    const TableModel* model = uitable->getModel();
    const BufferString text =
	!uitable->isHeaderVisible(OD::Vertical) || rc.row()<0 ? "" :
				mHdrText( model, OD::Vertical, rc.row() );

//    mGetColorString(uitable->getHeaderBackground(rc.col(),true),
//		    true, colorstr);
    mParForm( answer, form, text, countRows(uitable,rc.row()) );
//    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


bool GetTableViewColCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, false );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParExtraFormInit( parnexxxxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    const TableModel* model = uitable->getModel();
    const BufferString text =
	uitable->isHeaderVisible(OD::Horizontal) || rc.col()<0 ? "" :
				mHdrText( model, OD::Horizontal, rc.col() );

//    mGetColorString(uitable->getHeaderBackground(rc.col(),false),
//		    true, colorstr);
    mParForm( answer, form, text, countRows(uitable,rc.col()) );
//    mParExtraForm( answer, form, Colour, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


bool GetTableViewItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParExtraFormInit( parnexxxxxt, partail, form, "Number`Color" );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    const TableModel* model = uitable->getModel();
    const TableModel::CellData cd = model->getCellData( rc.row(), rc.col() );
    mGetTableItemNr( uitable, rc, itemnr );
    mGetColorString( model->cellColor(rc.row(),rc.col()), true, colorstr );
    mParForm( answer, form, cd.text(), itemnr );
    mParExtraForm( answer, form, Color, colorstr );
    mParEscIdentPost( identname.buf(), answer.buf(), parnext, form!=Color );
    return true;
}


#define mInterceptTableMenu( menupath, allowroot, uitable, rc ) \
\
    BufferStringSet clicktags; clicktags.add( "Right" ); \
    CmdDriver::InterceptMode mode = \
		    allowroot ? CmdDriver::NodeInfo : CmdDriver::ItemInfo; \
    prepareIntercept( menupath, 0, mode ); \
    mActivate( TableView, Activator(*uitable, rc, clicktags) ); \
    BufferString objnm = "Table cell ("; \
    objnm += rc.row()+1; objnm += ","; objnm += rc.col()+1; objnm += ")"; \
    if ( !didInterceptSucceed(objnm) ) \
	return false;

bool NrTableViewMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, partail, menupath );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    mInterceptTableMenu( menupath, true, uitable, rc );
    mParIdentPost( identname, interceptedMenuInfo().nrchildren_, parnext );
    return true;
}


bool IsTableViewMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, partail, menupath );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    mInterceptTableMenu( menupath, false, uitable, rc );
    mParIdentPost( identname, interceptedMenuInfo().ison_, parnext );
    return true;
}


bool GetTableViewMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "table", parnext, parnexxt, keys, selnr );
    mParTableTag( parnexxt, parnexxxt, tag, true );
    mParItemSelInit( tabletagstr[tag], parnexxxt, parnexxxxt,
		     itemstr1, itemnr1, false );
    mParItemSelInit( tabletagstr[ColTag], parnexxxxt, parnexxxxxt,
		     itemstr2, itemnr2, tag!=RowTag );
    mRowColCheck( tag, itemnr2, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, parnexxxxxxt, menupath );
    mParFormInit( parnexxxxxxt, partail, form );
    mParTail( partail );

    mFindListTableObjs( "table", objsfound, uiTableView, keys, nrgrey );
    mParKeyStrPre( "table", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTableView*, uitable, objsfound[0] );

    mParTableSelPre( "", tag, uitable, itemstr1, itemnr1, itemrcs1, true );
    mParTableSelPre( "", ColTag, uitable, itemstr2, itemnr2, itemrcs2, true );

    const RowCol rc = tag==RowTag ? RowCol(itemrcs1[0].row(),itemrcs2[0].col())
				  : itemrcs1[0];

    mInterceptTableMenu( menupath, false, uitable, rc );
    const MenuInfo menuinfo = interceptedMenuInfo();
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname.buf(), answer.buf(), parnext );
    return true;
}


//====== TableViewState class =====================================================

bool TableViewState::headInsert( const RowCol& rc )
{
    if ( set_.isEmpty() )
	set_ += rc;
    else
    {
	int step = 0;
	int col = set_[0].col();
	if ( col < 0 )
	{
	    if ( set_.size() < 2 )
		return false;

	    step = -col;
	    col = set_[1].col();
	}
	if ( rc.col()>col || (rc.col()==col && rc.row()>=set_[0].row()) )
	    return false;

	if ( rc.col() != col )
	    set_.insert( 0, rc );
	else if ( !step )
	    set_.insert( 0, RowCol(rc.row(), rc.row()-set_[0].row()) );
	else if ( rc.row()+step == set_[0].row() )
	    set_[0].row() = rc.row();
	else
	    set_.insert( 0, rc );
    }
    return true;
}


int TableViewState::remove( const RowCol& rc, int startidx )
{
    const int idx = indexOf( rc, startidx );

    if ( idx >= 0 )
    {
	if ( set_[idx].col() < 0 )
	{
	    const int step = -set_[idx].col();
	    if ( rc.row()+step < set_[idx+1].row() )
		set_.insert( idx+1, RowCol(rc.row()+step, -step) );
	    else if ( rc.row()+step > set_[idx+1].row() )
		set_.removeSingle( idx+1 );

	    if ( rc.row()-step >= set_[idx].row() )
		set_.insert( idx+1, RowCol(rc.row()-step, rc.col()) );
	    if ( rc.row()-step <= set_[idx].row() )
		set_.removeSingle( idx );
	}
	else
	    set_.removeSingle( idx );
    }

    return idx;
}


int TableViewState::indexOf( const RowCol& rc, int startidx ) const
{
    const int sz = set_.size();
    for ( int idx=(startidx<0 ? 0 : startidx); idx<sz; idx++ )
    {
	int step = 1;
	int col = set_[idx].col();
	const int row0 = set_[idx].row();
	int row1 = row0;
	const int res = idx;

	if ( col < 0 )
	{
	    idx++;
	    if ( idx >= sz )
		return -1;

	    step = -col;
	    col = set_[idx].col();
	    row1 = set_[idx].row();
	}
	if ( rc.col()<col || (rc.col()==col && rc.row()<row0) )
	    return -1;

	if ( rc.col()==col && rc.row()<=row1 && (rc.row()-row0)%step==0 )
	    return res;
    }
    return -1;
}


#define mBodyTableViewStateEqualTo( selcondition ) \
{ \
    if ( !table_ ) \
	return false; \
\
    const TableModel* model = table_->getModel(); \
    int startidx = 0; \
    RowCol rc; \
    for ( rc.col()=0; rc.col()<model->nrCols(); rc.col()++ ) \
    { \
	if ( table_->isColumnHidden(rc.col()) ) \
	    continue; \
\
	for ( rc.row()=0; rc.row()<model->nrRows(); rc.row()++ ) \
	{ \
	    if ( table_->isRowHidden(rc.row()) ) \
		continue; \
\
	    const int idx = indexOf( rc, startidx ); \
	    if ( (idx<0) == selcondition ) \
		return false; \
\
	    if ( idx >= 0 ) \
		startidx = idx; \
	} \
    } \
    return true; \
}

bool TableViewState::equalToCurItemSel() const
mBodyTableViewStateEqualTo( table_->isCellSelected(rc) )

#define mIsCellMatch( rc1, rc2 ) \
    ( (rc1.row()==-1 || rc2.row()==-1 || rc1.row()==rc2.row()) && \
      (rc1.col()==-1 || rc2.col()==-1 || rc1.col()==rc2.col()) )

bool TableViewState::equalToClickedItem( const RowCol& clickedrc ) const
mBodyTableViewStateEqualTo( mIsCellMatch(clickedrc, rc) )


#define mBodyStoreTableViewState( selcondition ) \
{ \
    if ( !table_ ) \
	return false; \
\
    const TableModel* model = table_->getModel(); \
    clear(); \
    RowCol rc; \
    for ( rc.col()=model->nrCols()-1; rc.col()>=0; rc.col()-- ) \
    { \
	for ( rc.row()=model->nrRows()-1; rc.row()>=0; rc.row()-- ) \
	{ \
	    if ( selcondition ) \
		headInsert( rc ); \
	} \
    } \
    return true; \
}

bool TableViewState::storeCurItemSel()
mBodyStoreTableViewState( table_->isCellSelected(rc) )

bool TableViewState::setAll()
mBodyStoreTableViewState( true )


//====== CmdComposers =========================================================


#define mGetHeaderItemSel( uitable, hdror, sizefunc, textfunc, \
			   itemhiddenfunc, curitemidx, curitemsel, casedep ) \
\
    BufferString curitemsel; \
    bool casedep = false; \
    if ( curitemidx >= 0 ) \
    { \
	BufferString curitemname = !uitable->isHeaderVisible(hdror) ? "*" : \
				   mHdrText( model, hdror, curitemidx ); \
	mDressNameString( curitemname, sItemName ); \
	int nrmatches = 0; \
	int selnr = 0; \
\
	for ( int itmidx=0; itmidx<model->sizefunc(); itmidx++ ) \
	{ \
	    if ( uitable->itemhiddenfunc(itmidx) ) \
		continue; \
	    const char* itmtxt = !uitable->isHeaderVisible(hdror) ? "*" : \
				 mHdrText( model, hdror, itmidx ); \
	    if ( SearchKey(curitemname,false).isMatching(itmtxt) ) \
	    { \
		if ( SearchKey(curitemname,true).isMatching(itmtxt) ) \
		{ \
		    nrmatches++; \
		    if ( itmidx == curitemidx ) \
			selnr = nrmatches; \
		} \
		else \
		    casedep = true; \
	    } \
	} \
\
	if ( selnr && nrmatches>1 ) \
	{ \
	    curitemname += "#"; curitemname += selnr; \
	} \
	if ( !uitable->isHeaderVisible(hdror) ) \
	{ \
	    curitemsel = " "; curitemsel += selnr; \
	} \
	else \
	{ \
	    curitemsel = " \""; curitemsel += curitemname; curitemsel += "\""; \
	} \
    }

#define mGetTopHeaderItemSel( uitable, curitemrc, curitemsel, casedep ) \
    mGetHeaderItemSel( uitable, OD::Horizontal, nrCols, columnLabel, \
		       isColumnHidden, curitemrc.col(), curitemsel, casedep )

#define mGetLeftHeaderItemSel( uitable, curitemrc, curitemsel, casedep ) \
    mGetHeaderItemSel( uitable, OD::Vertical, nrRows, rowLabel, \
		       isRowHidden, curitemrc.row(), curitemsel, casedep )


void TableViewCmdComposer::init()
{
    reInit();
    stagenr_ = -1;

    bursteventnames_.add( "selectionChanged" );
}


void TableViewCmdComposer::reInit()
{
    stagenr_ = 0;
    selchanged_ = false;
    clickedrc_ = RowCol(-2,-2);
    leftclicked_ = false;
    ctrlclicked_ = false;
    tablecmdsflushed_ = false;
}


#define mGetTable( uitable, retval ) \
\
    if ( eventlist_.isEmpty() ) \
	return retval; \
    mDynamicCastGet( const uiTableView*, uitable, eventlist_[0]->object_ ); \
    if ( !uitable ) \
	return retval;

void TableViewCmdComposer::updateInternalState()
{
    mGetTable( uitable, );
    if ( updateflag_ || CmdRecStopper::isInStopperList(uitable) )
    {
	storeTableState();
	updateflag_ = false;
    }
}


void TableViewCmdComposer::storeTableState()
{
    mGetTable( uitable, );
    selectedcells_.setTableView( uitable );
    selectedcells_.storeCurItemSel();
}


void TableViewCmdComposer::labelStoredStateOld()
{
    wasselectedcells_ = selectedcells_;
}


void TableViewCmdComposer::labelStoredStateNew()
{
    isselectedcells_ = selectedcells_;
}


#define mIsSet( iswasselected, cellrc ) \
    ( iswasselected##cells_.indexOf(cellrc)>=0 ? 1 : 0 )


void TableViewCmdComposer::writeTableSelect()
{
    if ( selchanged_ )
    {
	if ( stagenr_ < 2 )
	{
	    storeTableState();
	    labelStoredStateNew();
	}

	if ( stagenr_>1 && isselectedcells_.equalToClickedItem(clickedrc_) )
	    return;

	const int nrifdifferential = writeTableSelect( true, true );
	const int nrifclearedfirst = writeTableSelect( false, true );

	const bool differential = nrifclearedfirst==0 ||
				  nrifdifferential < nrifclearedfirst;

	writeTableSelect( differential );
    }
}


#define mGetFirstLastVisible( firstidx, lastidx, uitable, nrfunc, hiddenfunc ) \
\
    int firstidx = mUdf(int); \
    int lastidx = mUdf(int); \
    for ( int idx=0; idx<uitable->getModel()->nrfunc(); idx++ ) \
    { \
	if ( !uitable->hiddenfunc(idx) ) \
	{ \
	    lastidx = idx; \
	    if ( mIsUdf(firstidx) ) \
		firstidx = idx; \
	} \
    }

#define mInitStatesInLoop( oldstate, curstate, rc ) \
\
    if ( uitable->isRowHidden(rc.row()) || uitable->isColumnHidden(rc.col()) ) \
	continue; \
\
    const int oldstate = differential ? mIsSet(wasselected,rc) : 0; \
\
    int curstate = oldstate; \
    if ( rc.row()==model->nrRows() || accessible.indexOf(rc)<0 ) \
	curstate = mUdf(int); \
    else if ( !mIsCellMatch(clickedrc_, rc) ) \
	curstate = mIsSet( isselected, rc ); \
    else if ( !mIsSet(isselected, rc) ) \
	curstate = 1; \
    else if ( leftclicked_ || clickedrc_.row()==-1 || clickedrc_.col()==-1 ) \
	curstate = 0; \
    else if ( !mIsUdf(firstrow) ) \
	curstate = blockstate; \
\
    if ( !virtually && mIsCellMatch(clickedrc_, rc) ) \
	ctrlclicked_ = curstate != mIsSet(isselected, rc);


int TableViewCmdComposer::writeTableSelect( bool differential, bool virtually )
{
    mGetTable( uitable, -1 );
    TableViewState accessible( uitable );
    accessible.setAll();

    int nrtableselects = 0;

    const TableModel* model = uitable->getModel();
    RowCol rc0;
    for ( rc0.col()=0; rc0.col()<model->nrCols(); rc0.col()++ )
    {
	int topmargin = 0;
	int firstrow = mUdf(int);
	int lastrow = -1;
	int blockstate = -1;

	for ( rc0.row()=0; rc0.row()<=model->nrRows(); rc0.row()++ )
	{
	    mInitStatesInLoop( oldstate0, curstate0, rc0 );

	    if ( mIsUdf(firstrow) )
	    {
		if ( mIsUdf(curstate0) )
		    topmargin = rc0.row()+1;
		else if ( curstate0 != oldstate0 )
		{
		    firstrow = rc0.row();
		    lastrow = rc0.row();
		    blockstate = curstate0;
		}
		continue;
	    }

	    if ( curstate0 == blockstate )
	    {
		if ( curstate0 != oldstate0 )
		    lastrow = rc0.row();

		continue;
	    }

	    rc0.row()--;

	    int bottommargin = rc0.row();
	    int lastcol = rc0.col();

	    RowCol rc1;
	    for ( rc1.col()=rc0.col()+1; rc1.col()<model->nrCols();
		  rc1.col()++ )
	    {
		bool excludecurcol = false;
		bool includecurcol = false;
		for ( rc1.row()=firstrow; rc1.row()<=lastrow; rc1.row()++ )
		{
		    mInitStatesInLoop( oldstate1, curstate1, rc1 );
		    if ( curstate1 != blockstate  )
			excludecurcol = true;
		    if ( curstate1 != oldstate1 )
			includecurcol = true;
		}
		if ( excludecurcol )
		    break;
		if ( includecurcol )
		    lastcol = rc1.col();

		for ( rc1.row()=lastrow+1; rc1.row()<=bottommargin;
		      rc1.row()++ )
		{
		    mInitStatesInLoop( oldstate1, curstate1, rc1 );
		    if ( curstate1 != blockstate  )
		    {
			bottommargin = rc1.row()-1;
			break;
		    }
		    if ( curstate1 != oldstate1 )
			lastrow = rc1.row();
		}

		for ( rc1.row()=firstrow-1; rc1.row()>=topmargin; rc1.row()-- )
		{
		    mInitStatesInLoop( oldstate1, curstate1, rc1 );
		    if ( curstate1 != blockstate  )
		    {
			topmargin = rc1.row()+1;
			break;
		    }
		    if ( curstate1 != oldstate1 )
			firstrow = rc1.row();
		}

	    }

	    int startidx = 0;
	    for ( rc1.col()=rc0.col()+1; rc1.col()<=lastcol; rc1.col()++ )
	    {
		for ( rc1.row()=firstrow; rc1.row()<=lastrow; rc1.row()++ )
		{
		    startidx = accessible.remove( rc1, startidx );
		}
	    }


	    if ( !virtually )
	    {
		mGetFirstLastVisible( firstvisrowidx, lastvisrowidx,
				      uitable, nrRows, isRowHidden );
		mGetFirstLastVisible( firstviscolidx, lastviscolidx,
				      uitable, nrCols, isColumnHidden );

		const bool allormultiplerows = firstrow!=lastrow ||
					       firstvisrowidx==lastvisrowidx;

		const bool colhead = allormultiplerows && topmargin==0 &&
				     bottommargin==model->nrRows()-1;
		const bool rowhead = !colhead && rc0.col()==firstviscolidx &&
				     lastcol==lastviscolidx;

		const RowCol firstrc( colhead ? -1 : firstrow,
				      rowhead ? -1 : rc0.col()  );
		const RowCol lastrc(  colhead ? -1 : lastrow,
				      rowhead ? -1 : lastcol  );

		const bool clear = !differential && !nrtableselects;
		writeTableSelect( firstrc, lastrc, blockstate, clear );
	    }

	    nrtableselects++;
	    firstrow = mUdf(int);
	    topmargin = lastrow+1;
	}
    }
    return nrtableselects;
}


#define mHeaderCheck( rc, rowsel, colsel ) \
\
    if ( rc.row() == -1 ) \
	rowsel = " ColHead"; \
    else if ( rc.col() == -1 ) \
    { \
	colsel = rowsel; \
	rowsel = " RowHead"; \
    } \

void TableViewCmdComposer::writeTableSelect( const RowCol& firstrc,
					 const RowCol& lastrc,
					 int blockstate, bool clear )
{
    mGetTable( uitable, );
    const TableModel* model = uitable->getModel();

    mGetLeftHeaderItemSel( uitable, firstrc, rowsel1, rowcasedep1 );
    mGetTopHeaderItemSel(  uitable, firstrc, colsel1, colcasedep1 );
    mGetLeftHeaderItemSel( uitable, lastrc,  rowsel2, rowcasedep2 );
    mGetTopHeaderItemSel(  uitable, lastrc,  colsel2, colcasedep2 );
    mHeaderCheck( firstrc, rowsel1, colsel1 );
    mHeaderCheck( lastrc,  rowsel2, colsel2 );

    BufferString itemrg = rowsel1; itemrg += colsel1;
    if ( firstrc != lastrc )
    {
	if ( lastrc.row()>=0 && lastrc.col()>=0 )
	    itemrg += rowsel2;

	itemrg += colsel2;
    }

    const char* onoff = clear ? "" : ( blockstate ? " On" : " Off" );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, rowcasedep1 || colcasedep1 ||
					     rowcasedep2 || colcasedep2 );
    mRecOutStrm << "TableSelect \"" << ev.keystr_ << "\""
		<< itemrg << onoff << od_endl;
}


void TableViewCmdComposer::writeTableFill()
{
    mGetTable( uitable, );
    const TableModel* model = uitable->getModel();
    mGetLeftHeaderItemSel( uitable, clickedrc_, rowsel, rowcasedep );
    mGetTopHeaderItemSel( uitable, clickedrc_, colsel, colcasedep );

    const TableModel::CellData cd =
	model->getCellData( clickedrc_.row(), clickedrc_.col() );
    mGetInputString( inpptr, cd.text(), true );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, rowcasedep || colcasedep );
    mRecOutStrm << "TableFill \"" << ev.keystr_ << "\"" << rowsel << colsel
		<< " " << inpptr << od_endl;
}


#define mGetMouseTag( mousetag ) \
\
    BufferString mousetag = " "; \
    if ( ctrlclicked_ ) \
	mousetag += "Ctrl"; \
    if ( clickedrc_.row()>=0 && clickedrc_.col()>=0 ) \
    { \
	if ( stagenr_==3 || stagenr_==4 ) \
	    mousetag += "Double"; \
	mousetag += leftclicked_ ? "Left" : "Right"; \
    }

void TableViewCmdComposer::writeTableMenu( const CmdRecEvent& menuevent )
{
    mGetTable( uitable, );
    const TableModel* model = uitable->getModel();
    mGetLeftHeaderItemSel( uitable, clickedrc_, rowsel, rowcasedep );
    mGetTopHeaderItemSel( uitable, clickedrc_, colsel, colcasedep );
    mHeaderCheck( clickedrc_, rowsel, colsel );

    mGetMouseTag( mousetag );
    if ( mMatchCI(mousetag, " Right") )
	mousetag.setEmpty();

    const char* onoff = !menuevent.mnuitm_->isCheckable() ? "" :
			( menuevent.mnuitm_->isChecked() ? " On" : " Off" );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, menuevent.casedep_ || rowcasedep || colcasedep );
    mRecOutStrm << "TableMenu \"" << ev.keystr_ << "\"" << rowsel << colsel
		<< mousetag << " \"" << menuevent.menupath_ << "\""
		<< onoff << od_endl;
}


void TableViewCmdComposer::writeTableClick()
{
    if ( stagenr_ < 2 )
	return;

    mGetTable( uitable, );
    const TableModel* model = uitable->getModel();
    mGetLeftHeaderItemSel( uitable, clickedrc_, rowsel, rowcasedep );
    mGetTopHeaderItemSel( uitable, clickedrc_, colsel, colcasedep );
    mHeaderCheck( clickedrc_, rowsel, colsel );

    mGetMouseTag( mousetag );

    const CmdRecEvent& ev = *eventlist_[eventlist_.size()-1];
    insertWindowCaseExec( ev, rowcasedep || colcasedep );
    mRecOutStrm << "TableViewClick \"" << ev.keystr_ << "\"" << rowsel << colsel
		<< mousetag << od_endl;
}


bool TableViewCmdComposer::accept( const CmdRecEvent& ev )
{
    const bool accepted = CmdComposer::accept( ev );
    if ( quitflag_ || ignoreflag_ )
	return accepted;

    if ( !accepted && stagenr_<2 && !selchanged_ )
	return false;

    BufferString notifiername;
    char* msgnexxt; char* msgnexxxt;
    RowCol cellrc(-2,-2);

    if ( accepted )
    {
	if ( done() )
	{
	    updateflag_ = true;
	    notDone();
	}

	if ( ev.nraccepts_ )
	    return true;

	const char* msgnext = getNextNonBlanks( ev.msg_,
					        notifiername.getCStr() );
	cellrc.row() = strtol( msgnext, &msgnexxt, 0 );
	cellrc.col() = strtol( msgnexxt, &msgnexxxt, 0 );

	const bool notienter = mMatchCI(notifiername, "cellEntered") ||
			       mMatchCI(notifiername, "cellPressed") ||
			       mMatchCI(notifiername, "rowPressed")  ||
			       mMatchCI(notifiername, "columnPressed");
	if ( notienter && !ev.begin_ )
	{
	    shrinkEventList( 1, -3 );
	    voideventnames_.add( "cellEntered" );
	    voideventnames_.add( "cellPressed" );
	    voideventnames_.add( "rowPressed" );
	    voideventnames_.add( "columnPressed" );
	    stagenr_ = 0;
	    //mNotifyTest( uiTableView, ev.object_, selectionChanged );
	    return true;
	}
    }

    if ( ev.begin_ == ev.openqdlg_ )
	return accepted;

    if ( accepted )
    {
	const bool notileft = mMatchCI(notifiername, "leftClicked") ||
			      mMatchCI(notifiername, "rowClicked")  ||
			      mMatchCI(notifiername, "columnClicked");

	const bool notiright = mMatchCI( notifiername, "rightClicked" );

	if ( stagenr_ == -1 )
	    return true;

	if ( stagenr_ == 0 )
	{
	    shrinkEventList( 3, -2 );
	    labelStoredStateOld();
	    stagenr_ = 1;
	}

	if ( stagenr_ == 1 )
	{
	    if ( notileft || notiright )
	    {
		stagenr_ = 2;
		clickedrc_ = cellrc;
		leftclicked_ = notileft;
		ctrlclicked_ = false;
		storeTableState();
		labelStoredStateNew();
	    }
	    if ( mMatchCI(notifiername, "selectionChanged") )
		selchanged_ = true;

	    if ( mMatchCI(notifiername, "valueChanged") )
	    {
		stagenr_ = 5;
		clickedrc_ = cellrc;
	    }

	    return true;
	}

	const bool notidouble = mMatchCI(notifiername, "doubleClicked")    ||
				mMatchCI(notifiername, "rowDoubleClicked") ||
				mMatchCI(notifiername, "columnDoubleClicked");

	if ( stagenr_ == 2 )
	{
	    if ( notidouble && clickedrc_==cellrc )
	    {
		stagenr_ = 4;
		return true;
	    }
	}

	if ( stagenr_ == 3 )
	{
	    if ( notileft || notiright )
	    {
		if ( clickedrc_==cellrc && leftclicked_==notileft )
		    stagenr_ = 4;
	    }
	    return true;
	}
    }

    if ( !tablecmdsflushed_ )
    {
	writeTableSelect();

	if ( stagenr_ == 5 )
	    writeTableFill();
	else if ( ev.dynamicpopup_ )
	    writeTableMenu( ev );
	else
	    writeTableClick();

	tablecmdsflushed_ = true;
    }

    if ( stagenr_ != 3 )
    {
	reInit();
	if ( accepted )
	    return accept( ev );
    }

    return accepted;
}


void TableViewCmdComposer::getExecPrefix( CmdRecEvent& ev, const RowCol& rc )
{
    mDynamicCastGet( const uiTableView*, uitable, ev.object_ );
    if ( !uitable )
	return;

    const TableModel* model = uitable->getModel();
    mGetLeftHeaderItemSel( uitable, rc, rowsel, rowcasedep );
    mGetTopHeaderItemSel( uitable, rc, colsel, colcasedep );

    ev.casedep_ = ev.casedep_ || rowcasedep || colcasedep;

    ev.execprefix_ = "TableExec \""; ev.execprefix_ += ev.keystr_;
    ev.execprefix_ += "\""; ev.execprefix_ += rowsel;
    ev.execprefix_ += colsel; ev.execprefix_ += " ";
}


} // namespace CmdDrive
