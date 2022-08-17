/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
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
	BufferStringSet typnms;
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Interval]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::RMS]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Avg]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Delta]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Epsilon]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Eta]);
	context.toselect_.require_.set(StoredFunctionSource::sKeyVelocityType(),
				       typnms );
    }
    else
    {
	BufferStringSet typnms;
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Interval]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Avg]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Delta]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Epsilon]);
	typnms.add(VelocityDesc::TypeNames()[(int)VelocityDesc::Eta]);
	context.toselect_.require_.set(StoredFunctionSource::sKeyVelocityType(),
				       typnms );
    }

    context.forread_ = true;

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

    if ( !source_->load( ioobj->key() ) )
    {
	uiString errmsg = tr("Cannot load %1")
			.arg(ioobj->name());
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}


FunctionSource* uiStoredFunction::getSource()
{ return source_; }


} // namespace Vel
