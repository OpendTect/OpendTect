/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uizaxistransform.h"

#include "datainpspec.h"
#include "refcount.h"
#include "zaxistransform.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"

// uiZAxisTransform

mImplFactory2Param( uiZAxisTransform, uiParent*, const uiZAxisTranformSetup&,
						uiZAxisTransform::factory );

uiZAxisTransform::uiZAxisTransform( uiParent* p )
    : uiDlgGroup( p, uiString::empty() )
{
}


uiZAxisTransform::~uiZAxisTransform()
{
}


void uiZAxisTransform::enableTargetSampling()
{
}


bool uiZAxisTransform::getTargetSampling( ZSampling& ) const
{
    return false;
}


bool uiZAxisTransform::isField( const uiParent* p )
{
    mDynamicCastGet(const uiZAxisTransformSel*,sel,p);
    return sel && sel->isField();
}


// uiZAxisTransformSel

uiZAxisTransformSel::uiZAxisTransformSel( uiParent* p, bool withnone,
	const char* fromdomain, const char* todomain, bool withsampling,
						    bool isfield, bool is2d )
    : uiDlgGroup( p, uiString::empty() )
    , isfield_(isfield)
{
    if ( isfield_ && withsampling )
    {
	pErrMsg( "Field style cannot be used with sampling" );
	return;
    }

    transflds_.setNullAllowed();
    uiStringSet names;

    const BufferStringSet& factorynames =
	uiZAxisTransform::factory().getNames();

    const uiStringSet& usernames =
	uiZAxisTransform::factory().getUserNames();

    uiZAxisTranformSetup setup;
    setup.fromdomain_ = fromdomain;
    setup.todomain_ = todomain;
    const OD::Pol2D3D poltype = is2d ? OD::Only2D : OD::Only3D;
    setup.datatype_ = poltype;
    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	auto* uizat = uiZAxisTransform::factory().create(
				    factorynames[idx]->buf(), this, setup );
	if ( !uizat )
	    continue;

	if ( isfield_ && !uizat->canBeField() )
	{
	    delete uizat;
	    continue;
	}

	if ( withsampling )
	    uizat->enableTargetSampling();

	transflds_ += uizat;
	names += usernames[idx];
    }

    const bool hastransforms = names.size();

    const uiString nonestr = uiStrings::sNone();

    if ( hastransforms && withnone )
    {
	transflds_.insertAt( nullptr, 0 );
	names.insert( 0, nonestr );
    }

    if ( names.size()>1 )
    {
	selfld_ = new uiGenInput( this, tr("Z transform"),
				  StringListInpSpec(names) );
	mAttachCB( selfld_->valueChanged, uiZAxisTransformSel::selCB );
	for ( int idx=0; idx<transflds_.size(); idx++ )
	{
	    if ( !transflds_[idx] )
		continue;

	    transflds_[idx]->attach( isfield_ ? rightOf : alignedBelow,
				     selfld_ );
	}

	setHAlignObj( selfld_ );
	mAttachCB( postFinalize(), uiZAxisTransformSel::initGrp );
    }
    else if ( hastransforms )
	setHAlignObj( transflds_.first() );
}


uiZAxisTransformSel::~uiZAxisTransformSel()
{
    detachAllNotifiers();
}


void uiZAxisTransformSel::initGrp( CallBacker* )
{
    selCB( nullptr );
}


void uiZAxisTransformSel::selCB( CallBacker* )
{
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    for ( int idx=0; idx<transflds_.size(); idx++ )
    {
	if ( !transflds_[idx] )
	    continue;

	transflds_[idx]->display( idx==selidx );
    }
}


bool uiZAxisTransformSel::isField() const
{
    return isfield_;
}


void uiZAxisTransformSel::setLabel(const uiString& lbl )
{
    selfld_->setTitleText( lbl );
}


NotifierAccess* uiZAxisTransformSel::selectionDone()
{
    return &selfld_->valueChanged;
}


bool uiZAxisTransformSel::getTargetSampling( ZSampling& zrg ) const
{
    const int idx = selfld_ ? selfld_->getIntValue() : 0;
    if ( !transflds_[idx] )
	return false;

    return transflds_[idx]->getTargetSampling( zrg );

}


int uiZAxisTransformSel::nrTransforms() const
{
    int res = transflds_.size();
    if ( res && !transflds_[0] )
	res--;
    return res;
}


ZAxisTransform* uiZAxisTransformSel::getSelection()
{
    const int idx = selfld_ ? selfld_->getIntValue() : 0;
    return transflds_[idx] ? transflds_[idx]->getSelection() : nullptr;
}


StringView uiZAxisTransformSel::selectedToDomain() const
{
    const int idx = selfld_ ? selfld_->getIntValue() : 0;
    if ( transflds_.validIdx(idx) && transflds_[idx] )
	return transflds_[idx]->toDomain();

    return sKey::EmptyString();
}


bool uiZAxisTransformSel::fillPar( IOPar& par )
{
    ConstRefMan<ZAxisTransform> sel = getSelection();
    if ( !sel )
	return false;

    sel->fillPar( par );
    return true;
}


bool uiZAxisTransformSel::acceptOK()
{
    const int idx = selfld_ ? selfld_->getIntValue() : 0;
    if ( !transflds_[idx] )
	return true;

    if ( !transflds_[idx]->acceptOK() )
	return false;

    ZSampling zrg;
    if ( !getTargetSampling(zrg) )
	return true;

    if ( zrg.isUdf() )
    {
	uiMSG().error(tr("Sampling is not set"));
	return false;
    }

    if ( zrg.isRev() )
    {
	uiMSG().error(tr("Sampling is reversed."));
	return false;
    }

    if ( zrg.step<=0 )
    {
	uiMSG().error(tr("Sampling step is zero or negative"));
	return false;
    }

    return true;
}
