/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocchain.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "survinfo.h"


VolProc::Chain::Connection::Connection( Step::ID outpstepid,
			       Step::OutputSlotID outpslotid,
			       Step::ID inpstepid,
			       Step::InputSlotID inpslotid )
    : outputstepid_( outpstepid )
    , outputslotid_( outpslotid )
    , inputstepid_( inpstepid )
    , inputslotid_( inpslotid )
{

}


bool VolProc::Chain::Connection::isUdf() const
{
    return  mIsUdf( outputstepid_ ) ||
	    mIsUdf( outputslotid_) ||
	    mIsUdf( inputstepid_) ||
	    mIsUdf( inputslotid_ );
}


bool VolProc::Chain::Connection::operator==( const Chain::Connection& b ) const
{
    return outputstepid_==b.outputstepid_ &&
	    outputslotid_==b.outputslotid_ &&
	    inputstepid_==b.inputstepid_ &&
	    inputslotid_==b.inputslotid_;
}


void VolProc::Chain::Connection::fillPar( IOPar& iopar, const char* key ) const
{
    iopar.set( key, outputstepid_, outputslotid_,
	       inputstepid_, inputslotid_ );
}


bool VolProc::Chain::Connection::usePar( const IOPar& iopar, const char* key )
{
    return iopar.get( key, outputstepid_, outputslotid_,
		      inputstepid_, inputslotid_ );
}


bool VolProc::Chain::Connection::operator!=( const Chain::Connection& b ) const
{
    return !((*this)==b);
}


VolProc::Chain::Chain()
    : zstep_( SI().zRange(true).step )
    , zist_( SI().zIsTime() )
    , freeid_( 0 )
    , outputstepid_( Step::cUndefID() )
    , outputslotid_( Step::cUndefSlotID() )
{
    outcompscalers_.allowNull( true );
}


VolProc::Chain::~Chain()
{
    deepErase( steps_ );
    deepErase( outcompscalers_ );
}


bool VolProc::Chain::addConnection( const Chain::Connection& conn )
{
    if ( !validConnection(conn) )
	return false;

    web_.getConnections().addIfNew( conn );
    return true;
}


void VolProc::Chain::removeConnection( const Chain::Connection& conn )
{
    web_.getConnections() -= conn;
}


void VolProc::Chain::updateConnections()
{
    const Chain::Web oldweb = web_;
    web_.getConnections().erase();

    for ( int idx=1; idx<steps_.size(); idx++ )
    {
	Step* step = steps_[idx];
	if ( step->isInputPrevStep() )
	{
	    Step* prevstep = steps_[idx-1];
	    Chain::Connection connection( prevstep->getID(), 0,
		    step->getID(), step->getInputSlotID(0) );
	    addConnection( connection );
	}
	else
	{
	    TypeSet<Chain::Connection> conns;
	    oldweb.getConnections( step->getID(), true, conns );
	    for ( int cidx=0; cidx<conns.size(); cidx++ )
		addConnection( conns[cidx] );
	}
    }
}


bool VolProc::Chain::validConnection( const Chain::Connection& conn ) const
{
    if ( conn.isUdf() )
	return false;

    const Step* outputstep = getStepFromID( conn.outputstepid_ );
    if ( !outputstep || !outputstep->validOutputSlotID(conn.outputslotid_) )
	return false;

    const Step* inputstep = getStepFromID( conn.inputstepid_ );
    if ( !inputstep || !inputstep->validInputSlotID(conn.inputslotid_) )
	return false;

    return true;
}


int VolProc::Chain::nrSteps() const
{
    return steps_.size();
}


VolProc::Step* VolProc::Chain::getStep( int idx )
{
    return steps_.validIdx(idx) ? steps_[idx] : 0;
}


VolProc::Step* VolProc::Chain::getStepFromID( Step::ID id )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
	if ( steps_[idx]->getID()==id )
	    return steps_[idx];

    return 0;
}


const VolProc::Step* VolProc::Chain::getStepFromID( Step::ID id ) const
{
    return const_cast<Chain*>( this )->getStepFromID( id );
}


VolProc::Step* VolProc::Chain::getStepFromName( const char* nm )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	const FixedString usrnm = steps_[idx]->userName();
	if ( usrnm == nm )
	    return steps_[idx];
    }

    return 0;
}


const VolProc::Step* VolProc::Chain::getStepFromName( const char* nm ) const
{
    return const_cast<Chain*>(this)->getStepFromName( nm );
}


int VolProc::Chain::indexOf( const Step* stp ) const
{
    return steps_.indexOf( stp );
}


void VolProc::Chain::addStep( Step* stp )
{
    stp->setChain( *this );
    steps_ += stp;
}


void VolProc::Chain::insertStep( int idx, Step* stp )
{
    steps_.insertAt( stp, idx );
}


void VolProc::Chain::swapSteps( int o1, int o2 )
{
    steps_.swap( o1, o2 );
    updateConnections();
}


void VolProc::Chain::removeStep( int sidx )
{
    if ( !steps_.validIdx(sidx) ) return;

    delete steps_.removeSingle( sidx );
    updateConnections();
}


int VolProc::Chain::getNrUsers( Step::ID stepid,
				Step::InputSlotID inpslotid ) const
{
    TypeSet<Connection> inpconnections;
    web_.getConnections( stepid, true, inpconnections );
    Step::ID senderid = Step::cUndefID();
    for ( int idx=0; idx<inpconnections.size(); idx++ )
    {
	if ( inpconnections[idx].inputslotid_ != inpslotid )
	    continue;

	senderid = inpconnections[idx].outputstepid_;
	break;
    }

    if ( senderid == Step::cUndefID() )
	return 0;

    TypeSet<Connection> outconnections;
    web_.getConnections( senderid, false, outconnections );

    return outconnections.size();
}


const VelocityDesc* VolProc::Chain::getVelDesc() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( steps_[idx]->getVelDesc() )
	    return steps_[idx]->getVelDesc();

    return 0;
}


bool VolProc::Chain::areSamplesIndependent() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( !steps_[idx]->areSamplesIndependent() )
	    return false;

    return true;
}


bool VolProc::Chain::needsFullVolume() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( steps_[idx]->needsFullVolume() )
	    return true;

    return false;
}


void VolProc::Chain::fillPar( IOPar& par ) const
{
    par.set( sKeyNrSteps(), steps_.size() );
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	IOPar oppar;
	oppar.set( sKeyStepType(), steps_[idx]->factoryKeyword() );
	steps_[idx]->fillPar( oppar );

	par.mergeComp( oppar, toString(idx) );
    }

    BufferString key;
    const TypeSet<Chain::Connection>& conns = web_.getConnections();
    par.set( sKeyNrConnections(), conns.size() );
    for ( int idx=0; idx<conns.size(); idx++ )
	conns[idx].fillPar( par, sKeyConnection(idx,key)  );

    par.set( sKey::Output(), outputstepid_, outputslotid_ );
    const BufferString outscalerstr(
				IOPar::compKey(sKey::Output(),sKey::Scale()) );
    for ( int idx=0; idx<outcompscalers_.size(); idx++ )
    {
	if ( !outcompscalers_[idx] || outcompscalers_[idx]->isEmpty() )
	    continue;

	BufferString scalerstr( 256, false );
	outcompscalers_[idx]->put( scalerstr.getCStr(), scalerstr.bufSize() );
	par.set( IOPar::compKey(outscalerstr.str(),idx), scalerstr );
    }
}


const char* VolProc::Chain::sKeyConnection( int idx, BufferString& str )
{
    str = "Connection ";
    str += idx;
    return str.str();
}


bool VolProc::Chain::usePar( const IOPar& par )
{
    deepErase( steps_ );
    deepErase( outcompscalers_ );
    web_.getConnections().erase();

    const uiString parseerror = tr("Parsing error");

    int nrsteps;
    if ( !par.get(sKeyNrSteps(),nrsteps) )
    {
	errmsg_ = parseerror;
	return false;
    }

    int highestid = -1;
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	PtrMan<IOPar> steppar = par.subselect( toString(idx) );
	if ( !steppar )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	BufferString type;
	if ( !steppar->get(sKeyStepType(),type) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	Step* step = Step::factory().create( type.buf() );
	if ( !step )
	{
	    errmsg_ = tr( "Cannot create Volume processing step %1. "
			  "Perhaps all plugins are not loaded?")
			  .arg( type.buf() );

	    return false;
	}

	if ( !step->usePar( *steppar ) )
	{
	    errmsg_ = tr("Cannot parse Volume Processing's parameters.\n"
	    		 "Processing step: %1\n"
	    		 "of type: %2\n\n"
			 "%3")
			.arg(step->userName()).arg(step->factoryDisplayName())
			.arg( step->errMsg() );
	    delete step;
	    return false;
	}

	if ( step->id_ > highestid )
	    highestid = step->id_;

	addStep( step );
    }

    if ( highestid > freeid_ )
	freeid_ = highestid+1;

    int nrconns;
    if ( par.get(sKeyNrConnections(),nrconns) )
    {
	Step::ID outputstepid;
	Step::OutputSlotID outputslotid;
	if ( !par.get(sKey::Output(),outputstepid,outputslotid) ||
	     !setOutputSlot(outputstepid,outputslotid) )
	{
	    errmsg_ = tr("Cannot parse or set output slot.");
	    return false;
	}

	BufferString key;
	for ( int idx=0; idx<nrconns; idx++ )
	{
	    Connection newconn;
	    if ( !newconn.usePar( par, sKeyConnection(idx,key) ) )
	    {
		errmsg_ = tr("Cannot parse Connection %1").arg( toString(idx) );
		return false;
	    }

	    if ( !addConnection(newconn) )
	    {
		errmsg_ = uiStrings::phrCannotAdd(tr("connection %1")
				    .arg( toString(idx)) );
		return false;
	    }
	}
    }
    else if ( steps_.size() ) //Old format, all connections implicit
    {
	for ( int idx=1; idx<steps_.size(); idx++ )
	{
	    Connection conn;
	    conn.inputstepid_ = steps_[idx]->getID();
	    conn.inputslotid_ = steps_[idx]->getInputSlotID( 0 );
	    conn.outputstepid_ = steps_[idx-1]->getID();
	    conn.outputslotid_ = steps_[idx-1]->getOutputSlotID( 0 );

	    if ( !addConnection( conn ) )
	    {
		pErrMsg("Should never happen");
		return false;
	    }
	}

	if ( !setOutputSlot( steps_.last()->getID(),
			     steps_.last()->getOutputSlotID( 0 ) ) )
	{
	    pErrMsg("Should never happen");
	    return false;
	}
    }

    const BufferString outscalerstr(
				IOPar::compKey(sKey::Output(),sKey::Scale()) );
    PtrMan<IOPar> scalerpar = par.subselect( outscalerstr );
    if ( scalerpar.ptr() )
    {
	for ( int idx=0; idx<scalerpar->size(); idx++ )
	{
	    const BufferString compidxstr( scalerpar->getKey(idx) );
	    const int compidx = compidxstr.toInt();
	    BufferString scalerstr;
	    if ( !scalerpar->get(compidxstr,scalerstr) || scalerstr.isEmpty() )
		continue;

	    Scaler* scaler = Scaler::get( scalerstr );
	    if ( !scaler ) continue;

	    for ( int idy=0; idy<=compidx; idy++ )
	    {
		if ( !outcompscalers_.validIdx(idy) )
		    outcompscalers_ += 0;
	    }

	    delete outcompscalers_.replace( compidx, scaler );
	}
    }

    return true;
}


void VolProc::Chain::setStorageID( const MultiID& mid )
{
    storageid_ = mid;
}


uiString VolProc::Chain::name() const
{
    PtrMan<IOObj>  ioobj = IOM().get( storageid_ );
    return !ioobj ? uiString::emptyString() :
	tr("Executing volume builder chain \'%1\'").arg(ioobj->uiName());
}


bool VolProc::Chain::setOutputSlot( Step::ID stepid, Step::OutputSlotID slotid )
{
    if ( steps_.size() > 1 )
    {
	const Step* step = getStepFromID( stepid );
	if ( !step || !step->validOutputSlotID(slotid) )
	    return false;
    }

    outputstepid_ = stepid;
    outputslotid_ = slotid;

    return true;
}


void VolProc::Chain::setOutputScalers( const ObjectSet<Scaler>& scalers )
{
    deepErase( outcompscalers_ );
    for ( int idx=0; idx<scalers.size(); idx++ )
	outcompscalers_ += scalers[idx] ? scalers[idx]->clone() : 0;
}


const ObjectSet<Scaler>& VolProc::Chain::getOutputScalers() const
{
    return outcompscalers_;
}


uiString VolProc::Chain::errMsg() const
{
    return errmsg_;
}


void VolProc::Chain::Web::getConnections( Step::ID stepid, bool isinput,
			    TypeSet<VolProc::Chain::Connection>& res ) const
{
    for ( int idx=0; idx<connections_.size(); idx++ )
	if ( (isinput && connections_[idx].inputstepid_==stepid) ||
	     (!isinput && connections_[idx].outputstepid_==stepid))
		res += connections_[idx];
}
