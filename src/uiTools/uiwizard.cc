/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwizard.h"

#include "uibutton.h"
#include "mousecursor.h"
#include "uigroup.h"

uiWizard::uiWizard( uiParent* p, uiDialog::Setup& s_ )
    : uiDialog(p,s_)
    , pageidx(0)
    , rotatemode(false)
{
    postFinalize().notify( mCB(this,uiWizard,doFinalize) );
}


uiWizard::~uiWizard()
{}


void uiWizard::doFinalize( CallBacker* )
{
    isStarting();
    setCurrentPage( firstPage() );
    updateButtonText();
}


int uiWizard::addPage( uiGroup* page, bool disp )
{
    if ( !page ) return -1;

    pages += page;
    dodisplay += disp;
    return pages.size()-1;
}


void uiWizard::displayPage( int idx, bool yn )
{
    if ( idx>=0 && idx<dodisplay.size() )
    {
	dodisplay[idx] = yn;
	updateButtonText();
    }
}


bool uiWizard::acceptOK( CallBacker* )
{
    const int prevpage = pageidx;
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    if ( !leavePage(pageidx,false) )
	return false;

    if ( pageidx==firstPage() )
	return isClosing(true);

    pageidx--;
    while ( !dodisplay[pageidx] && !pageidx )
	pageidx--;

    if ( displayCurrentPage() )
	updateButtonText();
    else
	pageidx = prevpage;

    return false;
}


bool uiWizard::rejectOK( CallBacker* cb )
{
    if ( cb!=button(uiDialog::CANCEL) )
    {
	leavePage(pageidx,false);
	isClosing(true);
	return true;
    }

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    if ( !leavePage(pageidx,true) )
	return false;

    if ( lastPage()==-1 || pageidx==lastPage() )
    {
	if ( rotatemode )
	{
	    reset();
	    pageidx = 0;
	}
	else
	{
	    return isClosing(false);
	}
    }
    else
	pageidx++;

    while ( !dodisplay[pageidx] && pageidx<pages.size() )
	pageidx++;

    updateButtonText();
    displayCurrentPage();
    return false;
}


bool uiWizard::displayCurrentPage()
{
    if ( !preparePage(pageidx) )
	return false;

    for ( int idx=0; idx<pages.size(); idx++ )
	pages[idx]->display( idx==pageidx );

    return true;
}


void uiWizard::setCurrentPage( int idx )
{
    pageidx = idx;
    displayCurrentPage();
}


void uiWizard::updateButtonText()
{
    bool firstpage = pageidx == firstPage();
    bool lastpage = pageidx == lastPage();
    const uiString oktxt = firstpage ? uiStrings::sCancel() : tr("<< Back");
    const uiString canceltxt = lastpage && !rotatemode ? uiStrings::sFinish()
			       : tr("Next >>");

    setButtonText( uiDialog::OK, oktxt );
    setButtonText( uiDialog::CANCEL, canceltxt );
}


void uiWizard::setRotateMode( bool yn )
{
    rotatemode=yn;
    updateButtonText();
}


int uiWizard::firstPage() const
{
    int firstidx = -1;
    for ( int idx=0; idx<pages.size(); idx++ )
    {
	if ( dodisplay[idx] )
	{
	    firstidx = idx;
	    break;
	}
    }

    return firstidx;
}


int uiWizard::lastPage() const
{
    int lastidx=-1;
    for ( int idx=0; idx<pages.size(); idx++ )
    {
	if ( dodisplay[idx] )
	    lastidx = idx;
    }

    return lastidx;
}
