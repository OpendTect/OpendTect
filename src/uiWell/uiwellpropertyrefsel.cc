/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwellpropertyrefsel.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uiunitsel.h"
#include "uimsg.h"
#include "uiwelllogcalc.h"
#include "uiwelllogdisplay.h"

#include "elasticprop.h"
#include "elasticpropsel.h"
#include "unitofmeasure.h"
#include "property.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellman.h"


uiPropSelFromList::uiPropSelFromList( uiParent* p, const PropertyRef& pr,
	                                const PropertyRef* alternatepr )
    : uiGroup( p, pr.name() )
    , propref_(pr)
    , altpropref_(0)
    , checkboxfld_(0)
    , comboChg_(this)
{
    typefld_ = new uiComboBox( this, BufferString(pr.name()," type") );
    typefld_->selectionChanged.notify(
	   		       mCB( this, uiPropSelFromList, updateSelCB ) );
    typelbl_ = new uiLabel( this, pr.name(), typefld_ );

    unfld_ = new uiUnitSel( this, propref_.stdType(), 0, false, true );
    unfld_->setUnit( propref_.disp_.unit_ );
    unfld_->attach( rightOf, typefld_ );

    if ( alternatepr )
    {
	altpropref_ = new PropertyRef(*alternatepr);
	checkboxfld_ = new uiCheckBox( this, alternatepr->name() );
	checkboxfld_->attach( rightOf, unfld_ );
	checkboxfld_->activated.notify(
				mCB( this, uiPropSelFromList,switchPropCB ) );
    }
    setHAlignObj( typefld_ );
}


uiPropSelFromList::~uiPropSelFromList()
{
    delete altpropref_;
}


void uiPropSelFromList::updateSelCB( CallBacker* )
{
    comboChg_.trigger();
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
    unfld_->setUnit( &um );
}


void uiPropSelFromList::set( const char* txt, bool alt, const UnitOfMeasure* um)
{
    setCurrent( txt ); setUseAlternate( alt ); unfld_->setUnit( um );
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



uiWellPropSel::uiWellPropSel( uiParent* p, const PropertyRefSelection& prs )
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
	fld->comboChg_.notify( mCB( this, uiWellPropSel, updateSelCB) );
	if ( propflds_.size() > 0 )
	    fld->attach( alignedBelow, propflds_[propflds_.size()-1] );
	else
	    setHAlignObj( fld->typeFld() );

	delete altpr;
	propflds_ += fld;
    }
}


void uiWellPropSel::updateSelCB( CallBacker* c )
{
    if ( !c ) return;

    mDynamicCastGet(uiPropSelFromList*, fld, c);
    if ( !fld ) return;
    const Well::Data* wd = Well::MGR().get( wellid_, false );
    if  ( !wd ) return;

    const Well::Log* log = wd->logs().getLog( fld->text() );
    const char* logunitnm = log ? log->unitMeasLabel() : 0;
    const UnitOfMeasure* logun = UnitOfMeasure::getGuessed( logunitnm );
    if ( !logun )
    {
	const UnitOfMeasure* emptyuom = 0;
	fld->setUOM( *emptyuom );
	return;
    }
    const PropertyRef& propref = fld->propRef();
    const PropertyRef* altpropref = fld->altPropRef();
    bool isreverted;
    if ( propref.stdType() == logun->propType() )
	isreverted = false;
    else if ( altpropref->stdType() == logun->propType() )
	isreverted = true;
    else return;
    fld->setUseAlternate( isreverted );
    fld->setUOM ( *logun );
}


bool uiWellPropSel::setLogs( const Well::LogSet& logs  )
{
    bool propertyhasnoinput = false;
    for ( int iprop=0; iprop<propflds_.size(); iprop++ )
    {
	BufferStringSet lognms;
	lognms.add( sKeyPlsSel() );

	const PropertyRef& propref = propflds_[iprop]->propRef();
	const PropertyRef* altpropref = propflds_[iprop]->altPropRef();

	TypeSet<int> propidx;
	TypeSet<int> isaltpropref;
	uiWellLogCalc::getSuitableLogs( logs, lognms, propidx, isaltpropref,
					propref, altpropref );

	propflds_[iprop]->setNames( lognms );

	if ( lognms.size() < 2 || !propidx.size() )
	{
	    propertyhasnoinput = true;
	    const UnitOfMeasure* nouom = 0;
	    propflds_[iprop]->setUseAlternate( false );
	    propflds_[iprop]->setUOM ( *nouom );
	    continue;
	}

	int logidx = -1;
	int logidxalt = -1;
	for ( int ipropidx=0; ipropidx<propidx.size(); ipropidx++)
	{
	    BufferString lognm = logs.getLog(propidx[ipropidx]).name();
	    const char* uomlbl = logs.getLog(propidx[ipropidx]).unitMeasLabel();
	    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( uomlbl );
	    if ( uom && uom->propType() == propref.stdType() )
	    {
		if ( logidx == -1 )
		    logidx = ipropidx;
		if ( lognm.isStartOf(propref.name(),true) )
		{
		    logidx = ipropidx;
		    break;
		}
	    }
	    else if ( !uom && lognm.isStartOf(propref.name(),true) )
	    {
		logidx = ipropidx;
		break;
	    }
	    else if ( altpropref )
	    {
		if ( uom && uom->propType() == altpropref->stdType() )
		{
		    if ( logidxalt == -1 )
			logidxalt = ipropidx;
		}
		else if ( !uom && lognm.isStartOf(altpropref->name(),true) )
		{
		    logidxalt = ipropidx;
		}
	    }
	}
	if ( logidx == -1 )
	{
	    if ( logidxalt == -1 )
		logidx = 0;
	    else
		logidx = logidxalt;
	}
	const char* uomlbl = logs.getLog(propidx[logidx]).unitMeasLabel();
	const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( uomlbl );
	propflds_[iprop]->set( logs.getLog(propidx[logidx]).name(),
			       isaltpropref[logidx], uom );
    }

    if ( propertyhasnoinput )
	return false;

    return true;
}


bool uiWellPropSel::isOK() const
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( !strcmp ( propflds_[idx]->text(), sKeyPlsSel() ) )
	{
	    uiMSG().error( BufferString("Please create/select a log for ",
					propflds_[idx]->propRef().name()) );
	    return false;
	}
    }
    return true;
}


bool uiWellPropSel::setLog( const PropertyRef::StdType tp, 
				const char* nm, bool usealt,
				const UnitOfMeasure* uom, int idx )
{
    if ( propflds_[idx]->propRef().hasType( tp ) )
	propflds_[idx]->set( nm, usealt, uom );
    return false;
}


bool uiWellPropSel::getLog( const PropertyRef::StdType tp, BufferString& bs, 
			bool& check, BufferString& uom, int idx ) const
{
    check = propflds_[idx]->isUseAlternate();
    const PropertyRef* alternatepr = check ? propflds_[idx]->altPropRef() : 0;
    if ( propflds_[idx]->propRef().hasType(tp)
	    || (alternatepr && alternatepr->hasType(tp)) ) 
    {
	bs = propflds_[idx]->text();
	if ( propflds_[idx]->uom() )
	{
	    if ( *propflds_[idx]->uom()->symbol() != '\0' )
		uom = propflds_[idx]->uom()->symbol();
	    else
		uom = propflds_[idx]->uom()->name();
	}
	else
	    uom.setEmpty();
	return true;
    }

    return false;
}


uiPropSelFromList*  uiWellPropSel::getPropSelFromListByName(
						const BufferString& bfs )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( propflds_[idx] && propflds_[idx]->getLabel() )
	{
	    BufferString lblnm = BufferString(
		    			propflds_[idx]->getLabel()->text() );
	    if ( lblnm == bfs )
		return propflds_[idx];
	}
    }

    return 0;
}


uiPropSelFromList* uiWellPropSel::getPropSelFromListByIndex( int idx )
{
    if ( propflds_.validIdx(idx) )
	return propflds_[idx];

    return 0;
}


uiWellPropSelWithCreate::uiWellPropSelWithCreate( uiParent* p,
				const PropertyRefSelection& prs )
    : uiWellPropSel(p,prs)
    , logscreated(this) 
{ 
    for ( int idx=0; idx<propflds_.size(); idx ++ )
    {
	uiToolButton* createbut = new uiToolButton( this, "newlog",
		"Create a log from other logs",
		mCB(this,uiWellPropSelWithCreate,createLogPushed) );
	uiToolButton* viewbut = new uiToolButton( this, "view_log",
		"View log", mCB(this,uiWellPropSelWithCreate,viewLogPushed) );
	if ( idx )
	    createbut->attach( ensureBelow, createbuts_[idx-1] );
	for ( int idotherpr=0; idotherpr<propflds_.size(); idotherpr ++ )
	    createbut->attach( ensureRightOf, propflds_[idotherpr] );
	viewbut->attach( rightOf, createbut );
	createbuts_ += createbut;
	viewbuts_ += viewbut;
    }
}


void uiWellPropSelWithCreate::createLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = createbuts_.indexOf( but );
    if ( !propflds_.validIdx( idxofbut ) )
	return;

    const Well::Data* wd = Well::MGR().get( wellid_, false );
    if  ( !wd ) return;

    const Well::LogSet& logs = wd->logs();
    BufferStringSet lognms;
    for ( int idx=0; idx<logs.size(); idx++ )
	lognms.add( logs.getLog(idx).name() );

    TypeSet<MultiID> idset; idset += wellid_;
    uiWellLogCalc dlg( this, logs, lognms, idset );
    dlg.setOutputLogName( propflds_[idxofbut]->propRef().name() );
    dlg.go();

    if ( dlg.haveNewLogs() )
    {
	logscreated.trigger();
	propflds_[idxofbut]->setCurrent( dlg.getOutputLogName() );
    }
} 


void uiWellPropSelWithCreate::viewLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = viewbuts_.indexOf( but );
    if ( !propflds_.validIdx( idxofbut ) )
	return;

    const BufferString lognm( propflds_[idxofbut]->text() );
    if ( lognm == sKeyPlsSel() )
	return;

    const Well::Data* wd = Well::MGR().get( wellid_, false );
    if  ( !wd ) return;

    const Well::LogSet& logs = wd->logs();
    const Well::Log* wl = logs.getLog( lognm );
    if ( !wl )
	return; // the log was removed since popup of the window ... unlikely

    (void)uiWellLogDispDlg::popupNonModal( this, wl );
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
