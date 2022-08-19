/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiofferinfo.h"
#include "uitextedit.h"


uiOfferInfoWin::uiOfferInfoWin( uiParent* p, const uiString& captn, int nrln )
    : uiMainWin(p,captn,0,false,false)
{
    setDeleteOnClose( true );

    uitb_ = new uiTextBrowser(this, mFromUiStringTodo(captn), mUdf(int), false);
    uitb_->setPrefHeightInChar( nrln );
    uitb_->setPrefWidthInChar( 80 );
}


void uiOfferInfoWin::setText( const char* txt )
{
    uitb_->setHtmlText( txt );
}


uiOfferInfo::uiOfferInfo( uiParent* p, bool setinsens )
	: uiToolButton(p,"info",tr("View info"),mCB(this,uiOfferInfo,infoReq) )
	, insens_(setinsens)
	, infowin_(0)
	, caption_(uiStrings::sInformation())
{
    setInfo( 0, uiStrings::sInformation() );
}


void uiOfferInfo::updateWin()
{
    if ( infowin_ )
    {
	if ( info_.isEmpty() )
	    { delete infowin_; infowin_ = 0; }
	else
	{
	    infowin_->setCaption( caption_ );
	    infowin_->setText( info_ );
	}
    }
}


void uiOfferInfo::setInfo( const char* txt, const uiString& captn )
{
    info_ = txt;
    if ( !captn.isEmpty() )
	caption_ = captn;

    const bool isactive = !info_.isEmpty();
    if ( insens_ )
	setSensitive( isactive );
    else
	display( isactive );

    updateWin();
    setToolTip( toUiString(txt) );
}


void uiOfferInfo::infoReq( CallBacker* )
{
    if ( !infowin_ )
    {
	if ( info_.isEmpty() )
	    return;

	int nrlines = info_.count( '\n' ) + 1;
	if ( nrlines < 5 ) nrlines = 5;
	if ( nrlines > 20 ) nrlines = 20;
	infowin_ = new uiOfferInfoWin( mainwin(), caption_, nrlines );
	infowin_->windowClosed.notify( mCB(this,uiOfferInfo,winClose) );
	infowin_->show();
    }

    updateWin();
    if ( infowin_ )
	infowin_->raise();
}


void uiOfferInfo::winClose( CallBacker* )
{
    infowin_ = 0;
}
