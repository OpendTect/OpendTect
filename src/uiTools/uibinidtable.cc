/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id: uibinidtable.cc,v 1.1 2003-02-19 16:12:32 nanne Exp $
 ________________________________________________________________________

-*/

#include "uibinidtable.h"
#include "uitable.h"
#include "position.h"


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
    ObjectSet<BufferString> colnms;
    colnms += new BufferString("Inline");
    colnms += new BufferString("Crossline");

    ObjectSet<BufferString> rownms;
    for ( int idx=0; idx<nrrows+5; idx++ )
    {
	BufferString base( "Node " );
	base += idx;
	rownms += new BufferString( base );
    }

    table = new uiTable( this, "Table" );
    table->setColumnLabels( colnms );
    table->setRowLabels( rownms );

}
    

void uiBinIDTable::setBinIDs( const TypeSet<BinID>& bids )
{
    const int nrbids = bids.size();
    for ( int idx=0; idx<nrbids; idx++ )
    {
	const BinID bid = bids[idx];
	table->setText( idx, 0, BufferString(bid.inl) );
	table->setText( idx, 1, BufferString(bid.crl) );
    }
}


void uiBinIDTable::getBinIDs( TypeSet<BinID>& bids )
{
    const int nrrows = table->numRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BinID bid(0,0);
	BufferString inlstr = table->text(idx,0);
	BufferString crlstr = table->text(idx,1);
	if ( !(*inlstr) || !(*crlstr) )
	    continue;
	bid.inl = atoi(inlstr);
	bid.crl = atoi(crlstr);
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
