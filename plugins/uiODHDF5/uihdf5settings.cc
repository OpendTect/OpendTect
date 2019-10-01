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
#include "uistrings.h"


uiHDF5Settings::uiHDF5Settings( uiSettingsGroup& pgrp )
    : uiSettingsSubGroup(pgrp)
    , usehdffld_(0)
    , initialglobenabled_(HDF5::Access::isEnabled())
{
    useflds_.setNullAllowed( true );

    if ( !HDF5::isAvailable() )
    {
	new uiLabel( this, HDF5::Access::sHDF5NotAvailable().quote(false) );
	return;
    }
    else if ( HDF5::isEnvBlocked() )
    {
	new uiLabel( this, envBlockStr(uiStrings::sAll(),0) );
	return;
    }

    usehdffld_ = new uiGenInput( this, tr("Create HDF5 data files"),
				 BoolInpSpec(initialglobenabled_) );
    mAttachCB( usehdffld_->valuechanged, uiHDF5Settings::useChgCB );

    uiStringSet uitypenms;
    typenms_.add( HDF5::sSeismicsType() );
    uitypenms.add( uiStrings::sSeismicData() );
    typenms_.add( HDF5::sPickSetType() );
    uitypenms.add( uiStrings::sPointSet(mPlural) );
    typenms_.add( HDF5::sWellType() );
    uitypenms.add( uiStrings::sWell(mPlural) );

    uiObject* lastfld = usehdffld_->attachObj();
    for ( int ityp=0; ityp<typenms_.size(); ityp++ )
    {
	const char* typnm = typenms_.get( ityp ).str();
	const uiString& uitypnm = uitypenms.get( ityp );
	const bool isenab = !setts_.isFalse( getSubKey(ityp) );
	initialenabled_ += isenab;
	uiObject* newfld;
	if ( HDF5::isEnvBlocked(typnm) )
	{
	    newfld = new uiLabel( this, envBlockStr(uitypnm,typnm) );
	    useflds_ += 0;
	}
	else
	{
	    uiGenInput* inpfld = new uiGenInput( this,
			tr("Use HDF5 for %1").arg(uitypnm),
			BoolInpSpec(isenab) );
	    useflds_ += inpfld;
	    newfld = inpfld->attachObj();
	}
	newfld->attach( alignedBelow, lastfld );
	lastfld = newfld;
    }

    setHAlignObj( usehdffld_ );
    addToParent( pgrp );
    mAttachCB( postFinalise(), uiHDF5Settings::useChgCB );
}


uiString uiHDF5Settings::envBlockStr( const uiString& what, const char* typ )
{
    BufferString envnm( "OD_NO_HDF5" );
    if ( typ && *typ )
	envnm.add( "_" ).add( typ ).toUpper();
    return tr("HDF5 file creation [%1] blocked (\"%2\" set)")
	    .arg( what ).arg( envnm );
}


void uiHDF5Settings::useChgCB( CallBacker* )
{
    const bool canuse = usehdffld_->getBoolValue();
    for ( auto fld : useflds_ )
	if ( fld )
	    fld->display( canuse );
}


BufferString uiHDF5Settings::getSubKey( int ityp ) const
{
    return BufferString( HDF5::Access::sSettingsEnabKey(), ".",
			 typenms_.get(ityp) );
}


bool uiHDF5Settings::commit( uiRetVal& uirv )
{
    if ( !usehdffld_ )
	return true;

    const bool douse = usehdffld_->getBoolValue();
    updateSettings( initialglobenabled_, douse,
		    HDF5::Access::sSettingsEnabKey() );

    for ( int ityp=0; ityp<typenms_.size(); ityp++ )
    {
	const BufferString ky = getSubKey( ityp );
	auto fld = useflds_[ityp];
	const bool usetyp = douse && fld && fld->getBoolValue();
	updateSettings( initialenabled_[ityp], usetyp, ky );
    }

    return true;
}
