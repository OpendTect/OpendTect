/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiwellpropertyrefsel.cc,v 1.15 2012-08-23 07:22:13 cvsbert Exp $";


#include "uiwellpropertyrefsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uiunitsel.h"
#include "uimsg.h"

#include "elasticprop.h"
#include "elasticpropsel.h"
#include "unitofmeasure.h"
#include "property.h"
#include "welllogset.h"
#include "welllog.h"


uiPropSelFromList::uiPropSelFromList( uiParent* p, const PropertyRef& pr,
	                                const PropertyRef* alternatepr )
    : uiGroup( p, pr.name() )
    , propref_(pr)
    , altpropref_(0)
    , checkboxfld_(0)
{
    typefld_ = new uiComboBox( this, BufferString(pr.name()," type") );
    typelbl_ = new uiLabel( this, pr.name(), typefld_ );

    unfld_ = new uiUnitSel( this, propref_.stdType(), 0, false, true );
    unfld_->setUnit( propref_.disp_.unit_ );
    unfld_->attach( rightOf, typefld_ );

    if ( alternatepr )
    {
	altpropref_ = new PropertyRef(*alternatepr);
	checkboxfld_ = new uiCheckBox( this, alternatepr->name() );
	checkboxfld_->attach( rightOf, unfld_ );
	checkboxfld_->activated.notify(mCB(this,uiPropSelFromList,switchPropCB));
    }
    setHAlignObj( typefld_ );
}


uiPropSelFromList::~uiPropSelFromList()
{
    delete altpropref_;
}


void uiPropSelFromList::switchPropCB( CallBacker* )
{
    if ( !altpropref_ ) return;
    const bool isaltprop = checkboxfld_->isChecked();
    const PropertyRef& pr = isaltprop ? *altpropref_ : propref_;

    unfld_->setPropType( pr.stdType() );
    unfld_->setUnit( pr.disp_.unit_ );
}


void uiPropSelFromList::setNames( const BufferStringSet& nms )
{
    typefld_->setEmpty();
    typefld_->addItems( nms );
}


void uiPropSelFromList::setCurrent( const char* lnm )
{
    typefld_->setCurrentItem( lnm );
}


void uiPropSelFromList::setUOM( const UnitOfMeasure& um )
{
    unfld_->setUnit( um.symbol() );
}


void uiPropSelFromList::set( const char* txt, bool alt, const UnitOfMeasure* um)
{
    setCurrent( txt ); setUseAlternate( alt );
    if ( um ) setUOM( *um );
}


void uiPropSelFromList::getData( BufferString& lognm, UnitOfMeasure& un ) const
{
    if ( unfld_->getUnit() )
	un = *unfld_->getUnit();

    lognm = typefld_->text();
}


const char* uiPropSelFromList::text() const
{
    return typefld_->text();
}


const UnitOfMeasure* uiPropSelFromList::uom() const
{
    return unfld_->getUnit();
}


void uiPropSelFromList::setUseAlternate( bool yn )
{
    if ( checkboxfld_ && altpropref_ )
	checkboxfld_->setChecked( yn );
}


bool uiPropSelFromList::isUseAlternate() const
{
    return checkboxfld_ ? checkboxfld_->isChecked() : false;
}


const PropertyRef& uiPropSelFromList::propRef() const
{
    return propref_;
}



uiWellPropSel::uiWellPropSel( uiParent* p, 
				const PropertyRefSelection& prs )
    : uiGroup(p," property selection from well logs")
    , proprefsel_(prs)  
{
    initFlds();
}


void uiWellPropSel::initFlds()
{
    for ( int idx=0; idx<proprefsel_.size(); idx ++ )
    {
	const PropertyRef& pr = *proprefsel_[idx];
	if ( pr.isThickness() )
	    continue;

	const PropertyRef* altpr = 0;
	//TODO check on something like 1/uom = uom for more generic ...
	const bool issonic = pr.hasType( PropertyRef::Son );
	const bool isvel = pr.hasType( PropertyRef::Vel );
	if ( issonic || isvel )
	    altpr = issonic ? new PropertyRef( "Velocity", PropertyRef::Vel )
			    : new PropertyRef( "Sonic", PropertyRef::Son ); 

	uiPropSelFromList* fld = new uiPropSelFromList( this, pr, altpr );
	if ( propflds_.size() > 0 )
	    fld->attach( alignedBelow, propflds_[propflds_.size()-1] );
	else
	    setHAlignObj( fld->typeFld() );

	delete altpr;
	propflds_ += fld;
    }
}


void uiWellPropSel::setLogs( const Well::LogSet& logs  )
{
    BufferStringSet lognms; 
    for ( int idx=0; idx<logs.size(); idx++ )
	lognms.add( logs.getLog(idx).name() );

    BufferStringSet allnms;
    allnms.add( sKeyPlsSel() );
    allnms.add( lognms, true );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	propflds_[idx]->setNames( allnms );
	bool found = false;
	const PropertyRef& propref = propflds_[idx]->propRef();
	const PropertyRef* altpropref = propflds_[idx]->altPropRef();
	for ( int idlog=0; idlog<logs.size(); idlog++ )
	{
	    const char* uomlbl = logs.getLog( idlog ).unitMeasLabel();
	    const UnitOfMeasure* um = UnitOfMeasure::getGuessed( uomlbl );
	    if ( um && propref.stdType() == um->propType() )
	    {
		propflds_[idx]->set( lognms[idlog]->buf(), false, um );
		found = true; break;
	    }
	}
	if ( !found && altpropref )
	{
	    for ( int idlog=0; idlog<logs.size(); idlog++ )
	    {
		const char* uomlbl = logs.getLog( idlog ).unitMeasLabel();
		const UnitOfMeasure* um = UnitOfMeasure::getGuessed( uomlbl );
		if ( um && altpropref->stdType() == um->propType())
		{
		    propflds_[idx]->set( lognms[idlog]->buf(), true, um );
		    break;
		}
	    }
	}
    }
}


bool uiWellPropSel::isOK() const
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( !strcmp ( propflds_[idx]->text(), sKeyPlsSel() ) )
	{
	    BufferString propnm( propflds_[idx]->propRef().name() );
	    BufferString msg( "Please select a log for " ); msg += propnm; 
	    uiMSG().error( msg.buf() ); return false;
	}
    }
    return true;
}


bool uiWellPropSel::setLog( const PropertyRef::StdType tp, 
				const char* nm, bool usealt,
				const UnitOfMeasure* uom )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( propflds_[idx]->propRef().hasType( tp ) )
	    propflds_[idx]->set( nm, usealt, uom );
    }
    return false;
}


bool uiWellPropSel::getLog( const PropertyRef::StdType tp, BufferString& bs, 
			bool& check, BufferString& uom ) const
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	const bool usealternate = propflds_[idx]->isUseAlternate();
	const PropertyRef* alternatepr = usealternate ? 
			 propflds_[idx]->altPropRef() : 0;
	if ( propflds_[idx]->propRef().hasType(tp) 
			|| (alternatepr && alternatepr->hasType(tp)) ) 
	{
	    bs = propflds_[idx]->text();
	    check = usealternate;
	    uom = propflds_[idx]->uom() ? propflds_[idx]->uom()->symbol() : ""; 
	    return true;
	}
    }
    return false;
}



uiWellElasticPropSel::uiWellElasticPropSel( uiParent* p, bool withswaves )
    : uiWellPropSel(p,*new ElasticPropSelection())
{
    propflds_[propflds_.size()-1]->display( withswaves );
}


uiWellElasticPropSel::~uiWellElasticPropSel()
{
    delete &proprefsel_;
}


bool uiWellElasticPropSel::setDenLog( const char* nm, const UnitOfMeasure* uom )
{
    const PropertyRef::StdType tp = 
	ElasticPropertyRef::elasticToStdType(ElasticFormula::Den);

    return setLog( tp, nm, false, uom );
}


bool uiWellElasticPropSel::setVelLog( const char* nm, const UnitOfMeasure* uom,
				    bool rev )
{
    const PropertyRef::StdType tp = 
	ElasticPropertyRef::elasticToStdType(ElasticFormula::PVel);

    return setLog( tp, nm, rev, uom );
}


bool uiWellElasticPropSel::getDenLog( BufferString& nm, BufferString& uom) const
{
    const PropertyRef::StdType tp =
	        ElasticPropertyRef::elasticToStdType(ElasticFormula::Den);
    bool dummy;
    return getLog( tp, nm, dummy, uom );
}


bool uiWellElasticPropSel::getVelLog( BufferString& nm, BufferString& um,
					bool& isrev ) const
{
    const PropertyRef::StdType tp =
		ElasticPropertyRef::elasticToStdType(ElasticFormula::PVel);
    return getLog( tp, nm, isrev, um );
}

