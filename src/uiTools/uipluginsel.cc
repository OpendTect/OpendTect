/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uipluginsel.cc,v 1.9 2008-08-18 13:42:58 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uipluginsel.h"
#include "uibutton.h"
#include "uigroup.h"
#include "uilabel.h"
#include "plugins.h"
#include "settings.h"
#include "odver.h"
#include <math.h>

const char* uiPluginSel::sKeyDoAtStartup = "dTect.Select Plugins";


uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup("","","0.2.6")
			.savebutton(true)
			.savetext("Show this dialog at startup"))
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl += ": Candidate auto-loaded plugins";
    setCaption( titl );

    setCtrlStyle( uiDialog::LeaveOnly );
    setCancelText( "&Ok" );
    setSaveButtonChecked( true );

    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    BufferStringSet piusrnms;
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK()
	  && data.autotype_ == PI_AUTO_INIT_LATE )
	{
	    pluginnms_.add( data.name_ );
	    piusrnms.add( PIM().userName(data.name_) );
	}
    }
    piusrnms.sort( &pluginnms_ );

    const int maxlen = piusrnms.maxLength();
    const float rowspercol = maxlen / 10.;
    const int nrplugins = pluginnms_.size();
    int nrcols = (int)(Math::Sqrt( rowspercol * nrplugins ) + .5);
    if ( nrcols < 1 ) nrcols = 1;
    if ( nrcols > 3 ) nrcols = 3;
    int nrows = nrplugins / nrcols;
    if ( nrplugins % nrcols )
	nrows++;

    uiLabel* lbl = new uiLabel( this, "Please select the plugins to auto-load");
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );
    grp->attach( centeredBelow, lbl );

    for ( int idx=0; idx<nrplugins; idx++ )
    {
	const int colnr = idx / nrows;
	const int rownr = idx - colnr * nrows;

	BufferString dispnm = piusrnms.get( idx );
	uiCheckBox* cb = new uiCheckBox( grp, dispnm );
	cb->setChecked( true );
	cbs_ += cb;
	if ( colnr != nrcols - 1 )
	    cb->setPrefWidthInChar( maxlen+5 );
	if ( idx == 0 ) continue;

	if ( rownr == 0 )
	    cb->attach( rightOf, cbs_[(colnr-1)*nrows] );
	else
	    cb->attach( alignedBelow, cbs_[idx-1] );
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
