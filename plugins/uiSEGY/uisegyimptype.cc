/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegyimptype.h"
#include "uicombobox.h"
#include "survinfo.h"


SEGY::ImpType::ImpType( bool isvsp )
    : tidx_(0)
{
    init();
    if ( isvsp )
	tidx_ = types_.size() - 1;
}


SEGY::ImpType::ImpType( Seis::GeomType gt )
    : tidx_(0)
{
    init();
    setGeomType( gt );
}


void SEGY::ImpType::init()
{
    if ( SI().has3D() )
    {
	types_ += (int)Seis::Vol;
	types_ += (int)Seis::VolPS;
    }
    if ( SI().has2D() )
    {
	types_ += (int)Seis::Line;
	types_ += (int)Seis::LinePS;
    }
    types_ += (int)Seis::LinePS + 1;
}


void SEGY::ImpType::setGeomType( Seis::GeomType gt )
{
    for ( int idx=0; idx<types_.size()-1; idx++ )
    {
	if ( types_[idx] == (int)gt )
	    { tidx_ = idx; return; }
    }

    pErrMsg( "Setting type not in survey" );
}



uiSEGYImpType::uiSEGYImpType( uiParent* p, const uiString* lbltxt )
    : uiGroup(p,"Import Type")
    , typeChanged(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
					lbltxt ? *lbltxt : tr("Data type") );
    fld_ = lcb->box();
    fld_->setHSzPol( uiObject::MedVar );
    fld_->selectionChanged.notify( mCB(this,uiSEGYImpType,typChg) );

#   define mAddItem(txt,ic) { \
    fld_->addItem( tr(txt) ); \
    fld_->setIcon( fld_->size()-1, ic ); }

    for ( int idx=0; idx<typ_.types_.size(); idx++ )
    {
	const int typ = typ_.types_[idx];
	if ( typ == (int)Seis::Vol )
	{
	    fld_->addItem( tr("3D seismic data") );
	    fld_->setIcon( fld_->size()-1, "seismiccube" );
	}
	else if ( typ == (int)Seis::VolPS )
	{
	    fld_->addItem( tr("3D PreStack data") );
	    fld_->setIcon( fld_->size()-1, "prestackdataset" );
	}
	else if ( typ == (int)Seis::Line )
	{
	    fld_->addItem( tr("2D Seismic data") );
	    fld_->setIcon( fld_->size()-1, "seismicline2d" );
	}
	else if ( typ == (int)Seis::LinePS )
	{
	    fld_->addItem( tr("2D PreStack data") );
	    fld_->setIcon( fld_->size()-1, "prestackdataset2d" );
	}
	else
	{
	    fld_->addItem( tr("Zero-offset VSP") );
	    fld_->setIcon( fld_->size()-1, "vsp0" );
	}
    }


    setHAlignObj( fld_ );
}


const SEGY::ImpType& uiSEGYImpType::impType()
{
    typ_.tidx_ = fld_->currentItem();
    return typ_;
}


void uiSEGYImpType::setTypIdx( int tidx )
{
    typ_.tidx_ = tidx;
    fld_->setCurrentItem( typ_.tidx_ );
}
