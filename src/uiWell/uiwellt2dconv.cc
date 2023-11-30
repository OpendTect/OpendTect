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
#include "welltransl.h"
#include "zdomain.h"


uiWellT2DTransform::uiWellT2DTransform( uiParent* p )
    : uiTime2DepthZTransformBase(p,true)
{
    selfld_ = new uiIOObjSel( this, mIOObjContext(Well),
			   uiString::emptyString() );
    mAttachCB( selfld_->selectionDone, uiWellT2DTransform::setZRangeCB );
    setHAlignObj( selfld_ );
}


uiWellT2DTransform::~uiWellT2DTransform()
{
    detachAllNotifiers();
}


void uiWellT2DTransform::doInitGrp()
{
    setZRangeCB( nullptr );
}


ZAxisTransform* uiWellT2DTransform::getSelection()
{
    transform_ = nullptr;
    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj )
	return nullptr;

    transform_ = new WellT2DTransform( ioobj->key() );
    if ( !transform_ || !transform_->isOK() )
	transform_ = nullptr;

    return transform_;
}


void uiWellT2DTransform::setZRangeCB( CallBacker* )
{
    ConstRefMan<ZAxisTransform> trans = getSelection();
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	ZSampling range = ZSampling::udf();
	if ( trans )
	    range = trans->getZInterval( false );

	rangefld_->setZRange( range );
    }
}


bool uiWellT2DTransform::acceptOK()
{
    if ( !selfld_->ioobj(false) )
	return false;

    if ( !transform_ )
    {
	uiMSG().error( uiStrings::phrCannotCreate(tr("well-transform")) );
	return false;
    }

    if ( rangefld_ )
    {
	const ZSampling range = rangefld_->getFZRange();
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
				    const uiZAxisTranformSetup& setup )
{
    if ( setup.fromdomain_ != ZDomain::sKeyTime() ||
				setup.todomain_ != ZDomain::sKeyDepth() )
	return nullptr;

    return new uiWellT2DTransform( p );
}
