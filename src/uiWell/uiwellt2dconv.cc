/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiwellt2dconv.cc 32104 2013-10-23 20:11:53Z kristofer.tingdahl@dgbes.com $";

#include "uiwellt2dconv.h"

#include "uiioobjsel.h"
#include "welltransl.h"
#include "wellt2dtransform.h"
#include "uimsg.h"
#include "zdomain.h"


uiWellT2DTransform::uiWellT2DTransform( uiParent* p )
    : uiTime2DepthZTransformBase(p, true )
    , transform_( 0 )
{
    fld_ = new uiIOObjSel( this, mIOObjContext(Well), "" );
    setHAlignObj( fld_ );
}


uiWellT2DTransform::~uiWellT2DTransform()
{
    unRefAndZeroPtr( transform_ );
}


ZAxisTransform* uiWellT2DTransform::getSelection()
{
    return transform_;
}


bool uiWellT2DTransform::acceptOK()
{
    unRefAndZeroPtr( transform_ );

    const IOObj* ioobj = fld_->ioobj( false );
    if ( !ioobj )
	return false;

    transform_ = new WellT2DTransform( ioobj->key() );
    refPtr( transform_ );
    if ( !transform_ || !transform_->isOK() )
    {
	uiMSG().error( tr("Could not create well-transform") );
	unRefAndZeroPtr( transform_ );
	return false;
    }

    return true;
}



void uiWellT2DTransform::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
				    WellT2DTransform::sFactoryKeyword(),
				    "Well's Depth model");
}


uiZAxisTransform* uiWellT2DTransform::createInstance(uiParent* p,
				const char* fromdomain, const char* todomain )
{
    if ( fromdomain!=ZDomain::sKeyTime() || todomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiWellT2DTransform( p );
}


