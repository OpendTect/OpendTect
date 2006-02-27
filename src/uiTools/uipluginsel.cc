/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.cc,v 1.2 2006-02-27 16:40:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uipluginsel.h"
#include "uibutton.h"
#include "plugins.h"
#include "settings.h"
#include <math.h>

const char* uiPluginSel::sKeyDoAtStartup = "dTect.Select Plugins";


uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup("OpendTect: Candidate auto-loaded plugins",
		    	   "Select OpendTect plugins to load","0.2.6")
			.savebutton(true)
			.savetext("Show this dialog at startup"))
{
    setCtrlStyle( uiDialog::LeaveOnly );
    setCancelText( "Ok" );
    setSaveButtonChecked( true );

    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    BufferStringSet piusrnms;
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	PluginManager::Data& data = *pimdata[idx];
	if ( data.handle_ )
	{
	    pluginnms_.add( data.name_ );
	    piusrnms.add( PIM().userName(data.name_) );
	}
    }

    const int maxlen = piusrnms.maxLength();
    const float rowspercol = maxlen / 10.;
    int nrcols = (int)(sqrt( rowspercol * pluginnms_.size() ) + .5);
    if ( nrcols < 1 ) nrcols = 1;
    if ( nrcols > 3 ) nrcols = 3;
    int nrows = pluginnms_.size() / nrcols;
    if ( pluginnms_.size() % nrcols )
	nrows++;

    for ( int idx=0; idx<pluginnms_.size(); idx++ )
    {
	BufferString dispnm = piusrnms.get( idx );
	uiCheckBox* cb = new uiCheckBox( this, dispnm );
	cb->setChecked( true );
	cb->setPrefWidthInChar( maxlen );
	cbs_ += cb;
	if ( !idx ) continue;

	if ( idx % nrows )
	    cb->attach( alignedBelow, cbs_[idx-1] );
	else
	{
	    int colnr = idx / nrows - 1;
	    cb->attach( rightOf, cbs_[colnr*nrows] );
	}
    }
}


bool uiPluginSel::rejectOK( CallBacker* )
{
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
	if ( !cbs_[idx]->isChecked() )
	{
	    PluginManager::Data* data = PIM().findData( pluginnms_.get(idx) );
	    if ( data )
		data->autosource_ = PluginManager::Data::None;
	}
    }

    if ( !saveButtonChecked() )
    {
	Settings::common().setYN( sKeyDoAtStartup, false );
	Settings::common().write();
    }

    return true;
}
