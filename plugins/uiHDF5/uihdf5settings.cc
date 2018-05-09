/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2018
________________________________________________________________________

-*/

#include "uihdf5settings.h"

#include "uigeninput.h"
#include "uilabel.h"

#include "hdf5access.h"
#include "settings.h"


uiHDF5Settings::uiHDF5Settings( uiSettingsGroup& pgrp )
    : uiSettingsSubGroup(pgrp)
    , usehdffld_(0)
    , initialenabled_(HDF5::Access::isEnabled())
{
    if ( !HDF5::isAvailable() )
    {
	new uiLabel( this, HDF5::Access::sHDF5NotAvailable().quote(false) );
	return;
    }

    usehdffld_ = new uiGenInput( this, tr("Create HDF5 data files"),
				 BoolInpSpec(initialenabled_) );

    setHAlignObj( usehdffld_ );
    addToParent( pgrp );
}


bool uiHDF5Settings::commit( uiRetVal& uirv )
{
    if ( !usehdffld_ )
	return true;

    updateSettings( initialenabled_, usehdffld_->getBoolValue(),
		    HDF5::Access::sSettingsEnabKey() );
    return true;
}
