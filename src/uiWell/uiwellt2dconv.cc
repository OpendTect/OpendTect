/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellt2dconv.h"

#include "binidvalue.h"
#include "survinfo.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uizrangeinput.h"

#include "wellt2dtransform.h"
#include "welltransl.h"
#include "zdomain.h"


uiWellT2DTransform::uiWellT2DTransform( uiParent* p )
    : uiTime2DepthZTransformBase(p,true)
    , transform_(nullptr)
{
    fld_ = new uiIOObjSel( this, mIOObjContext(Well),
			   uiString::emptyString() );
    mAttachCB( fld_->selectionDone, uiWellT2DTransform::setZRangeCB );
    setHAlignObj( fld_ );
    mAttachCB( postFinalize(), uiWellT2DTransform::initGrpCB );
}


uiWellT2DTransform::~uiWellT2DTransform()
{
    detachAllNotifiers();
    unRefPtr( transform_ );
}


void uiWellT2DTransform::initGrpCB( CallBacker* )
{
    setZRangeCB( nullptr );
    uiTime2DepthZTransformBase::finalizeDoneCB( nullptr );
}


ZAxisTransform* uiWellT2DTransform::getSelection()
{
    unRefAndZeroPtr( transform_ );
    const IOObj* ioobj = fld_->ioobj( true );
    if ( !ioobj )
	return nullptr;

    transform_ = new WellT2DTransform( ioobj->key() );
    refPtr( transform_ );
    if ( !transform_ || !transform_->isOK() )
	unRefAndZeroPtr( transform_ );

    return transform_;
}


void uiWellT2DTransform::setZRangeCB( CallBacker* )
{
    ConstRefMan<ZAxisTransform> trans = getSelection();
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	StepInterval<float> range = StepInterval<float>::udf();
	if ( trans )
	{
	    range.setFrom( trans->getZInterval( false ) );
	    range.step = trans->getGoodZStep();
	}

	rangefld_->setZRange( range );
    }
}


bool uiWellT2DTransform::acceptOK()
{
    if ( !fld_->ioobj(false) )
	return false;

    if ( !transform_ )
    {
	uiMSG().error( uiStrings::phrCannotCreate(tr("well-transform")) );
	return false;
    }

    if ( rangefld_ )
    {
	const StepInterval<float> range = rangefld_->getFZRange();
	if ( range.isUdf() )
	{
	    uiMSG().error( tr("Z-Range is not set") );
	    return false;
	}
    }

    return true;
}


void uiWellT2DTransform::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
				    WellT2DTransform::sFactoryKeyword(),
				    tr("Well's Depth model"));
}


uiZAxisTransform* uiWellT2DTransform::createInstance(uiParent* p,
				const char* fromdomain, const char* todomain )
{
    if ( StringView(fromdomain) != ZDomain::sKeyTime() ||
	 StringView(todomain) != ZDomain::sKeyDepth() )
	return nullptr;

    return new uiWellT2DTransform( p );
}
