/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
					      tr("Stored Functions") );
}


uiFunctionSettings* uiStoredFunction::create( uiParent* p, FunctionSource* vs )
{
    mDynamicCastGet( StoredFunctionSource*, source, vs );
    if ( vs && !source )
	return nullptr;

    return new uiStoredFunction( p, source );
}


uiStoredFunction::uiStoredFunction( uiParent* p, StoredFunctionSource* s )
    : uiFunctionSettings( p, "Stored" )
    , source_( s )
{
    IOObjContext context = StoredFunctionSource::ioContext();
    BufferStringSet typnms = OD::VelocityTypeDef().keys();
    typnms.remove( toString(OD::VelocityType::Unknown) );
    if ( !SI().zIsTime() )
	typnms.remove( OD::toString(OD::VelocityType::RMS) );

    context.forread_ = true;
    context.require( StoredFunctionSource::sKeyVelocityType(), typnms.cat("`"));
    funcsel_ = new uiIOObjSel( this, context, uiStrings::sInput() );

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

    if ( !source_->setFrom(ioobj->key()) )
    {
	uiString errmsg = tr("Cannot read %1").arg(ioobj->name());
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}


FunctionSource* uiStoredFunction::getSource()
{ return source_; }


} // namespace Vel
