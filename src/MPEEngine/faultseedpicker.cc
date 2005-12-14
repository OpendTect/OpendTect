/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faultseedpicker.cc,v 1.1 2005-12-14 16:50:34 cvskris Exp $";

#include "faultseedpicker.h"

#include "emfault.h"
#include "emmanager.h"


namespace MPE 
{

FaultSeedPicker::FaultSeedPicker( MPE::EMTracker& t )
    : tracker( t )
    , sectionid( -1 )
    , isactive( false )
{}


bool FaultSeedPicker::canSetSectionID() const
{ return true; }


bool FaultSeedPicker::setSectionID( const EM::SectionID& sid )
{ sectionid = sid; return true; }


EM::SectionID FaultSeedPicker::getSectionID() const
{ return sectionid; }


bool FaultSeedPicker::startSeedPick()
{
    if ( !sectionIsEmpty() )
	return false;

    const EM::ObjectID emobjid = tracker.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );
    didchecksupport = fault->geometry.checkSupport(false);

    isactive = true;
    nrseeds = 0;
    return true;
}


bool FaultSeedPicker::canAddSeed() const
{ return sectionIsEmpty(); }


bool FaultSeedPicker::addSeed( const Coord3& pos )
{
    const EM::ObjectID emobjid = tracker.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );

    if ( sectionIsEmpty() )
    {
	stickstart = RowCol(0,0);

	//inc nrseeds here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds++;
	const EM::PosID pid( fault->id(), sectionid,stickstart.getSerialized());
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds--;
	    return false;
	}

	fault->setPosAttrib( pid, EM::EMObject::sSeedNode, true );

    }
    else if ( nrSeeds()==1 )
    {
	const Coord3 existingpos = fault->getPos( sectionid, 0 );
	isrowstick = mIsEqual(existingpos.z,pos.z,mDefEps );
	const RowCol newseedrc = stickstep = isrowstick
	    ? RowCol( 0, fault->geometry.step().row )
	    : RowCol( (existingpos.z<pos.z?1:-1)*fault->geometry.step().col, 0);

	//inc nrseeds here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds++;
	const EM::PosID pid( fault->id(), sectionid, newseedrc.getSerialized());
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds--;
	    return false;
	}
	fault->setPosAttrib( pid, EM::EMObject::sSeedNode, true );
    }
    else
    {
	if ( isrowstick )
	{
	    const Coord3 existingpos = fault->getPos( sectionid, 0 );
	    if ( !mIsEqual(existingpos.z,pos.z,mDefEps ) )
	    {
		errmsg = "New seed must be at the same depth as the previous"
		         " ones.";
		return false;
	    }

	}

	const RowCol newseedrc = getNewSeedRc( pos );
	const EM::PosID pid( fault->id(), sectionid,newseedrc.getSerialized());
	if ( fault->isDefined(pid) )
	{
	    if ( isrowstick
		    ? !fault->geometry.insertCol(sectionid,newseedrc.col,true)
		    : !fault->geometry.insertRow(sectionid,newseedrc.row,true) )
		return false;
	}

	//inc nrseeds here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds++;
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds--;
	    return false;
	}
	fault->setPosAttrib( pid, EM::EMObject::sSeedNode, true );
    }

    return true;
}


bool FaultSeedPicker::canRemoveSeed() const
{ return false; }


bool FaultSeedPicker::removeSeed( const EM::PosID& )
{
    return false;
}


bool FaultSeedPicker::reTrack() { return true; }


int FaultSeedPicker::nrSeeds() const { return nrseeds; }


bool FaultSeedPicker::stopSeedPick(bool iscancel)
{
    const EM::ObjectID emobjid = tracker.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );
    fault->geometry.checkSupport(didchecksupport);

    return true;
}


const char* FaultSeedPicker::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }


bool FaultSeedPicker::sectionIsEmpty() const
{
    const EM::ObjectID emobjid = tracker.objectID();
    const EM::EMObject* emobject = EM::EMM().getObject(emobjid);
    PtrMan<EM::EMObjectIterator> iterator = emobject->createIterator(sectionid);
    
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	if ( emobject->isDefined(pid) )
	    return false;
    }

    return true;
}


RowCol FaultSeedPicker::getNewSeedRc( const Coord3& pos ) const
{
    const EM::ObjectID emobjid = tracker.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );

    if ( !isrowstick )
    {
	const int inc = stickstep.row>0 ? 1 : -1;
	const float compz = pos.z*inc;
	for ( int idx=0; idx<nrseeds; idx++ )
	{
	    const float seedcompz = fault->getPos( sectionid,
			(stickstart+stickstep*idx).getSerialized() ).z * inc;
	    if ( seedcompz>compz )
	    {
		if ( !idx )
		    return stickstart-stickstep;

		return stickstart+stickstep*idx;
	    }
	}

	return stickstart+stickstep*nrseeds;
    }


    float minsqdist;
    RowCol closestrc;

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const RowCol rc = stickstart+stickstep*idx;
	const Coord seedpos = fault->getPos( sectionid, rc.getSerialized() );
	const float sqdist = seedpos.sqDistance( pos );

	if ( !idx || sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    closestrc = rc;
	}
    }

    const RowCol prevrc = closestrc-stickstep;
    const RowCol nextrc = closestrc+stickstep;

    const Coord3 prevknotpos =fault->getPos( sectionid, prevrc.getSerialized());
    const Coord3 closestpos =fault->getPos(sectionid,closestrc.getSerialized());
    const Coord3 nextknotpos =fault->getPos( sectionid, nextrc.getSerialized());

    if ( prevknotpos.isDefined() && nextknotpos.isDefined() )
    {
       const float prevdist = pos.distance(prevknotpos);
       const float nextdist = pos.distance(nextknotpos);
       return prevdist<nextdist ? closestrc : nextrc;
    }
    else if ( prevknotpos.isDefined() )
    {
       const Coord3 backvec = (closestpos-prevknotpos).normalize();
       const Coord3 forwardvec = (closestpos-pos).normalize();

       return backvec.dot(forwardvec)>M_SQRT1_2 ? closestrc : nextrc;
    }
    else if ( nextknotpos.isDefined() )
    {
	const Coord3 backvec = (closestpos-nextknotpos).normalize();
	const Coord3 forwardvec = (closestpos-pos).normalize();

	return backvec.dot(forwardvec)>M_SQRT1_2 ? nextrc : closestrc;
    }

    pErrMsg("Hmm");
    return RowCol(-1,-1);
}



}; // namespace MPE

