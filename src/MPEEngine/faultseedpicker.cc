/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "faultseedpicker.h"

#include "emfault.h"
#include "emmanager.h"


namespace MPE 
{

FaultSeedPicker::FaultSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , sectionid_( -1 )
    , isactive_( false )
    , nrseeds_(0)
{}


bool FaultSeedPicker::canSetSectionID() const
{ return true; }


bool FaultSeedPicker::setSectionID( const EM::SectionID& sid )
{ sectionid_ = sid; return true; }


EM::SectionID FaultSeedPicker::getSectionID() const
{ return sectionid_; }


bool FaultSeedPicker::startSeedPick()
{
    if ( !sectionIsEmpty() )
	return false;

    const EM::ObjectID emobjid = tracker_.objectID();
    mDynamicCastGet(EM::Fault*,fault,EM::EMM().getObject(emobjid));
    didchecksupport_ = fault->enableGeometryChecks( false );

    isactive_ = true;
    nrseeds_ = 0;
    return true;
}


bool FaultSeedPicker::canAddSeed() const
{ return true; }


bool FaultSeedPicker::addSeed( const Coord3& pos, bool )
{
    const EM::ObjectID emobjid = tracker_.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );

    if ( sectionIsEmpty() )
    {
	stickstart_ = RowCol(0,0);

	//inc nrseeds_ here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds_++;
	const EM::PosID pid( fault->id(), sectionid_,
			     stickstart_.toInt64() );
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds_--;
	    return false;
	}

	fault->setPosAttrib( pid, EM::EMObject::sSeedNode(), true );
    }
    else if ( nrSeeds()==1 )
    {
	const Coord3 existingpos = fault->getPos( sectionid_, 0 );
	bool newstick = false; //TODO
	const RowCol newseedrc = stickstep_ = newstick
	    ? RowCol( 0, 1 )
	    : RowCol( existingpos.z<pos.z?1:-1, 0);

	//inc nrseeds_ here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds_++;
	const EM::PosID pid( fault->id(), sectionid_,newseedrc.toInt64());
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds_--;
	    return false;
	}

	fault->setPosAttrib( pid, EM::EMObject::sSeedNode(), true );
    }
    else
    {
	/*
	const RowCol newseedrc = getNewSeedRc( pos );
	const EM::PosID pid( fault->id(), sectionid_,newseedrc.toInt64());
	if ( fault->isDefined(pid) )
	{
	    EM::FaultGeometry& geom = fault->geometry();
	    if ( isrowstick_
		    ? !geom.insertCol(sectionid_,newseedrc.col,true)
		    : !geom.insertRow(sectionid_,newseedrc.row,true))
		return false;
	}

	//inc nrseeds_ here since fault->setPos will trigger cbs that will 
	//ask the nr of seeds. 
	nrseeds_++;
	if ( !fault->setPos( pid, pos, true ) )
	{
	    nrseeds_--;
	    return false;
	}
	fault->setPosAttrib( pid, EM::EMObject::sSeedNode, true );
	*/
    }

    return true;
}


bool FaultSeedPicker::canRemoveSeed() const
{ return false; }


bool FaultSeedPicker::removeSeed( const EM::PosID& pid, bool enviromment,
       				  bool retrack )
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );
    emobj->setPosAttrib( pid, EM::EMObject::sSeedNode(), false );
    emobj->unSetPos( pid, true );
    return true;
}


bool FaultSeedPicker::reTrack() { return true; }


int FaultSeedPicker::nrSeeds() const { return nrseeds_; }


bool FaultSeedPicker::stopSeedPick(bool iscancel)
{
    const EM::ObjectID emobjid = tracker_.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );
    fault->enableGeometryChecks( didchecksupport_ );

    return true;
}


const char* FaultSeedPicker::errMsg() const
{ return errmsg_.str(); }


bool FaultSeedPicker::sectionIsEmpty() const
{
    const EM::ObjectID emobjid = tracker_.objectID();
    const EM::EMObject* emobject = EM::EMM().getObject(emobjid);
    PtrMan<EM::EMObjectIterator> iterator = emobject->createIterator(sectionid_);

    if ( !iterator )
	return true;
    
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
    return RowCol(-1,-1);
/*
    const EM::ObjectID emobjid = tracker_.objectID();
    mDynamicCastGet( EM::Fault*, fault, EM::EMM().getObject(emobjid) );

    if ( !isrowstick_ )
    {
	const int inc = stickstep_.row>0 ? 1 : -1;
	const float compz = pos.z*inc;
	for ( int idx=0; idx<nrseeds_; idx++ )
	{
	    const float seedcompz = fault->getPos( sectionid_,
			(stickstart_+stickstep_*idx).toInt64() ).z * inc;
	    if ( seedcompz>compz )
	    {
		if ( !idx )
		    return stickstart_-stickstep_;

		return stickstart_+stickstep_*idx;
	    }
	}

	return stickstart_+stickstep_*nrseeds_;
    }


    float minsqdist;
    RowCol closestrc;

    for ( int idx=0; idx<nrseeds_; idx++ )
    {
	const RowCol rc = stickstart_+stickstep_*idx;
	const Coord seedpos = fault->getPos( sectionid_, rc.toInt64() );
	const float sqdist = seedpos.sqDistTo( pos );

	if ( !idx || sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    closestrc = rc;
	}
    }

    const RowCol prevrc = closestrc-stickstep_;
    const RowCol nextrc = closestrc+stickstep_;

    const Coord3 prevknotpos =fault->getPos( sectionid_, prevrc.toInt64());
    const Coord3 closestpos =fault->getPos(sectionid_,closestrc.toInt64());
    const Coord3 nextknotpos =fault->getPos( sectionid_, nextrc.toInt64());

    if ( prevknotpos.isDefined() && nextknotpos.isDefined() )
    {
       const float prevdist = pos.distTo(prevknotpos);
       const float nextdist = pos.distTo(nextknotpos);
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
    */
}


const char* FaultSeedPicker::seedConModeText( int mode, bool abbrev)
{
    if ( (FaultSeedConnectMode)mode == DrawBetweenSeeds )
	return "Line manual";
    return "Unknown mode";
}


}; // namespace MPE

