/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityfunctionvolume.cc,v 1.4 2009/07/22 16:01:43 cvsbert Exp $";

#include "uivelocityfunctionvolume.h"


#include "uigeninput.h"
#include "uiveldesc.h"
#include "velocityfunctionvolume.h"
#include "seistrctr.h"
#include "seisselection.h"


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
	return 0;

    return new uiVolumeFunction( p, source );
}


uiVolumeFunction::uiVolumeFunction( uiParent* p, VolumeFunctionSource* s )
    : uiFunctionSettings( p, "Volume" )
    , source_( s )
{
    if ( source_ ) source_->ref();

    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    volumesel_ = new uiVelSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    if ( source_ ) volumesel_->setInput( source_->multiID() );

    setHAlignObj( volumesel_ );
}


uiVolumeFunction::~uiVolumeFunction()
{
    if ( source_ ) source_->unRef();
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


}; //namespace
