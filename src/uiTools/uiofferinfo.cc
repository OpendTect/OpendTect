/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiofferinfo.h"
#include "uitextedit.h"


uiOfferInfoWin::uiOfferInfoWin( uiParent* p, const uiString& captn, int nrln )
    : uiMainWin(p,Setup(captn).withmenubar(false).nrstatusflds(0))
{
    uitb_ = new uiTextBrowser(this, mFromUiStringTodo(captn), mUdf(int), false);
    uitb_->setPrefHeightInChar( nrln );
    uitb_->setPrefWidthInChar( 80 );
    showAlwaysOnTop();
}


uiOfferInfoWin::~uiOfferInfoWin()
{}


void uiOfferInfoWin::setText( const char* txt )
{
    uitb_->setText( txt );
}



// uiOfferInfo
uiOfferInfo::uiOfferInfo( uiParent* p, bool setinsens )
	: uiToolButton(p,"info",tr("View info"),mCB(this,uiOfferInfo,infoReq) )
	, caption_(uiStrings::sInformation())
	, insens_(setinsens)
{
    setInfo( 0, uiStrings::sInformation() );
}


uiOfferInfo::~uiOfferInfo()
{}


void uiOfferInfo::updateWin()
{
    if ( !infowin_ )
	return;

    if ( info_.isEmpty() )
	deleteAndNullPtr( infowin_ );
    else
    {
	infowin_->setCaption( caption_ );
	infowin_->setText( info_ );
	infowin_->raise();
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
	if ( info_.startsWith("<html>") )
	    nrlines = 15; // hack for html text
	else
	{
	    if ( nrlines < 5 )
		nrlines = 5;
	    if ( nrlines > 20 )
		nrlines = 20;
	}

	infowin_ = new uiOfferInfoWin( mainwin(), caption_, nrlines );
	infowin_->windowClosed.notify( mCB(this,uiOfferInfo,winClose) );
	infowin_->show();
    }

    updateWin();
}


void uiOfferInfo::winClose( CallBacker* )
{
    infowin_ = nullptr;
}
