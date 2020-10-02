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
#include "settings.h"


uiCompoundParSel::uiCompoundParSel( uiParent* p, const uiString& seltxt,
				    OD::StdActionType typ )
    : uiGroup(p,mFromUiStringTodo(seltxt))
    , butPush(this)
{
    crTextFld( seltxt );
    selbut_ = uiButton::getStd( this, typ, mCB(this,uiCompoundParSel,doSel),
				false );
    finishCreation( seltxt, selbut_->text() );
}


uiCompoundParSel::uiCompoundParSel( uiParent* p, const uiString& seltxt,
				    const uiString& btxt, const char* icid )
    : uiGroup(p,mFromUiStringTodo(seltxt))
    , butPush(this)
{
    crTextFld( seltxt );
    selbut_ = new uiPushButton( this, btxt, mCB(this,uiCompoundParSel,doSel),
				false );
    if ( icid )
	selbut_->setIcon( icid );
    finishCreation( seltxt, btxt );
}


uiCompoundParSel::~uiCompoundParSel()
{
}


void uiCompoundParSel::crTextFld( const uiString& seltxt )
{
    txtfld_ = new uiGenInput( this, seltxt, "" );
    txtfld_->setReadOnly( true );
}


void uiCompoundParSel::finishCreation( const uiString& seltxt,
				       const uiString& btxt )
{
    selbut_->attach( rightOf, txtfld_ );

    const uiString stnm = uiStrings::phrJoinStrings(btxt,seltxt);
    selbut_->setName( mFromUiStringTodo(stnm) );

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


void uiCompoundParSel::setSelText( const uiString& txt )
{
    txtfld_->setTitleText( txt );
}


void uiCompoundParSel::setSelIcon( const char* ident )
{
    selbut_->setIcon( ident );
}


uiCheckedCompoundParSel::uiCheckedCompoundParSel( uiParent* p,
			 const uiString& seltxt, bool invis,
			 const uiString& btxt )
    : uiCompoundParSel(p,uiStrings::sEmptyString(),btxt)
    , mkinvis_(invis)
    , checked(this)
{
    cbox_ = new uiCheckBox( this, seltxt );
    cbox_->setChecked( true );
    cbox_->activated.notify( mCB(this,uiCheckedCompoundParSel,checkCB) );
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
