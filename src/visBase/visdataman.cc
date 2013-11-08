/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "separstr.h"
#include "envvars.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include <iostream>

namespace visBase
{

mImplFactory( DataObject, DataManager::factory );

const char* DataManager::sKeyFreeID()		{ return "Free ID"; }
const char* DataManager::sKeySelManPrefix()	{ return "SelMan"; }

DataManager& DM()
{
    mDefineStaticLocalObject( DataManager, manager, );
    return manager;
}


DataManager::DataManager()
    : freeid_( 0 )
    , selman_( *new SelectionManager )
    , removeallnotify( this )
{ }


DataManager::~DataManager()
{
    delete &selman_;
}


const char* DataManager::errMsg() const
{ return errmsg_.str(); }


int DataManager::highestID() const
{
    int max = 0;

    const int nrobjects = objects_.size();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	if ( objects_[idx]->id()>max )
	    max = objects_[idx]->id();
    }

    return max;
}


DataObject* DataManager::getObject( int id ) 
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id ) return objects_[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( int id ) const
{ return const_cast<DataManager*>(this)->getObject(id); }


int DataManager::getID( const osg::Node* node ) const
{
    if ( node )
    {
	for ( int idx=0; idx<objects_.size(); idx++ )
	{
	    if ( objects_[idx]->osgNode()==node ) return objects_[idx]->id();
	}
    }

    return -1;
}


void DataManager::addObject( DataObject* obj )
{
    if ( objects_.indexOf(obj)==-1 )
    {
	objects_ += obj;
	obj->setID(freeid_++);
    }
}


void DataManager::getIDs( const std::type_info& ti,
			  TypeSet<int>& res) const
{
    res.erase();

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( typeid(*objects_[idx]) == ti )
	    res += objects_[idx]->id();
    }
}


void DataManager::removeObject( DataObject* dobj )
{ objects_ -= dobj; }


int DataManager::nrObjects() const
{ return objects_.size(); }


DataObject* DataManager::getIndexedObject( int idx )
{ return idx>=0 && idx<nrObjects() ? objects_[idx] : 0; }


const DataObject* DataManager::getIndexedObject( int idx ) const
{ return const_cast<DataManager*>(this)->getIndexedObject(idx); }


}; //namespace
