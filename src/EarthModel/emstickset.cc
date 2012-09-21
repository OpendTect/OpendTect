
/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "emstickset.h"

#include "emmanager.h"
#include "emsticksettransl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"


const char* EM::StickSet::typeStr()
{ return  EMStickSetTranslatorGroup::keyword; }


EM::EMObject* EM::StickSet::create(EMManager& emm)
{ return new EM::StickSet( emm ); }


void EM::StickSet::initClass(EM::EMManager& emm)
{
    emm.addFactory( new ObjectFactory( create,
			 EMStickSetTranslatorGroup::ioContext(),
			 typeStr()) );
}


EM::StickSet::StickSet( EMManager& man )
    : EMObject( man )
{}


EM::StickSet::~StickSet()
{
    cleanUp();
}


int EM::StickSet::nrSticks() const
{
    return sticks.size();
}


EM::StickID EM::StickSet::stickID(int idx) const
{
    return stickids[idx];
}


EM::StickID EM::StickSet::addStick(bool addtoundo)
{
    StickID stickid = 0;
    while ( getStickIndex(stickid)!=-1 ) stickid++;

    sticks += new TypeSet<BinIDValue>;
    stickids += stickid;
    firstknots += -1;

    //TODO Add to undo
    
    return stickid;
}


void EM::StickSet::removeStick(const StickID& stickid)
{
    const int idx = getStickIndex(stickid);
    if ( idx<0 )
	return;

    delete sticks[idx];
    sticks.remove(idx);
    stickids.remove(idx);
    firstknots.remove(idx);
}


int EM::StickSet::nrKnots(const StickID& stickid) const
{
    const int idx = getStickIndex(stickid);

    if ( idx<0 )
	return 0;

    return sticks[idx]->size();
}


EM::KnotID EM::StickSet::firstKnot(const StickID& stickid) const
{
    const int idx = getStickIndex(stickid);

    if ( idx<0 )
	return -1;

    return firstknots[idx];
}


bool EM::StickSet::setPos( const StickID& stickid, const KnotID& knotid,
			   const Coord3& newpos, bool addtoundo )
{
    const int idx = getStickIndex(stickid);

    if ( idx<0 )
	return false;

    BinIDValue bidval( SI().transform( newpos ), newpos.z );

    TypeSet<BinIDValue>& stick = *sticks[idx];
    const int firstknot = firstknots[idx];

    int relpos = knotid-firstknot;
    if ( stick.isEmpty() || relpos==stick.size() )
    {
	if ( stick.isEmpty() )
	{
	    firstknots[idx] = knotid;
	}

	stick += bidval;
	
    }
    else if ( relpos==-1 )
    {
	stick.insert(0, bidval);
	firstknots[idx] = knotid;
	return true;
    }
    else if ( relpos>stick.size() || relpos<-1 )
	return false;
    else
    {
	stick[relpos]=bidval;
    }

    //TODO Add to undo
    return true;
}


bool EM::StickSet::setPos( const EM::PosID& posid, const Coord3& newpos,
			   bool addtoundo )
{
    return setPos( posid.sectionID(), posid.subID(), newpos, addtoundo );
}


Coord3 EM::StickSet::getPos( const StickID& stickid,
			     const KnotID& knotid ) const
{
    const int index = getStickIndex(stickid);
    if ( index>=0 )
    {
	const int relpos = knotid-firstknots[index];
	if ( relpos>=0 && relpos<sticks[index]->size() )
	{
	    const BinIDValue& bid = (*sticks[index])[relpos];
	    return Coord3( SI().transform(bid.binid), bid.value );
	}
    }

    return Coord3::udf();
}


Coord3 EM::StickSet::getPos( const EM::PosID& posid ) const
{
    return getPos( posid.sectionID(), posid.subID() );
}


bool EM::StickSet::isLoaded() const
{
    return isloaded;
}


Executor* EM::StickSet::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    if ( !ioobj )
    {
	errmsg = "Cannot find the stickset object";
	return 0;
    }

    return EMStickSetTranslator::writer(*this, ioobj, errmsg);
}


Executor* EM::StickSet::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    if ( !ioobj )
    {
	errmsg = "Cannot find the stickset object";
	return 0;
    }

    return EMStickSetTranslator::reader(*this, ioobj, errmsg);
}




int EM::StickSet::getStickIndex(const StickID& stickid) const
{
    return stickids.indexOf(stickid);
}


void EM::StickSet::cleanUp()
{
    deepErase( sticks );
}


const IOObjContext& EM::StickSet::getIOObjContext() const
{ return EMStickSetTranslatorGroup::ioContext(); }
