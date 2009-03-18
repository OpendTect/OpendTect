/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityfunctionstored.cc,v 1.1 2009-03-18 18:45:26 cvskris Exp $";

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
    , ctxtioobj_( new CtxtIOObj( StoredFunctionSource::ioContext() ) )
{
    if ( SI().zIsTime() )
    {
	ctxtioobj_->ctxt.parconstraints.set(
	    StoredFunctionSource::sKeyVelocityType(),
	    VelocityDesc::TypeNames()[(int)VelocityDesc::Interval],
	    VelocityDesc::TypeNames()[(int)VelocityDesc::RMS] );
    }
    else
    {
	ctxtioobj_->ctxt.parconstraints.set(
	    StoredFunctionSource::sKeyVelocityType(),
	    VelocityDesc::TypeNames()[(int)VelocityDesc::Interval] );
    }

    if ( source_ ) source_->ref();

    if ( source_ ) ctxtioobj_->setObj( source_->multiID() );
    ctxtioobj_->ctxt.forread = true;

    funcsel_ = new uiIOObjSel( this, *ctxtioobj_, "Input" );
    setHAlignObj( funcsel_ );
}


uiStoredFunction::~uiStoredFunction()
{
    if ( source_ ) source_->unRef();
    delete ctxtioobj_->ioobj;
    delete ctxtioobj_;
}


bool uiStoredFunction::acceptOK()
{
    if ( !ctxtioobj_->ioobj )
	return false;

    if ( !source_ )
    {
	source_ = new StoredFunctionSource();
	source_->ref();
    }

    if ( !source_->load( ctxtioobj_->ioobj->key() ) )
    {
	BufferString errmsg = "Cannot load ";
	errmsg += ctxtioobj_->ioobj->name();
	uiMSG().error( errmsg.buf() );
	return false;
    }

    return true;
}


FunctionSource* uiStoredFunction::getSource()
{ return source_; }


}; //namespace
