/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityfunctionstored.cc,v 1.3 2009-04-30 14:54:41 cvskris Exp $";

#include "uivelocityfunctionstored.h"


#include "uigeninput.h"
#include "uiioobjsel.h"
#include "velocityfunctionstored.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "uimsg.h"
#include "survinfo.h"


namespace Vel
{


void uiStoredFunction::initClass()
{
    uiFunctionSettings::factory().addCreator( create, "Stored",
	    				      "Stored Functions" );
}


uiFunctionSettings* uiStoredFunction::create( uiParent* p, FunctionSource* vs )
{
    mDynamicCastGet( StoredFunctionSource*, source, vs );
    if ( vs && !source )
	return 0;

    return new uiStoredFunction( p, source );
}


uiStoredFunction::uiStoredFunction( uiParent* p, StoredFunctionSource* s )
    : uiFunctionSettings( p, "Stored" )
    , source_( s )
{
    IOObjContext context = StoredFunctionSource::ioContext();
    if ( SI().zIsTime() )
    {
	context.parconstraints.set(
	    StoredFunctionSource::sKeyVelocityType(),
	    VelocityDesc::TypeNames()[(int)VelocityDesc::Interval],
	    VelocityDesc::TypeNames()[(int)VelocityDesc::RMS] );
    }
    else
    {
	context.parconstraints.set(
	    StoredFunctionSource::sKeyVelocityType(),
	    VelocityDesc::TypeNames()[(int)VelocityDesc::Interval] );
    }

    context.forread = true;

    funcsel_ = new uiIOObjSel( this, context, "Input" );

    if ( source_ )
    {
	source_->ref();
	funcsel_->setInput( source_->multiID() );
    }

    setHAlignObj( funcsel_ );
}


uiStoredFunction::~uiStoredFunction()
{
    if ( source_ ) source_->unRef();
}


bool uiStoredFunction::acceptOK()
{
    const IOObj* ioobj = funcsel_->ioobj( false );
    if ( !ioobj )
	return false;

    if ( !source_ )
    {
	source_ = new StoredFunctionSource();
	source_->ref();
    }

    if ( !source_->load( ioobj->key() ) )
    {
	BufferString errmsg = "Cannot load ";
	errmsg += ioobj->name();
	uiMSG().error( errmsg.buf() );
	return false;
    }

    return true;
}


FunctionSource* uiStoredFunction::getSource()
{ return source_; }


}; //namespace
