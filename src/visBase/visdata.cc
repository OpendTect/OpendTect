/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdata.cc,v 1.21 2005-02-04 14:31:34 kristofer Exp $";

#include "visdata.h"

#include "errh.h"
#include "visdataman.h"
#include "visselman.h"
#include "iopar.h"

#include "Inventor/nodes/SoNode.h"
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>

namespace visBase
{


const char* DataObject::typestr = "Type";
const char* DataObject::namestr = "Name";


const SoNode* DataObject::getInventorNode() const
{ return const_cast<const SoNode*>(((DataObject*)this)->getInventorNode() ); }


const char* DataObject::name() const
{
    if ( !name_ || !(*name_)[0]) return 0;
    return (const char*)*name_;
}


void DataObject::setName( const char* nn )
{
    SoNode* node = getInventorNode();
    if ( node )
	node->setName( nn );

    if ( !name_ ) name_ = new BufferString;
    
    (*name_) = nn;
}


DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{ mRefCountConstructor; }


DataObject::~DataObject()
{
    DM().removeObject( this );
    delete name_;
}


void DataObject::select() const
{
    DM().selMan().select( id() );
}


void DataObject::deSelect() const
{
    DM().selMan().deSelect( id() );
}


bool DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void DataObject::setDisplayTransformation( Transformation* )
{   
    pErrMsg("Not implemented");
}   
    

void DataObject::fillPar( IOPar& par, TypeSet<int>& ) const
{
    par.set( typestr, getClassName() );

    const char* nm = name();

    if ( nm )
	par.set( namestr, nm );
}


bool DataObject::dumpOIgraph( const char* filename, bool binary )
{
    SoNode* node = getInventorNode();
    if ( !node ) return false;

    SoWriteAction writeaction;
    if ( !writeaction.getOutput()->openFile(filename) ) return false;
    writeaction.getOutput()->setBinary(binary);
    writeaction.apply( node );
    writeaction.getOutput()->closeFile();
    return true;
}


int DataObject::usePar( const IOPar& par )
{
    const char* nm = par.find( namestr );

    if ( nm )
	setName( nm );

    return 1;
}


void DataObject::_init()
{ DM().addObject( this ); }


FactoryEntry::FactoryEntry( FactPtr funcptr_,
				     const char* name_ ) 
    : funcptr( funcptr_ )
    , name( name_ )
    , factory( &DM().factory() )
{
    factory->entries += this;
    factory->closing.notify( mCB(this, FactoryEntry, visIsClosingCB ));
}


FactoryEntry::~FactoryEntry()
{
    if ( !factory )
	return;

    factory->entries -= this;
    factory->closing.remove( mCB(this, FactoryEntry, visIsClosingCB ));
}


void FactoryEntry::visIsClosingCB(CallBacker*)
{
    factory = 0;
}


Factory::Factory()
     : closing( this )
{}


Factory::~Factory()
{ closing.trigger(); }


DataObject* Factory::create( const char* nm )
{
    if ( !nm ) return 0;

    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( !strcmp( nm, entries[idx]->name )) return entries[idx]->create();
    }

    return 0;
}

}; // namespace visBase
