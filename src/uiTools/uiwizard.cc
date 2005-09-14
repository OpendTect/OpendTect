/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.cc,v 1.7 2005-09-14 08:28:05 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "uibutton.h"
#include "uicursor.h"
#include "uigroup.h"

uiWizard::uiWizard( uiParent* p, uiDialog::Setup& s_ )
    : uiDialog(p,s_)
    , pageidx(0)
    , rotatemode(false)
{
    finaliseDone.notify( mCB(this,uiWizard,doFinalise) );
}


void uiWizard::doFinalise( CallBacker* )
{
    isStarting();
    setCurrentPage( firstPage() );
    handleButtonText();
}


int uiWizard::addPage( uiGroup* page, bool display )
{
    if ( !page ) return -1;

    pages += page;
    dodisplay += display;
    return pages.size()-1;
}


void uiWizard::displayPage( int idx, bool yn )
{
    if ( idx<dodisplay.size() )
	dodisplay[idx] = yn;
}


bool uiWizard::acceptOK( CallBacker* )
{
    const int prevpage = pageidx;
    uiCursorChanger cursorchanger( uiCursor::Wait );
    bool firstpage = pageidx == firstPage();

    if ( !leavePage(pageidx,false) )
	return false;

    if ( pageidx==firstPage() )
	return isClosing(true);

    pageidx--;
    while ( !dodisplay[pageidx] && !pageidx )
	pageidx--;

    if ( displayCurrentPage() )
	handleButtonText();
    else
	pageidx = prevpage;

    return false;
}


bool uiWizard::rejectOK( CallBacker* )
{
    uiCursorChanger cursorchanger( uiCursor::Wait );
    if ( !leavePage(pageidx,true) )
	return false;

    if ( pageidx==lastPage() && !rotatemode )
	return isClosing(false);

    if ( pageidx==lastPage() && rotatemode ) 
    {
	reset();
	pageidx = 0;
    }
    else
	pageidx++;

    while ( !dodisplay[pageidx] && pageidx<pages.size() )
	pageidx++;

    handleButtonText();
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


void uiWizard::handleButtonText()
{
    bool firstpage = pageidx == firstPage();
    bool lastpage = pageidx == lastPage();
    const char* oktxt = firstpage ? "Cancel" : "<< Back";
    const char* canceltxt = lastpage && !rotatemode ? "Finish" : "Next >>";

    setButtonText( uiDialog::OK, oktxt );
    setButtonText( uiDialog::CANCEL, canceltxt );
}


void uiWizard::setRotateMode( bool yn )
{
    rotatemode=yn;
    handleButtonText();
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
