/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id: uibinidtable.cc,v 1.7 2003-11-07 12:22:02 bert Exp $
 ________________________________________________________________________

-*/

#include "uibinidtable.h"
#include "uitable.h"
#include "uimsg.h"
#include "position.h"
#include "survinfo.h"
#include "bufstringset.h"


uiBinIDTable::uiBinIDTable( uiParent* p, int nr )
    : uiGroup(p,"BinID table")
{
    init( nr );
}


uiBinIDTable::uiBinIDTable( uiParent* p, const TypeSet<BinID>& bids )
    : uiGroup(p,"BinID table")
{
    init( bids.size() );
    setBinIDs( bids );
}


void uiBinIDTable::init( int nrrows )
{
    BufferStringSet colnms;
    colnms += new BufferString("Inline");
    colnms += new BufferString("Crossline");

    table = new uiTable( this, uiTable::Setup().rowdesc("Node").rowcangrow(),
		         "Table" );


    table->setColumnLabels( colnms );
    table->setNrRows( nrrows+5 );
    nodeAdded();

    table->rowInserted.notify( mCB(this,uiBinIDTable,nodeAdded) );
}

void uiBinIDTable::nodeAdded(CallBacker*)
{
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString labl( "Node " );
	labl += idx;
	table->setRowLabel( idx, labl );
    }
}

    

void uiBinIDTable::setBinIDs( const TypeSet<BinID>& bids )
{
    const int nrbids = bids.size();
    for ( int idx=0; idx<nrbids; idx++ )
    {
	const BinID bid = bids[idx];
	table->setText( uiTable::RowCol(idx,0), BufferString(bid.inl) );
	table->setText( uiTable::RowCol(idx,1), BufferString(bid.crl) );
    }
}


void uiBinIDTable::getBinIDs( TypeSet<BinID>& bids )
{
    int nrrows = table->size().height();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BinID bid(0,0);
	BufferString inlstr = table->text(uiTable::RowCol(idx,0));
	BufferString crlstr = table->text(uiTable::RowCol(idx,1));
	if ( !(*inlstr) || !(*crlstr) )
	    continue;
	bid.inl = atoi(inlstr);
	bid.crl = atoi(crlstr);
	if ( !SI().isReasonable(bid) )
	{
	    Coord c( atof(inlstr), atof(crlstr) );
	    if ( SI().isReasonable(c) )
		bid = SI().transform(c);
	    else
	    {
		BufferString msg( "Position " );
		msg += bid.inl; msg += "/"; msg += bid.crl;
		msg += " is probably wrong.\nDo you wish to discard it?";
		if ( uiMSG().askGoOn(msg) )
		{
		    table->removeRow( idx );
		    nrrows--; idx--;
		    continue;
		}
	    }
	}
	SI().snap( bid );
	bids += bid;
    }
}


uiBinIDTableDlg::uiBinIDTableDlg( uiParent* p, const char* title, int nr )
    : uiDialog(p,uiDialog::Setup(title,"",""))
{
    table = new uiBinIDTable( this, nr );
}


uiBinIDTableDlg::uiBinIDTableDlg( uiParent* p, const char* title, 
				  const TypeSet<BinID>& bids )
    : uiDialog(p,uiDialog::Setup(title,"",""))
{
    table = new uiBinIDTable( this, bids );
}


void uiBinIDTableDlg::setBinIDs( const TypeSet<BinID>& bids )
{
    table->setBinIDs( bids );
}


void uiBinIDTableDlg::getBinIDs( TypeSet<BinID>& bids )
{
    table->getBinIDs( bids );
}
