
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

static const char* rcsID = "$Id: uibouncymain.cc,v 1.2 2012/07/16 06:22:47 cvskris Exp $";

#include "uibouncymain.h"
#include "uibouncysettingsdlg.h"
#include "uimsg.h"
#include "uigeninput.h"

namespace uiBouncy
{
uiBouncyMain::uiBouncyMain( uiParent* p, uiBouncySettingsDlg** bsdlg )
	: uiDialog( p, Setup( "Bouncy start", mNoDlgTitle, mNoHelpID ) )
	, bsdlg_(bsdlg)
{
    namefld_ = new uiGenInput( this, "Enter your name", StringInpSpec("") );
    if ( !(*bsdlg_) )
    {
	*bsdlg_ = new uiBouncySettingsDlg( this );
    }
    (*bsdlg_)->attach ( alignedBelow, namefld_ );
}


uiBouncyMain::~uiBouncyMain()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiBouncyMain::acceptOK( CallBacker* )
{
    if ( strcmp( namefld_->text(), "") == 0 ) 
    {
	mErrRet( "Enter your name!" );
	return false;
    }
    else if ( !(*bsdlg_)->isOK() )
	return false;
    else 
	return true;
}


BufferString uiBouncyMain::getPlayerName() const
{
    return namefld_->text();
}


void uiBouncyMain::setPlayerName( const BufferString nm )
{
    namefld_->setText( nm );
}

}

