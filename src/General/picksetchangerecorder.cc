/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetchangerecorder.h"
#include "pickset.h"
#include "keystrs.h"
#include "uistrings.h"

namespace Pick
{
    class UdfSetChangeRecord : public SetChangeRecord
    {
    public:
	UdfSetChangeRecord()
	    : SetChangeRecord(Set::LocID::getInvalid(),Location::udf()) {}
	uiString name() const	{ return uiStrings::sNone(); }
	Record* clone() const	{ return new UdfSetChangeRecord(*this); }
	void	doApply(Set&,bool) const  {}
    };

    static const UdfSetChangeRecord udfchgrec_;
    const SetChangeRecord& SetChangeRecord::udf() { return udfchgrec_;}
}


void Pick::SetChangeRecord::replaceID( Pick::Set& ps,
					 LocID tmpid, LocID realid ) const
{
    ps.replaceID( tmpid, realid );
}


bool Pick::SetChangeRecord::apply( Monitorable& obj, Action act ) const
{
    mDynamicCastGet( Set*, ps, &obj );
    if ( !ps )
	{ pErrMsg("Huh"); return false; }

    doApply( *ps, act == ChangeRecorder::Undo );
    return true;
}


uiString Pick::SetLocCreateRecord::name() const
{
    return uiStrings::sCreate();
}


void Pick::SetLocCreateRecord::doApply( Set& ps, bool isundo ) const
{
    if ( isundo )
	ps.remove( locid_ );
    else
    {
	LocID newid = ps.insertBefore( beforeid_, loc_ );
	replaceID( ps, newid, locid_ );
    }
}


uiString Pick::SetLocMoveRecord::name() const
{
    return uiStrings::sMove();
}


void Pick::SetLocMoveRecord::doApply( Set& ps, bool isundo ) const
{
    if ( isundo )
	ps.set( locid_, prevloc_ );
    else
	ps.set( locid_, loc_ );
}


uiString Pick::SetLocRemoveRecord::name() const
{
    return uiStrings::sRemove();
}


void Pick::SetLocRemoveRecord::doApply( Set& ps, bool isundo ) const
{
    if ( !isundo )
	ps.remove( locid_ );
    else
    {
	LocID newid = ps.insertBefore( beforeid_, loc_ );
	replaceID( ps, newid, locid_ );
    }
}


Pick::SetChangeRecorder::SetChangeRecorder( Set& ps )
    : ChangeRecorder(ps,sKey::PickSet())
{
}


Pick::SetChangeRecorder::SetChangeRecorder( const Set& ps )
    : ChangeRecorder(ps,sKey::PickSet())
{
}


Pick::SetChangeRecorder::SetChangeRecorder( const SetChangeRecorder& oth )
    : ChangeRecorder(oth)
{
}


mImplMonitorableAssignmentWithNoMembers(Pick::SetChangeRecorder,ChangeRecorder)


void Pick::SetChangeRecorder::handleObjChange( const ChangeData& chgdata )
{
    mLock4Read();
    if ( !obj_ )
	return;

    if ( chgdata.isEntireObject() )
	{ clear(); return; }

    mDynamicCastGet( Set&, ps, *obj_ );
    const ChangeType chgtyp = chgdata.changeType();
    if ( !Set::isLocationUpdate(chgtyp) )
	return;

    mGetIDFromChgData( LocID, locid, chgdata );
    const Location loc = ps.get( locid );
    if ( chgtyp == Set::cLocationPreChange() )
    {
	Location& movestartloc = movestartloc_.getObject();
	if ( movestartloc.isUdf() )
	    movestartloc = loc;
	return;
    }

    if ( Set::isTempChange(chgtyp) )
	return;

    if ( chgtyp == Set::cLocationChange() )
    {
	Location& movestartloc = movestartloc_.getObject();
	if ( movestartloc.isUdf() )
	    { pErrMsg("Huh"); return; }
	mLock2Write();
	addRec( new SetLocMoveRecord( locid, movestartloc, loc ) );
	movestartloc.setUdf();
    }
    else
    {
	const LocID nextlocid = ps.locIDAfter( locid );
	mLock2Write();
	if ( chgtyp == Set::cLocationInsert() )
	    addRec( new SetLocCreateRecord( locid, loc, nextlocid ) );
	else
	    addRec( new SetLocRemoveRecord( locid, loc, nextlocid ) );
    }
}
