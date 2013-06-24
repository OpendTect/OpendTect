/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra & K. Tingdahl
 * DATE     : September 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "displaypropertylinks.h"
#include "ptrman.h"

mImplFactory1Param(DisplayPropertyLink,ObjectSet<DisplayPropertyHolder>&,
		   DisplayPropertyLink::factory);


// ***** DisplayPropertyHolder *****
DisplayPropertyHolder::DisplayPropertyHolder( bool reg )
    : propertyholderid_( -1 )
{ if ( reg ) DisplayLinkManager::getImpl().addHolder(this); }


DisplayPropertyHolder::~DisplayPropertyHolder()
{ DisplayLinkManager::getImpl().removeHolder(this); }


int DisplayPropertyHolder:: propertyHolderID() const
{ return propertyholderid_; }


const char* DisplayPropertyHolder::getDisplayPropertyHolderName() const
{ return 0; }


// ***** DisplayPropertyLink *****
DisplayPropertyLink::DisplayPropertyLink( DisplayPropertyHolder* h0,
					  DisplayPropertyHolder* h1 )
{
    holders_ += h0; holders_ += h1;
}


const char* DisplayPropertyLink::userType() const
{ return type(); }


bool DisplayPropertyLink::init()
{ return syncNow( holders_[0], holders_[1] ); }


bool DisplayPropertyLink::addHolder( DisplayPropertyHolder* dph )
{
    if ( !isHolderOK( dph ) )
	return false;

    holders_ += dph;
    
    return syncNow( holders_[0], dph );
}


void DisplayPropertyLink::removeHolder( DisplayPropertyHolder* dph )
{ holders_ -= dph; }


bool DisplayPropertyLink::isValid() const
{ return holders_.size()>1; }


void DisplayPropertyLink::getDescription( BufferString& desc ) const
{ desc = userType(); }


int DisplayPropertyLink::nrDisplayPropertyHolders() const
{ return holders_.size(); }


const DisplayPropertyHolder*
    DisplayPropertyLink::getDisplayPropertyHolder( int idx ) const
{ return holders_[idx]; }


bool DisplayPropertyLink::syncNow( const DisplayPropertyHolder* origin,
				   DisplayPropertyHolder* target)
{ return true; }


bool DisplayPropertyLink::isHolderOK( const DisplayPropertyHolder* )
{ return true; }


// ***** DisplayLinkManager *****
DisplayLinkManager::DisplayLinkManager()
    : freeholderid_( 0 )
    , freelinkid_( 0 )
{}


DisplayLinkManager::~DisplayLinkManager()
{
    deepErase( propertylinks_ );
}


DisplayLinkManager& DisplayLinkManager::getImpl()
{
    static PtrMan<DisplayLinkManager> mgr = new DisplayLinkManager;
    return *mgr;
}


int DisplayLinkManager::addHolder( DisplayPropertyHolder* hldr )
{
    Threads::Locker lckr( lock_ );
    if ( !holders_.isPresent( hldr ) )
    {
	hldr->propertyholderid_ = freeholderid_++;
	holders_ += hldr;
    }

    return hldr->propertyHolderID();
}


int DisplayLinkManager::nrHolders() const
{ return holders_.size(); }

const DisplayPropertyHolder* DisplayLinkManager::getHolder( int idx ) const
{ return holders_.validIdx(idx) ? holders_[idx] : 0; }


void DisplayLinkManager::createPossibleLinks( DisplayPropertyHolder* hldr,
				    ObjectSet<DisplayPropertyLink>& links )
{
    Threads::Locker lckr( lock_ );
    ObjectSet<DisplayPropertyHolder> tmpholders;
    tmpholders += hldr;
    for ( int idx=holders_.size()-1; idx>=0; idx-- )
    {
	if ( holders_[idx]==hldr )
	    continue;

	tmpholders += holders_[idx];

	const BufferStringSet& names =
	    DisplayPropertyLink::factory().getNames( false );

	for ( int idy=names.size()-1; idy>=0; idy-- )
	{
	    DisplayPropertyLink* link =
		DisplayPropertyLink::factory().create( names[idy]->buf(),
						       tmpholders );
	    if ( link )
		links += link;
	}

	tmpholders -= holders_[idx];
    }
}


int DisplayLinkManager::nrDisplayPropertyLinks() const
{
    Threads::Locker lckr( lock_ );
    return propertylinks_.size();
}


int DisplayLinkManager::getDisplayPropertyLinkID( int idx ) const
{
    Threads::Locker lckr( lock_ );
    if ( !propertylinkids_.validIdx(idx) )
	return -1;

    return propertylinkids_[idx];
}


int DisplayLinkManager::addDisplayPropertyLink( DisplayPropertyLink* lnk )
{
    FixedString lnktype = lnk->type();
    Threads::Locker lckr( lock_ );
    for ( int idx=propertylinks_.size()-1; idx>=0; idx-- )
    {
	if ( propertylinks_[idx]->type()!=lnktype )
	    continue;

	//If we have at least one holder in common, we can combine them
	bool hascommon = false;
	for ( int idy=lnk->holders_.size(); idy>=0; idy-- )
	{
	    if ( propertylinks_[idx]->holders_.isPresent(lnk->holders_[idy]) )
	    {
		hascommon = true;
		break;
	    }
	}

	if ( !hascommon )
	    continue;

	//Add everything that's not present in existing link from previous link
	for ( int idy=lnk->holders_.size(); idy>=0; idy-- )
	{
	    if ( !propertylinks_[idx]->holders_.isPresent(lnk->holders_[idy]) )
	    {
		if ( !propertylinks_[idx]->addHolder( lnk->holders_[idy] ) )
		{
		    pErrMsg("Something is wrong");
		}
	    }
	}

	delete lnk;

	return propertylinkids_[idx];
    }

    lnk->init();
    propertylinks_ += lnk;
    const int res = freeholderid_++;
    propertylinkids_ += res;

    return res;
}


void DisplayLinkManager::removeDisplayPropertyLink( int id )
{
    Threads::Locker lckr( lock_ );
    const int idx = propertylinkids_.indexOf( id );
    if ( idx<0 )
	return;

    delete propertylinks_.removeSingle( idx );
    propertylinkids_.removeSingle( idx );
}


const DisplayPropertyLink*
    DisplayLinkManager::getDisplayPropertyLink( int id ) const 
{ return const_cast<DisplayLinkManager*>(this)->getDisplayPropertyLink( id ); }


DisplayPropertyLink* DisplayLinkManager::getDisplayPropertyLink( int id )
{
    Threads::Locker lckr( lock_ );
    const int idx = propertylinkids_.indexOf( id );
    if ( idx<0 )
        return 0;

    return propertylinks_[idx];
}


void DisplayLinkManager::removeHolder( DisplayPropertyHolder* hldr )
{
    Threads::Locker lckr( lock_ );
    if ( !holders_.isPresent( hldr ) )
	return;

    for ( int idx=propertylinks_.size()-1; idx>=0; idx-- )
    {
	DisplayPropertyLink& link = *propertylinks_[idx];
	for ( int idy=link.nrDisplayPropertyHolders()-1; idy>=0; idy-- )
	{
	    if ( link.getDisplayPropertyHolder( idy ) == hldr )
	    {
		link.removeHolder( hldr );
		if ( !link.isValid() )
		{
		    delete propertylinks_.removeSingle( idx );
		    break;
		}
	    }
	}
    }

    holders_ -= hldr;
}
