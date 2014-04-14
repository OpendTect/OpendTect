/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uizaxistransform.h"

#include "datainpspec.h"
#include "refcount.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "zaxistransform.h"
#include "uimsg.h"

mImplFactory3Param( uiZAxisTransform, uiParent*, const char*,
		    const char*, uiZAxisTransform::factory );

bool uiZAxisTransform::isField( const uiParent* p )
{
    mDynamicCastGet( const uiZAxisTransformSel*, sel, p );
    return sel && sel->isField();
}


uiZAxisTransform::uiZAxisTransform( uiParent* p ) 
    : uiDlgGroup( p, 0 )
{}


void uiZAxisTransform::enableTargetSampling()
{}


bool uiZAxisTransform::getTargetSampling( StepInterval<float>& ) const
{ return false; }


uiZAxisTransformSel::uiZAxisTransformSel( uiParent* p, bool withnone,
	const char* fromdomain, const char* todomain, bool withsampling,
	bool isfield )
    : uiDlgGroup( p, 0 )
    , selfld_( 0 )
    , isfield_( isfield )
{
    if ( isfield_ && withsampling )
    {
	pErrMsg( "Field style cannot be used with sampling" );
	return;
    }

    transflds_.allowNull( true );
    TypeSet<uiString> names;

    const BufferStringSet& factorynames =
	uiZAxisTransform::factory().getNames();

    const TypeSet<uiString>& usernames =
	uiZAxisTransform::factory().getUserNames();

    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	uiZAxisTransform* uizat = uiZAxisTransform::factory().create(
		factorynames[idx]->buf(), this, fromdomain, todomain );
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

    const uiString nonestr = tr("None");

    if ( hastransforms && withnone )
    {
	transflds_.insertAt( 0, 0 );
	names.insert( 0, nonestr );
    }

    if ( names.size()>1 )
    {
	selfld_ = new uiGenInput( this, "Z transform",
		StringListInpSpec(names) );
	selfld_->valuechanged.notify( mCB(this, uiZAxisTransformSel,selCB) );

	setHAlignObj( selfld_ );
	
    
	for ( int idx=0; idx<transflds_.size(); idx++ )
	{
	    if ( !transflds_[idx] )
		continue;

	    transflds_[idx]->attach( isfield_ ? rightOf : alignedBelow,
				     selfld_ );
	}
    }
    else if ( hastransforms )
    {
	setHAlignObj( transflds_[0] );
    }

    selCB( 0 );
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
    return &selfld_->valuechanged;
}

#define mGetSel		( selfld_ ? selfld_->getIntValue() : 0 )

bool uiZAxisTransformSel::getTargetSampling( StepInterval<float>& zrg ) const
{
    const int idx = mGetSel;
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
    const int idx = mGetSel;
    return transflds_[idx] ? transflds_[idx]->getSelection() : 0;
}


FixedString uiZAxisTransformSel::selectedToDomain() const
{
    const int idx = mGetSel;
    if ( transflds_.validIdx(idx) )
	return transflds_[idx]->toDomain();

    return sKey::EmptyString();
}


bool uiZAxisTransformSel::fillPar( IOPar& par )
{
    RefMan<ZAxisTransform> sel = getSelection();
    if ( !sel )
	return false;

    sel->fillPar( par );
    return true;
}


bool uiZAxisTransformSel::acceptOK()
{
    const int idx = mGetSel;
    if ( !transflds_[idx] )
	return true;    

    if ( !transflds_[idx]->acceptOK() )
	return false;
    
    StepInterval<float> zrg;
    if ( !getTargetSampling( zrg ) )
	return true;
    
    if ( zrg.isUdf() )
    {
	uiMSG().error("Sampling is not set");
	return false;
    }
    
    if ( zrg.isRev() )
    {
	uiMSG().error("Sampling is reversed.");
	return false;
    }
    
    if ( zrg.step<=0 )
    {
	uiMSG().error("Sampling step is zero or negative");
	return false;
    }
    
    return true;
}


void uiZAxisTransformSel::selCB( CallBacker* )
{
    const int selidx = mGetSel;
    for ( int idx=0; idx<transflds_.size(); idx++ )
    {
	if ( !transflds_[idx] )
	    continue;
	
	transflds_[idx]->display( idx==selidx );
    }
}
