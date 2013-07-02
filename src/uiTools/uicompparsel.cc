/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicompoundparsel.h"
#include "uigeninput.h"
#include "uibutton.h"


uiCompoundParSel::uiCompoundParSel( uiParent* p, const char* seltxt,
				    const char* btxt )
    : uiGroup(p,seltxt)
    , butPush(this)
{
    txtfld_ = new uiGenInput( this, seltxt, "" );
    txtfld_->setReadOnly( true );

    const char* buttxt = btxt ? btxt : "Select";
    selbut_ = new uiPushButton( this, buttxt, false );
    selbut_->setName( BufferString(buttxt," ",seltxt).buf() );
    selbut_->activated.notify( mCB(this,uiCompoundParSel,doSel) );
    selbut_->attach( rightOf, txtfld_ );

    setHAlignObj( txtfld_ );
    setHCenterObj( txtfld_ );

    postFinalise().notify( mCB(this,uiCompoundParSel,updSummary) );
}


void uiCompoundParSel::doSel( CallBacker* )
{
    butPush.trigger();
    updateSummary();
}


void uiCompoundParSel::updSummary( CallBacker* )
{
    txtfld_->setText( getSummary() );
}


void uiCompoundParSel::setSelText( const char* txt )
{
    txtfld_->setTitleText( txt );
}


uiCheckedCompoundParSel::uiCheckedCompoundParSel( uiParent* p,
			    const char* seltxt, bool invis, const char* btxt )
    : uiCompoundParSel(p,"",btxt)
    , mkinvis_(invis)
    , checked(this)
{
    cbox_ = new uiCheckBox( this, seltxt,
	   		    mCB(this,uiCheckedCompoundParSel,checkCB) );
    cbox_->attach( leftOf, txtfld_ );
}


void uiCheckedCompoundParSel::setChecked( bool yn )
{
    cbox_->setChecked( yn );
}


bool uiCheckedCompoundParSel::isChecked() const
{
    return cbox_->isChecked();
}


void uiCheckedCompoundParSel::checkCB( CallBacker* )
{
    const bool ischckd = cbox_->isChecked();
    if ( mkinvis_ )
    {
	txtfld_->display( ischckd );
	selbut_->display( ischckd );
    }
    else
    {
	txtfld_->setSensitive( ischckd );
	selbut_->setSensitive( ischckd );
    }

    checked.trigger();
}
