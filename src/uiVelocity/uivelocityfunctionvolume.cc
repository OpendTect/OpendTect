/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityfunctionvolume.cc,v 1.1 2008-07-22 17:39:21 cvskris Exp $";

#include "uivelocityfunctionvolume.h"


#include "uigeninput.h"
#include "uiseissel.h"
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
    , ctxtioobj_( new CtxtIOObj( SeisTrcTranslatorGroup::ioContext() ) )
{
    if ( source_ ) source_->ref();

    if ( source_ ) ctxtioobj_->setObj( source_->multiID() );
    ctxtioobj_->ctxt.forread = true;

    volumesel_ = new uiSeisSel( this, *ctxtioobj_,
	    			uiSeisSel::Setup(Seis::Line) );
    setHAlignObj( volumesel_ );
}


uiVolumeFunction::~uiVolumeFunction()
{
    if ( source_ ) source_->unRef();
    delete ctxtioobj_->ioobj;
    delete ctxtioobj_;
}


bool uiVolumeFunction::acceptOK()
{
    if ( !ctxtioobj_->ioobj )
	return false;

    if ( !source_ )
    {
	source_ = new VolumeFunctionSource();
	source_->ref();
    }

    if ( !source_->setFrom( ctxtioobj_->ioobj->key() ) )
	return false;

    return true;
}


FunctionSource* uiVolumeFunction::getSource()
{ return source_; }


}; //namespace
