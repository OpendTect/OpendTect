/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.cc,v 1.3 2004-04-28 12:41:10 nanne Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "uibutton.h"
#include "uigroup.h"

uiWizard::uiWizard( uiParent* p, uiDialog::Setup& s_ )
    : uiDialog(p,s_)
    , pageidx(0)
    , approved(true)
    , rotatemode(false)
    , next(this)
    , back(this)
    , finish(this)
    , cancel(this)
{
    finaliseDone.notify( mCB(this,uiWizard,doFinalise) );
}


void uiWizard::doFinalise( CallBacker* )
{
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
    if ( idx < dodisplay.size() )
	dodisplay[idx] = yn;
}


bool uiWizard::acceptOK( CallBacker* )
{
    approved = true;
    bool firstpage = pageidx == firstPage();
    if ( firstpage )
    {
	cancel.trigger();
	return approved;
    }

    back.trigger();
    if ( !approved )
	return false;

    pageidx--;
    while ( !dodisplay[pageidx] && !pageidx )
	pageidx--;

    handleButtonText();
    displayCurrentPage();
    return false;
}


bool uiWizard::rejectOK( CallBacker* )
{
    approved = true;
    bool lastpage = pageidx == lastPage();
    if ( lastpage && !rotatemode )
    {
	finish.trigger();
	return approved;
    }

    next.trigger();
    if ( !approved )
	return false;

    if ( lastpage && rotatemode ) 
	pageidx = 0;
    else
	pageidx++;

    while ( !dodisplay[pageidx] && pageidx<pages.size() )
	pageidx++;

    handleButtonText();
    displayCurrentPage();
    return false;
}


void uiWizard::displayCurrentPage()
{
    for ( int idx=0; idx<pages.size(); idx++ )
	pages[idx]->display( idx==pageidx );
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
    const char* oktxt = firstpage ? "Cancel" : "Back";
    const char* canceltxt = lastpage && !rotatemode ? "Finish" : "Next";

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
