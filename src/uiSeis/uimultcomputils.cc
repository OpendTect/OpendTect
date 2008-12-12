/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultcomputils.cc,v 1.4 2008-12-12 15:00:30 cvshelene Exp $";

#include "uimultcompdlg.h"
#include "bufstringset.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "uilistbox.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, LineKey lkey )
    	: uiDialog(p,uiDialog::Setup("Component dialog",
		    		     "Choose the component to display","") )
	, needdisplay_(true)
	, compfld_(0)
{
    BufferStringSet complist;
    SeisIOObjInfo::getCompNames( lkey, complist );

    if ( complist.size()>1 )
	compfld_ = new uiListBox( this, complist, "Component nr" );
    else
	needdisplay_ = false;
}


int uiMultCompDlg::getCompNr() const
{
    return compfld_->indexOf( compfld_->getText() );
}
