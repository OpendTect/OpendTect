/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdata.cc,v 1.2 2002-03-11 13:38:52 kristofer Exp $";

#include "visdata.h"
#include "visdataman.h"
#include "visselman.h"

#include "Inventor/nodes/SoNode.h"


const char* visBase::DataObject::name() const
{
    if ( !name_ || !(*name_)[0]) return 0;
    return (const char*)*name_;
}


void visBase::DataObject::setName( const char* nn )
{
    if ( !name_ ) name_ = new BufferString;
    
    (*name_) = nn;
}


visBase::DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{ }


visBase::DataObject::~DataObject()
{
    SoNode* data = getData();
    if ( data ) data->unref();

    const SoNode* selobj = getSelObj();
    if ( selobj )
	visBase::DataManager::manager.selMan().unRegSelObject( *this );

    delete name_;
}


void visBase::DataObject::ref() const
{
    DataManager::manager.ref( this );
}


void visBase::DataObject::unRef() const
{
    DataManager::manager.unRef( this );
}


void visBase::DataObject::init()
{
    id_ = DataManager::manager.addObj( this );

    SoNode* data = getData();
    if ( data ) data->ref();

    const SoNode* selobj = getSelObj();
    if ( selobj )
	visBase::DataManager::manager.selMan().regSelObject( *this );
}

