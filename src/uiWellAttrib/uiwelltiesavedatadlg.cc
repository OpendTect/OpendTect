/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiesavedatadlg.cc,v 1.1 2009-09-24 15:29:08 cvsbruno Exp $";

#include "uiwelltiesavedatadlg.h"

#include "wavelet.h"
#include "welltiedata.h"
#include "uitable.h"

static const char* colnms[] = { "Log Name", "Wavelet Name", 0 };
namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg( uiParent* p, const WellTie::DataHolder* dh )
    : uiDialog( p, uiDialog::Setup("Save current data","",mTODOHelpID) )
    , params_(*dh->dpms())  
{
    for ( int idx=0; colnms[idx]; idx++ )
    {
	tableset_ += new uiTable( this, uiTable::Setup()
				        .selmode(uiTable::SelectionMode(3)),
	       				"data table" );
	tableset_[idx]->setNrCols( 3 );
	tableset_[idx]->setColumnLabel( 0, colnms[idx] );
	tableset_[idx]->setColumnLabel( 1, "Specify name" );
	tableset_[idx]->setNrRows( 3 );
	tableset_[idx]->setColumnWidth(0,90);
	tableset_[idx]->setColumnWidth(1,90);
	tableset_[idx]->setTableReadOnly(true);
	if ( idx ) tableset_[idx]->attach( alignedBelow, tableset_[idx-1] );
    }

    BufferStringSet nms;

    nms.add( params_.ainm_ ); 	 	nms.add( dh->wvltset()[0]->name() ); 	
    nms.add( params_.refnm_ ); 		nms.add( dh->wvltset()[1]->name() );
    nms.add( params_.synthnm_ ); 

}


void uiSaveDataDlg::selDone( CallBacker* )
{
}

}; //namespace Well Tie
