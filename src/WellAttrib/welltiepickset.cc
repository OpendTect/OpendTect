/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.10 2009-06-24 09:03:55 cvsbruno Exp $";

#include "welltiepickset.h"

#include "welltiedata.h"

#include "arrayndimpl.h"
#include "valseriesevent.h"
#include "sorting.h"
#include "welldata.h"
#include "welld2tmodel.h"

WellTiePickSetMGR::WellTiePickSetMGR( const Well::Data* wd )
	: CallBacker(CallBacker::CallBacker())
    	, wd_(wd)
{
}

WellTiePickSetMGR::~WellTiePickSetMGR()
{}


void WellTiePickSetMGR::setDataParams( const WellTieParams::DataParams* dpms )
{
    datapms_ = dpms;			    
}


void WellTiePickSetMGR::setData( const WellTieDataSet* data )
{
    dispdata_ = data;
}


void WellTiePickSetMGR::setEventType( int seltype )
{
    if ( seltype==1 )
	evtype_ = VSEvent::Max;
    else if ( seltype==2 )
	evtype_ = VSEvent::Min;
    else if ( seltype==3 )
	evtype_ = VSEvent::ZC;
    else 
	evtype_ = VSEvent::Extr;
}


void WellTiePickSetMGR::addPick( float vwrszstart, float vwrszstop, 
				 float xpos, float zpos )
{
    const int seissz = seispickset_.getSize();
    const int synthsz = synthpickset_.getSize();
    if ( abs(seissz-synthsz)<2 )
    {
	if ( xpos<(vwrszstop-vwrszstart)/2 )
	{
	    if ( abs(synthsz+1-seissz) < 2 )
		synthpickset_.add( 0, xpos, findEventDah(zpos,true) );
	}
	else
	{
	    if ( abs(seissz+1-synthsz) < 2 )
		seispickset_.add( 0, xpos, findEventDah(zpos,false) );
	}
    }
}


float WellTiePickSetMGR::findEventDah( float zpos, bool issynth )
{
    if ( evtype_ == VSEvent::None ) 
	evtype_ = VSEvent::Extr;
    const int posidx = dispdata_->getIdx( zpos );  
    const char* colnm = issynth ? datapms_->synthnm_ : datapms_->attrnm_; 
    const int maxidx = dispdata_->getLength()-1;
    Interval<float> intv ( posidx, maxidx );
    SamplingData<int> sd;
    ValueSeriesEvFinder<float,float> evf( *dispdata_->get(colnm), maxidx, sd );
    const int evpos = mNINT( evf.find( evtype_, intv ).pos );
    if ( evpos > maxidx || evpos <0 )
	return zpos;

    return dispdata_->get( datapms_->dptnm_, evpos );
}


void WellTiePickSetMGR::updateShift( int vwridx, float curpos )
{
    /*used only for log stretch and squueze...
    logpickset_.setMousePos( curpos );
    mousemoving.trigger();*/
}


void WellTiePickSetMGR::clearAllPicks()
{
    logpickset_.clearAll();
    seispickset_.clearAll();
    synthpickset_.clearAll();
}


void WellTiePickSetMGR::clearLastPicks()
{
    seispickset_.clear( seispickset_.getSize()-1 );
    synthpickset_.clear( synthpickset_.getSize()-1 );
}


bool WellTiePickSetMGR::checkIfPick()
{
    if ( seispickset_.getSize() || synthpickset_.getSize())
	return true;
    else
	return false;
}


void WellTiePickSetMGR::sortByDah( WellTiePickSet& pickset )
{
    const int sz = pickset.getSize();
    TypeSet<float> zvals;
    for ( int idx=0; idx<sz; idx++ )
	zvals += pickset.getDah(idx);

    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int idx=0; idx<sz; idx++ )
	pickset.setDah( idx, zvals[idx]  );
}




WellTiePickSet::WellTiePickSet()
	: mousepos_(0)
	, nrpickstotal_(0)
   	, pickadded(this)
{
}


WellTiePickSet::~WellTiePickSet()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::add( int vwridx, float xpos, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx; pick->color_ = Color::DgbColor();
    pick->xpos_ = xpos;   pick->dah_ = zpos;
    pickset_ += pick;
    nrpickstotal_++;
    pickadded.trigger();
}


void WellTiePickSet::clear( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::clearAll()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::updateSupPickedPos( float& lastpos, float curpos,
       						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getDah( idx );
	if ( pos > curpos ) 
	{	
	    if ( ( pos - curpos ) < ( lastpos - curpos ) )
		lastpos = pos;
	}
    }
}


void WellTiePickSet::updateInfPickedPos( float& firstpos, float curpos,
						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getDah( idx );
	if ( pos < curpos )
	{
	    if ( (pos - curpos) > ( firstpos - curpos) )
		firstpos = pos;
	}
    }
}

