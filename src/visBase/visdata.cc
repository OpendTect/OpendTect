/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdata.cc,v 1.19 2004-01-05 09:43:23 kristofer Exp $";

#include "visdata.h"

#include "errh.h"
#include "visdataman.h"
#include "visselman.h"
#include "iopar.h"

#include "Inventor/nodes/SoNode.h"
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>


const char* visBase::DataObject::typestr = "Type";
const char* visBase::DataObject::namestr = "Name";


const SoNode* visBase::DataObject::getInventorNode() const
{ return const_cast<const SoNode*>(((visBase::DataObject*)this)->getInventorNode() ); }


const char* visBase::DataObject::name() const
{
    if ( !name_ || !(*name_)[0]) return 0;
    return (const char*)*name_;
}


void visBase::DataObject::setName( const char* nn )
{
    SoNode* node = getInventorNode();
    if ( node )
	node->setName( nn );

    if ( !name_ ) name_ = new BufferString;
    
    (*name_) = nn;
}


visBase::DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{ }


visBase::DataObject::~DataObject()
{
    delete name_;
}


void visBase::DataObject::ref() const
{
    DM().ref( this );
}


void visBase::DataObject::unRef() const
{
    DM().unRef( this, true );
}


void visBase::DataObject::unRefNoDelete() const
{
    DM().unRef( this, false );
}


void visBase::DataObject::select() const
{
    DM().selMan().select( id() );
}


void visBase::DataObject::deSelect() const
{
    DM().selMan().deSelect( id() );
}


bool visBase::DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void visBase::DataObject::setTransformation( visBase::Transformation* )
{   
    pErrMsg("Not implemented");
}   
    

void visBase::DataObject::fillPar( IOPar& par, TypeSet<int>& ) const
{
    par.set( typestr, getClassName() );

    const char* nm = name();

    if ( nm )
	par.set( namestr, nm );
}


bool visBase::DataObject::dumpOIgraph( const char* filename, bool binary )
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


int visBase::DataObject::usePar( const IOPar& par )
{
    const char* nm = par.find( namestr );

    if ( nm )
	setName( nm );

    return 1;
}


void visBase::DataObject::_init()
{
    id_ = DM().addObj( this );
}


visBase::FactoryEntry::FactoryEntry( FactPtr funcptr_,
				     const char* name_ ) 
    : funcptr( funcptr_ )
    , name( name_ )
    , factory( &DM().factory() )
{
    factory->entries += this;
    factory->closing.notify( mCB(this, visBase::FactoryEntry, visIsClosingCB ));
}


visBase::FactoryEntry::~FactoryEntry()
{
    if ( !factory )
	return;

    factory->entries -= this;
    factory->closing.remove( mCB(this, visBase::FactoryEntry, visIsClosingCB ));
}


void visBase::FactoryEntry::visIsClosingCB(CallBacker*)
{
    factory = 0;
}


visBase::Factory::Factory()
     : closing( this )
{}


visBase::Factory::~Factory()
{ closing.trigger(); }


visBase::DataObject* visBase::Factory::create( const char* nm )
{
    if ( !nm ) return 0;

    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( !strcmp( nm, entries[idx]->name )) return entries[idx]->create();
    }

    return 0;
}
