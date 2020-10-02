/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2015
________________________________________________________________________

-*/

#include "uisegyimptype.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "survinfo.h"
#include "keystrs.h"

static const char* sKeyImpTyp = "Import.Type";
static const char* sKeyVSP = "VSP";
static const int cVSPType = 4; // one after the last Seis::GeomType



SEGY::ImpType::ImpType( bool isvsp )
{
    init();
    if ( isvsp )
	tidx_ = types_.size() - 1;
}


SEGY::ImpType::ImpType( Seis::GeomType gt )
{
    init();
    setGeomType( gt );
}


void SEGY::ImpType::init()
{
    if ( SI().has2D() )
    {
	tidx_ = 0; // default is Line, unless there is 3D data
	types_ += (int)Seis::Line;
	types_ += (int)Seis::LinePS;
    }
    if ( SI().has3D() )
    {
	tidx_ = types_.size(); // When 3D data available, default is Vol
	types_ += (int)Seis::Vol;
	types_ += (int)Seis::VolPS;
    }
    types_ += cVSPType;
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


uiString SEGY::ImpType::dispText() const
{
    if ( isVSP() )
	return tr("Zero-offset VSP");

    switch ( geomType() )
    {
    case Seis::VolPS:
	return tr("3D Prestack Data");
    case Seis::Line:
	return tr("2D Seismic Data");
    case Seis::LinePS:
	return tr("2D Prestack Data");
    default: case Seis::Vol:
	return tr("3D Seismic Data");
    }
}


void SEGY::ImpType::fillPar( IOPar& iop ) const
{
    iop.set( sKeyImpTyp, isVSP() ? sKeyVSP : Seis::nameOf(geomType()) );
}



uiSEGYImpType::uiSEGYImpType( uiParent* p, bool withvsp,
				const uiString* lbltxt, bool defaultlbl )
    : uiGroup(p,"Import Type")
    , typeChanged(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
				    lbltxt ? *lbltxt : uiString::empty() );
    if (!lbltxt && defaultlbl )
	lcb->label()->setText( tr("Data type") );

    fld_ = lcb->box();
    fld_->setHSzPol( uiObject::MedVar );

#   define mAddItem(txt,ic) { \
    fld_->addItem( tr(txt) ); \
    fld_->setIcon( fld_->size()-1, ic ); }

    SEGY::ImpType imptyp;
    for ( int idx=0; idx<typ_.types_.size(); idx++ )
    {
	imptyp.tidx_ = idx;
	const bool isvsp = imptyp.isVSP();
	if ( !withvsp && isvsp )
	    continue;

	fld_->addItem( imptyp.dispText() );
	const int icidx = fld_->size() - 1;
	if ( isvsp )
	    fld_->setIcon( icidx, "vsp0" );
	else
	{
	    switch ( imptyp.geomType() )
	    {
	    case Seis::Vol:
		fld_->setIcon( icidx, "seismiccube" );		break;
	    case Seis::VolPS:
		fld_->setIcon( icidx, "prestackdataset" );	break;
	    case Seis::Line:
		fld_->setIcon( icidx, "seismicline2d" );	break;
	    case Seis::LinePS:
		fld_->setIcon( icidx, "prestackdataset2d" );	break;
	    }
	}
    }

    setTypIdx( typ_.tidx_ );
    fld_->selectionChanged.notify( mCB(this,uiSEGYImpType,typChg) );

    setHAlignObj( fld_ );
}


const SEGY::ImpType& uiSEGYImpType::impType() const
{
    typ_.tidx_ = fld_->currentItem();
    return typ_;
}


void uiSEGYImpType::setTypIdx( int tidx )
{
    typ_.tidx_ = tidx;
    fld_->setCurrentItem( typ_.tidx_ );
}


void uiSEGYImpType::setGeomType( Seis::GeomType gt )
{
    for ( int idx=0; idx<typ_.types_.size(); idx++ )
    {
	if ( typ_.types_[idx] == (int)gt )
	    { fld_->setCurrentItem( idx ); break; }
    }
}


void uiSEGYImpType::usePar( const IOPar& iop )
{
    BufferString res = iop.find( sKeyImpTyp );
    if ( res.isEmpty() || res == sKey::All() )
	res = iop.find( sKey::Geometry() );
    if ( res.isEmpty() )
	return;

    NotifyStopper ns( fld_->selectionChanged );
    if ( res == sKeyVSP )
	setTypIdx( fld_->size()-1 );
    else
	setGeomType( Seis::geomTypeOf(res) );
}
