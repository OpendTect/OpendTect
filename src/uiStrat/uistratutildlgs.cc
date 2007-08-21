/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          August 2007
 RCS:		$Id: uistratutildlgs.cc,v 1.1 2007-08-21 12:40:10 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratutildlgs.h"

#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilistbox.h"


uiStratUnitDlg::uiStratUnitDlg( uiParent* p, bool insert )
    : uiDialog(p,uiDialog::Setup("Create Stratigraphic Unit","", 0))
    , insertbafld_(0)
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitnmfld_ );
    sellithbut_ = new uiPushButton( this, "&Select", 
	    			    mCB(this,uiStratUnitDlg,selLithCB), false );
    sellithbut_->attach( rightTo, unitlithfld_ );
    if ( insert )
    {
	insertbafld_ = new uiGenInput( this, "Insert unit",
				       BoolInpSpec(true,"Before","After") );
	insertbafld_->attach( alignedBelow, unitlithfld_ );
    }
    
}


void uiStratUnitDlg::selLithCB( CallBacker* )
{
}


uiLithoDlg::uiLithoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Select Lithology","", 0))
{
    BufferStringSet lithoset;
    for ( int idx=0; idx<Strat::UnRepo().nrLiths(); idx++ )
	lithoset.add( Strat::UnRepo().lith(idx).name() );
    
    listlithfld_ = new uiLabeledListBox( p, lithoset, "Existing lithologies:" );
    newlithbut_ = new uiPushButton( this, "&Create new", 
	    			    mCB(this,uiLithoDlg,newLithCB), true );
    newlithbut_->attach( rightTo, listlithfld_ );
    lithnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    lithnmfld_->attach( alignedBelow, newlithbut_ );
    lithnmfld_->display(false);
}


void uiLithoDlg::newLithCB( CallBacker* )
{
    lithnmfld_->display(true);
}
