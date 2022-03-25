/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiwellt2dconv.h"

#include "uiioobjsel.h"
#include "survinfo.h"
#include "binidvalue.h"
#include "welltransl.h"
#include "wellt2dtransform.h"
#include "uimsg.h"
#include "uizrangeinput.h"
#include "zdomain.h"


uiWellT2DTransform::uiWellT2DTransform( uiParent* p )
    : uiTime2DepthZTransformBase(p, true )
    , transform_( 0 )
{
    fld_ = new uiIOObjSel( this, mIOObjContext(Well), 
			   uiString::emptyString() );
    fld_->selectionDone.notify( mCB(this,uiWellT2DTransform,setZRangeCB) );

    setHAlignObj( fld_ );
    postFinalize().notify( mCB(this,uiWellT2DTransform,setZRangeCB) );
}


uiWellT2DTransform::~uiWellT2DTransform()
{
    unRefAndZeroPtr( transform_ );
}


ZAxisTransform* uiWellT2DTransform::getSelection()
{
    unRefAndZeroPtr( transform_ );

    const IOObj* ioobj = fld_->ioobj( true );
    if ( !ioobj ) return 0;

    transform_ = new WellT2DTransform( ioobj->key() );
    refPtr( transform_ );
    if ( !transform_ || !transform_->isOK() )
    {
	unRefAndZeroPtr( transform_ );
	return 0;
    }

    return transform_;
}


void uiWellT2DTransform::setZRangeCB( CallBacker* )
{
    RefMan<ZAxisTransform> trans = getSelection();
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	StepInterval<float> range( StepInterval<float>::udf() );
	if ( trans )
	{
	    range = trans->getZInterval( false );
	    range.step = trans->getGoodZStep();
	    if ( range.isUdf() ) range.setUdf();
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
    if ( fromdomain!=ZDomain::sKeyTime() || todomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiWellT2DTransform( p );
}
