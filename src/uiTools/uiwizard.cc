/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.cc,v 1.1 2004-03-22 16:15:45 nanne Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "uibutton.h"
#include "uigroup.h"

uiWizard::uiWizard( uiParent* p, uiDialog::Setup& s_ )
    : uiDialog(p,s_)
    , pageidx(0)
    , nextpage(this)
    , prevpage(this)
    , finished(this)
    , cancelled(this)
{
    handleButtonText();
}


int uiWizard::addPage( uiGroup* page, bool display )
{
    if ( !page ) return -1;

    page->display( !pages.size() );
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
    pageidx--;
    while ( !dodisplay[pageidx] && !pageidx )
	pageidx--;

    bool docancel = pageidx < 0;
    if ( docancel )
    {
	cancelled.trigger();
	return true;
    }

    handleButtonText();
    displayCurrentPage();
    prevpage.trigger();

    return false;
}


bool uiWizard::rejectOK( CallBacker* )
{
    pageidx++;
    while ( !dodisplay[pageidx] && pageidx<pages.size() )
	pageidx++;

    bool dofinish = pageidx == pages.size();
    if ( dofinish )
    {
	finished.trigger();
	return true;
    }

    handleButtonText();
    displayCurrentPage();
    nextpage.trigger();

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
    const char* canceltxt = lastpage ? "Finish" : "Next";

    setButtonText( uiDialog::OK, oktxt );
    setButtonText( uiDialog::CANCEL, canceltxt );
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
