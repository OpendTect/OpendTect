/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocityfunctionvolume.h"

#include "uigeninput.h"
#include "uiveldesc.h"

#include "seisioobjinfo.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "velocityfunctionvolume.h"


namespace Vel
{


void uiVolumeFunction::initClass()
{
    uiFunctionSettings::factory().addCreator( create, "Volume" );
}


uiFunctionSettings* uiVolumeFunction::create( uiParent* p, FunctionSource* vs )
{
    mDynamicCastGet( VolumeFunctionSource*, source, vs );
    if ( vs && !source )
	return nullptr;

    return new uiVolumeFunction( p, source );
}


uiVolumeFunction::uiVolumeFunction( uiParent* p, VolumeFunctionSource* s )
    : uiFunctionSettings( p, "Volume" )
    , source_( s )
{
    if ( source_ )
	source_->ref();

    bool is2d = false;
    MultiID srcid;
    if ( source_ )
    {
	srcid = source_->multiID();
	if ( !srcid.isUdf() )
	{
	    const SeisIOObjInfo info( srcid );
	    if ( info.isOK() )
		is2d = info.is2D();
	}
    }

    auto* volumesel = new uiVelSel( this, VelocityDesc::getVelVolumeLabel(),
				    is2d );
    volumesel->setVelocityOnly( false );
    if ( source_ )
	volumesel->setInput( srcid );

    volumesel_ = volumesel;

    setHAlignObj( volumesel_ );
}


uiVolumeFunction::~uiVolumeFunction()
{
    unRefPtr( source_ );
}


bool uiVolumeFunction::acceptOK()
{
    const IOObj* ioobj = volumesel_->ioobj();
    if ( !ioobj )
	return false;

    if ( !source_ )
    {
	source_ = new VolumeFunctionSource();
	source_->ref();
    }

    return source_->setFrom( ioobj->key() );
}


FunctionSource* uiVolumeFunction::getSource()
{ return source_; }


} // namespace Vel
