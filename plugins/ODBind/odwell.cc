/*+
________________________________________________________________________________

 Copyright:  (C) 2022 dGB Beheer B.V.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/


#include "odsurvey.h"
#include "odwell.h"

#include "ioobj.h"
#include "latlong.h"
#include "math2.h"
#include "position.h"
#include "timedepthmodel.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "wellwriter.h"

#include <cmath>

float getDefaultTemporaryVelocity()
{ return Well::getDefaultVelocity() * 1.25f; }


odWell::odWell( const odSurvey& thesurvey, const char* name )
    : odSurveyObject(thesurvey, name, translatorGrp())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    wd_ = Well::MGR().get( ioobj->key(), Well::LoadReqs::AllNoLogs() );
    if ( !wd_ )
	errmsg_ = BufferString( "cannot load well data for: ", name );
}


odWell::odWell( const odSurvey& thesurvey, const char* name,
		double surfacecoord_x, double surfacecoord_y,
		float kbelev, float td, bool overwrite )
    : odSurveyObject(thesurvey, name, translatorGrp(), overwrite)
{
    if ( !errmsg_.isEmpty() )
	return;

    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    const float kb = getConvertedValue( kbelev, zduom, zsuom );
    const float totaldepth = getConvertedValue( td, zduom, zsuom );
    if ( mIsUdf(kb) || mIsUdf(totaldepth) || std::isnan(kb) ||
	 std::isnan(totaldepth) || totaldepth <= kb )
    {
	errmsg_ = "odWell::odWell - invalid kbelev or td.";
	return;
    }

    const Coord surfacecoord( surfacecoord_x, surfacecoord_y );
    RefMan<Well::Data> newwd = new Well::Data( name_.buf() );
    newwd->info().surfacecoord_ = surfacecoord;
    newwd->track().addPoint( Coord3(surfacecoord,-kb), 0.f );
    newwd->track().addPoint( Coord3(surfacecoord,totaldepth-kb), totaldepth );

    Well::D2TModel* d2t = new Well::D2TModel( "constant_velocty");
    newwd->info().replvel_ = Well::getDefaultVelocity();
    d2t->makeFromTrack( newwd->track(),
			getDefaultTemporaryVelocity(),
			newwd->info().replvel_ );
    newwd->setD2TModel( d2t );

    Well::Writer wtr( *ioobj, *newwd );
    if ( !wtr.put() )
    {
	errmsg_ = "odWell::odWell - cannot write well data: ";
	errmsg_.add( wtr.errMsg().getFullString() );
	return;
    }

    const MultiID key = ioobj->key();
    if ( Well::MGR().isLoaded(key) &&
	 !Well::MGR().reload(key,Well::LoadReqs::AllNoLogs()) )
    {
	errmsg_ = "odWell::odWell - cannot reload well data: ";
	errmsg_.add( Well::MGR().errMsg().getFullString() );
	return;
    }

    wd_ = Well::MGR().get( key, Well::LoadReqs::AllNoLogs() );
    if ( !wd_ )
    {
	errmsg_ = "odWell::odWell - cannot load well data for: ";
	errmsg_.add( name_.buf() );
    }
}


const Well::Data* odWell::wd() const
{
    survey_.activate();
    return wd_.ptr();
}


BufferStringSet* odWell::getLogNames() const
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
	return nullptr;

    auto* names = new BufferStringSet;
    wd_->logs().getNames( *names );
    return names;
}


void odWell::getLogInfo( OD::JSON::Array& jsarr,
			 const BufferStringSet& fornames ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    jsarr.setEmpty();
    PtrMan<BufferStringSet> allnms = getLogNames();
    if ( !allnms )
    {
	errmsg_ = "odWell::getLogInfo - invalid welldata object.";
	return;
    }

    BufferStringSet nms;
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    if ( !wd_ )
	return;

    const Well::LogSet& logs = wd_->logs();
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    for ( const auto* nm : nms )
    {
	const char* lognm = nm->buf();
	if ( !logs.isPresent(lognm) )
	    continue;

	const Interval<float> dahstored = logs.getDahRangeForLog( lognm );
	const Interval<float> dahrange(
			getConvertedValue(dahstored.start_, zsuom, zduom),
			getConvertedValue(dahstored.stop_, zsuom, zduom) );
	OD::JSON::Object loginfo;
	loginfo.set( "name", lognm );
	loginfo.set( "mnemonic", logs.getMnemonicLblOfLog(lognm) );
	loginfo.set( "uom", logs.getUnitOfMeasureLblOfLog(lognm) );
	loginfo.set( "dah_range", dahrange );
	loginfo.set( "log_range", logs.getValueRangeForLog(lognm) );
	jsarr.add( loginfo.clone() );
    }
}


BufferStringSet* odWell::getMarkerNames() const
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
	return nullptr;

    auto* names = new BufferStringSet;
    const Well::MarkerSet& ms = wd_->markers();
    ms.getNames( *names );
    return names;
}


void odWell::getMarkerInfo( OD::JSON::Array& jsarr,
			    const BufferStringSet& fornames ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    jsarr.setEmpty();
    PtrMan<BufferStringSet> allnms = getMarkerNames();
    if ( !allnms )
    {
	errmsg_ = "odWell::getMarkerInfo - invalid welldata object.";
	return;
    }

    BufferStringSet nms;
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    if ( !wd_ )
	return;

    const Well::MarkerSet& ms = wd_->markers();
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();

    for ( const auto* nm : nms )
    {
	const auto* marker = ms.getByName( nm->buf() );
	if ( !marker )
	    continue;

	OD::JSON::Object markerinfo;
	markerinfo.set( "name", marker->name() );
	markerinfo.set( "color", marker->color().getStdStr() );
	markerinfo.set( "dah", getConvertedValue(marker->dah(), zsuom, zduom) );
	jsarr.add( markerinfo.clone() );
    }
}


void odWell::getTrack( hAllocator allocator )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::getTrack - invalid welldata object.";
	return;
    }

    const Well::Track& wt = wd_->track();
    const int ndim = 1;
    int dims[ndim];
    dims[0] = wt.size();
    float* dah_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    float* tvdss_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    double* x_data = static_cast<double*>( allocator(ndim, dims, 'd') );
    double* y_data = static_cast<double*>( allocator(ndim, dims, 'd') );
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();

    for ( int idx=0; idx<dims[0]; idx++ )
    {
	const Coord3 pos = wt.pos( idx );
	*dah_data++ = getConvertedValue( wt.dah(idx), zsuom, zduom );
        *tvdss_data++ = getConvertedValue( pos.z_, zsuom, zduom );
        *x_data++ = pos.x_;
        *y_data++ = pos.y_;
    }
}


BufferStringSet* odWell::getD2TModelNames( bool checkshot ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
	return nullptr;

    auto* names = new BufferStringSet;
    const Well::LoadReqs lreqs( checkshot ? Well::CSMdl : Well::D2T );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wd_->multiID(), lreqs );
    if ( !wd )
	return names;

    if ( checkshot )
	wd->getCheckShotModelNames( *names );
    else
	wd->getD2TModelNames( *names );

    return names;
}


int odWell::getActiveD2TModelIndex( bool checkshot ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
	return -1;

    const Well::LoadReqs lreqs( checkshot ? Well::CSMdl : Well::D2T );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wd_->multiID(), lreqs );
    if ( !wd )
	return -1;

    const Well::D2TModel* active = checkshot ? wd->checkShotModel()
					     : wd->d2TModel();
    const int nrmodels = checkshot ? wd->nrCheckShotModels()
				   : wd->nrD2TModels();
    for ( int idx=0; active && idx<nrmodels; idx++ )
    {
	const Well::D2TModel* d2t = checkshot ? wd->checkShotModelByIndex( idx )
					      : wd->d2TModelByIndex( idx );
	if ( d2t == active )
	    return idx;
    }

    return -1;
}


void odWell::getD2TModel( hAllocator allocator, bool checkshot,
			  const char* name, int index )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::getD2TModel - invalid welldata object.";
	return;
    }

    const BufferString nm( name );
    const Well::LoadReqs lreqs( checkshot ? Well::CSMdl : Well::D2T );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wd_->multiID(), lreqs );
    const Well::D2TModel* d2t = nullptr;
    if ( wd && !nm.isEmpty() )
	d2t = checkshot ? wd->checkShotModelByName( nm )
			: wd->d2TModelByName( nm );
    else if ( wd && index >= 0 )
	d2t = checkshot ? wd->checkShotModelByIndex( index )
			: wd->d2TModelByIndex( index );
    else if ( wd )
	d2t = checkshot ? wd->checkShotModel() : wd->d2TModel();

    if ( !d2t )
    {
	errmsg_ = checkshot ? "odWell::getD2TModel - no checkshot model"
			    : "odWell::getD2TModel - no time-depth model";
	if ( !nm.isEmpty() )
	    errmsg_.add( " named: " ).add( nm );
	else if ( index >= 0 )
	    errmsg_.add( " at index: " ).add( index );

	return;
    }

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* tduom = UnitOfMeasure::surveyDefTimeUnit();
    const UnitOfMeasure* tsuom = UnitOfMeasure::surveyDefTimeStorageUnit();
    const int ndim = 1;
    int dims[ndim];
    dims[0] = d2t->size();
    float* dah_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    float* twt_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    for ( int idx=0; idx<dims[0]; idx++ )
    {
	*dah_data++ = getConvertedValue( d2t->dah(idx), zsuom, zduom );
	*twt_data++ = getConvertedValue( d2t->t(idx), tsuom, tduom );
    }
}


void odWell::getLogs( hAllocator allocator, const BufferStringSet& lognms,
		      OD::JSON::Object& jsuomobj, float zstep,
		      SampleMode samplemode )
{
    errmsg_.setEmpty();
    survey_.activate();
    jsuomobj.setEmpty();
    if ( !wd_ )
    {
	errmsg_ = "odWell::getLogs - invalid welldata object.";
	return;
    }

    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getLogNames();
    if ( lognms.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, lognms );

    const Well::LoadReqs lreqs( nms );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wd_->multiID(), lreqs );
    if ( !wd )
    {
	errmsg_ = "odWell::getLogs - cannot load requested logs";
	return;
    }

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    StepInterval<float> dahrg;
    dahrg.setUdf();
    dahrg.step_ = getConvertedValue( zstep, zduom, zsuom );
    const Well::LogSet& logs = wd->logs();
    for ( const auto* lognm : nms )
    {
	if ( logs.isPresent(lognm->buf()) )
	    dahrg.include( logs.getDahRangeForLog(lognm->buf()) );
    }

    const int ndim = 1;
    int dims[ndim];
    bool first = true;
    for ( const auto* lognm : nms )
    {
	const auto* log = logs.getLog( lognm->buf() );
	if ( !log || !log->isLoaded() )
	    continue;

	PtrMan<Well::Log> outlog;
	if ( samplemode==Upscale )
	    outlog = log->upScaleLog( dahrg );
	else
	    outlog = log->sampleLog( dahrg );

	dims[0] = outlog->size();
	if ( first )
	{
	    float* dah_data = static_cast<float*>(
					    allocator(ndim, dims, 'f') );
	    for ( int idx=0; idx<outlog->size(); idx++ )
		*dah_data++ = getConvertedValue( outlog->dah(idx),
						 zsuom, zduom );

	    first = false;
	    jsuomobj.set( "dah",
			 UnitOfMeasure::surveyDefDepthUnit()->getLabel() );
	}

	float* log_data = static_cast<float*>(
					    allocator(ndim, dims, 'f') );
	for ( int idx=0; idx<outlog->size();idx++ )
	    *log_data++ = mIsUdf(outlog->value(idx)) ? nanf("") :
						outlog->value(idx);

	jsuomobj.set( lognm->buf(), log->unitMeasLabel() );
    }
}


void odWell::getLog( hAllocator allocator, const char* lognm,
		     OD::JSON::Object& jsuomobj )
{
    survey_.activate();
    jsuomobj.setEmpty();
    errmsg_.setEmpty();
    if ( !wd_ )
    {
	errmsg_ = "odWell::getLog - invalid welldata object.";
	return;
    }

    const BufferString nm( lognm );
    if ( !wd_->logs().isPresent(nm) )
    {
	errmsg_ = "odWell::getLog - no log named: ";
	errmsg_.add( nm );
	return;
    }

    const Well::LoadReqs lreqs( nm );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wd_->multiID(), lreqs );
    const Well::Log* log = wd ? wd->logs().getLog( nm ) : nullptr;
    if ( !log || !log->isLoaded() )
    {
	errmsg_ = "odWell::getLog - cannot load requested log";
	return;
    }

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    const int ndim = 1;
    int dims[ndim];
    dims[0] = log->size();
    float* dah_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    for ( int idx=0; idx<log->size(); idx++ )
	*dah_data++ = getConvertedValue( log->dah(idx), zsuom, zduom );

    jsuomobj.set( "dah", zduom->getLabel() );
    float* log_data = static_cast<float*>( allocator(ndim, dims, 'f') );
    for ( int idx=0; idx<log->size(); idx++ )
	*log_data++ = mIsUdf(log->value(idx)) ? nanf("") : log->value(idx);

    jsuomobj.set( nm.buf(), log->unitMeasLabel() );
}

void odWell::putLog( const char* lognm, const float* dah, const float* logdata,
		     uint32_t sz, const char* uom, const char* mnem,
		     bool overwrite )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::putLog - invalid welldata object.";
	return;
    }

    if ( !overwrite && wd_->logs().isPresent(lognm) )
	return;

    if ( wd_->logs().isPresent(lognm) )
	Well::MGR().deleteLogs( wd_->multiID(), BufferStringSet(lognm) );

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    PtrMan<Well::Log> outlog = new Well::Log( lognm );
    if ( uom )
    {
	const UnitOfMeasure* loguom = UnitOfMeasure::getGuessed( uom );
	outlog->setUnitOfMeasure( loguom );
	if ( mnem )
	{
	    const BufferStringSet hintnms( lognm, mnem );
	    const Mnemonic* mn = MnemonicSelection::getGuessed( lognm, loguom,
								&hintnms );
	    if ( mn && !mn->isUdf() )
		outlog->setMnemonic( *mn );
	}
    }

    for ( uint32_t idz=0; idz<sz; idz++ )
    {
	const float dep = getConvertedValue( *dah++, zduom, zsuom );
	float logval = *logdata++;
	if ( std::isnan(logval) )
	    logval = mUdf(float);

	outlog->addValue( dep, logval );
    }
    outlog->updateAfterValueChanges();

    if ( !Well::MGR().writeAndRegister(wd_->multiID(), outlog) )
	errmsg_ = "odWell::putLog - saving log failed";
}


void odWell::putMarkers( const BufferStringSet& markernms, const float* dah,
			 const BufferStringSet& colors, bool overwrite )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::putMarkers - invalid welldata object.";
	return;
    }

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    const Well::MarkerSet origmarkers = wd_->markers();
    Well::MarkerSet& ms = wd_->markers();
    bool changed = false;
    for ( int idx=0; idx<markernms.size(); idx++ )
    {
	const char* markernm = markernms.get( idx ).buf();
	const int mrkidx = ms.indexOf( markernm );
	if ( mrkidx >= 0 )
	{
	    if ( !overwrite )
		continue;

	    delete ms.removeSingle( mrkidx );
	}

	OD::Color col;
	if ( colors.validIdx(idx) )
	    col.setStdStr( colors.get(idx).buf() );

	const float dep = getConvertedValue( dah[idx], zduom, zsuom );
	ms.insertNew( new Well::Marker(markernm,dep,col) );
	changed = true;
    }

    if ( !changed )
	return;

    Well::Writer wtr( wd_->multiID(), *wd_ );
    if ( !wtr.putMarkers() )
    {
	wd_->markers() = origmarkers;
	errmsg_ = "odWell::putMarkers - saving markers failed";
	return;
    }

    wd_->markerschanged.trigger();
}


void odWell::putTrack( const float* dah, const float* tvdss,
		       const double* x, const double* y, uint32_t sz )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::putTrack - invalid welldata object.";
	return;
    }

    if ( sz < 2 )
    {
	errmsg_ = "odWell::putTrack - a track needs at least two points.";
	return;
    }

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    Well::Track track;
    for ( uint32_t idx=0; idx<sz; idx++ )
    {
	const float md = getConvertedValue( dah[idx], zduom, zsuom );
	const float z = getConvertedValue( tvdss[idx], zduom, zsuom );
	if ( mIsUdf(md) || mIsUdf(z) || mIsUdf(x[idx]) || mIsUdf(y[idx]) ||
	     !Math::IsNormalNumber(md) || !Math::IsNormalNumber(z) ||
	     !Math::IsNormalNumber(x[idx]) || !Math::IsNormalNumber(y[idx]) )
	{
	    errmsg_ = "odWell::putTrack - undefined value in the track.";
	    return;
	}

	if ( idx>0 && md <= track.dah(idx-1) )
	{
	    errmsg_ = "odWell::putTrack - dah must increase.";
	    return;
	}

	track.addPoint( Coord3(x[idx],y[idx],z), md );
    }

    if ( !Well::MGR().get(wd_->multiID(),Well::LoadReqs(Well::D2T)) )
    {
	errmsg_ = "odWell::putTrack - cannot read the time-depth model.";
	return;
    }

    Well::D2TModel* d2t = wd_->d2TModel();
    TimeDepthModel dtmodel;
    if ( d2t && !d2t->getTimeDepthModel(*wd_,dtmodel) )
    {
	errmsg_ = "odWell::putTrack - the time-depth model is invalid.";
	return;
    }

    const Well::Track origtrack = wd_->track();
    const Coord origpos = wd_->info().surfacecoord_;
    const LatLong origllpos = wd_->info().surfacelatlong_;
    const float origreplvel = wd_->info().replvel_;
    PtrMan<Well::D2TModel> origd2t = d2t ? new Well::D2TModel( *d2t )
					 : nullptr;
    wd_->track() = track;
#define mRestore() \
    wd_->track() = origtrack; \
    wd_->info().surfacecoord_ = origpos; \
    wd_->info().surfacelatlong_ = origllpos; \
    wd_->info().replvel_ = origreplvel; \
    if ( d2t ) \
	*d2t = *origd2t;

    if ( d2t )
    {
	// The model as uiSetD2TFromOtherWell::acceptOK takes it, plus the
	// time at the TVDSS of each point of the new track
	const int mdlsz = dtmodel.size();
	TypeSet<double> depths, times;
	for ( int idx=0; idx<mdlsz; idx++ )
	{
	    depths += getConvertedValue( dtmodel.getDepth(idx), zduom, zsuom );
	    times += dtmodel.getTime( idx );
	}

	for ( int idx=0; idx<track.size(); idx++ )
	{
	    const float z = sCast(float,track.pos(idx).z_);
	    depths += z;
	    times += dtmodel.getTime( getConvertedValue(z,zsuom,zduom) );
	}

	uiString msg;
	if ( !d2t->ensureValid(*wd_,msg,&depths,&times) ||
	     !d2t->ensureValid(*wd_,msg) )
	{
	    mRestore()
	    errmsg_ = "odWell::putTrack - cannot project the time-depth "
		      "model onto the new track: ";
	    errmsg_.add( msg.getFullString() );
	    return;
	}
    }

    Well::Writer wtr( wd_->multiID(), *wd_ );
    if ( !wtr.putTrack() )
    {
	mRestore()
	errmsg_ = "odWell::putTrack - saving track failed";
	return;
    }

    if ( d2t && !wtr.putD2T() )
    {
	mRestore()
	wtr.putTrack();
	wtr.putD2T();
	errmsg_ = "odWell::putTrack - saving the time-depth model failed";
	return;
    }

#undef mRestore
    wd_->trackchanged.trigger();
    if ( d2t )
	wd_->d2tchanged.trigger();
}


void odWell::getInfo( OD::JSON::Object& jsobj ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    jsobj.setEmpty();
    if ( !wd_ )
	return;

    jsobj.set( "name", wd_->name().buf());
    jsobj.set( "uwid", wd_->info().uwid_.buf() );
    jsobj.set( "operator", wd_->info().oper_.buf() );
    jsobj.set( "field", wd_->info().field_.buf() );
    jsobj.set( "county", wd_->info().county_.buf() );
    jsobj.set( "state", wd_->info().state_.buf() );
    jsobj.set( "province", wd_->info().province_.buf() );
    jsobj.set( "country", wd_->info().country_.buf() );
    jsobj.set( "welltype", OD::toString(wd_->info().welltype_) );
    const Coord cd = wd_->info().surfacecoord_;
    jsobj.set( "x", cd.x_ );
    jsobj.set( "y", cd.y_ );

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    jsobj.set( "kb", getConvertedValue(wd_->track().getKbElev(),
				       zsuom, zduom) );
    jsobj.set( "td", getConvertedValue(wd_->track().td(), zsuom, zduom) );
    jsobj.set( "replacement_velocity", wd_->info().replvel_ );
    jsobj.set( "ground_elevation",
	       getConvertedValue(wd_->info().groundelev_, zsuom, zduom) );
    jsobj.set( "d2tmodel_count", wd_->nrD2TModels() );
}


void odWell::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
{
    errmsg_.setEmpty();
    jsobj.set( "type", "Feature" );
    auto* info = new OD::JSON::Object;
    getInfo( *info );
    jsobj.set( "properties", info );
    auto* geom = new OD::JSON::Object;
    geom->set( "type", "Point" );
    OD::JSON::Array* coords;
    if ( towgs )
	coords = new OD::JSON::Array( OD::JSON::DataType::String );
    else
	coords = new OD::JSON::Array(OD::JSON::DataType::Number);

    getPoints( *coords, towgs );
    geom->set( "coordinates", coords );
    jsobj.set( "geometry", geom );
}


void odWell::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !wd_ )
	return;

    auto coord = wd_->info().surfacecoord_;
    if ( towgs )
    {
	ConstRefMan<Coords::CoordSystem> coordsys =
						survey_.si().getCoordSystem();
	const LatLong ll( LatLong::transform(coord, true, coordsys.ptr()) );
	jsarr.add( strdup(toString(ll.lng_, 0, 'f', 6)) );
	jsarr.add( strdup(toString(ll.lat_, 0, 'f', 6)) );
    }
    else
    {
        jsarr.add( coord.x_ );
        jsarr.add( coord.y_ );
    }
}


BufferStringSet* odWell::getCommonMarkerNames( const odSurvey& survey,
					       const BufferStringSet& fornames )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames<odWell>( survey );
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    BufferStringSet common;
    for ( const auto* nm : nms )
    {
	odWell well( survey, *nm );
	PtrMan<BufferStringSet> marknms = well.getMarkerNames();
	if ( common.isEmpty() )
	    common = *marknms;
	else
	    common = odSurvey::getCommonItems( *marknms, common );
    }
    return common.clone();
}


BufferStringSet* odWell::getCommonLogNames( const odSurvey& survey,
					       const BufferStringSet& fornames )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames<odWell>( survey );
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    BufferStringSet common;
    for ( const auto* nm : nms )
    {
	odWell well( survey, *nm );
	PtrMan<BufferStringSet> lognms = well.getLogNames();
	if ( common.isEmpty() )
	    common = *lognms;
	else
	    common = odSurvey::getCommonItems( *lognms, common );
    }
    return common.clone();
}


mDefineBaseBindings(Well, well)
mDefineRemoveBindings(Well, well)

hWell well_newout( hSurvey survey, const char* name,
		   double x, double y, float kbelev, float td, bool overwrite )
{
    const auto* p = static_cast<const odSurvey*>(survey);
    if ( !p || !name )
	return nullptr;

    return new odWell( *p, name, x, y, kbelev, td, overwrite );
}


hStringSet well_lognames( hWell self )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p ) return nullptr;
    return p->getLogNames();
}


const char* well_loginfo( hWell self, const hStringSet fornms )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(fornms);
    if ( !p || !nms ) return nullptr;
    OD::JSON::Array jsarr( true );
    p->getLogInfo( jsarr, *nms );
    return strdup( jsarr.dumpJSon().buf() );
}


hStringSet well_markernames( hWell self )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p ) return nullptr;
    return p->getMarkerNames();
}


const char* well_markerinfo( hWell self, const hStringSet fornms )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(fornms);
    if ( !p || !nms ) return nullptr;
    OD::JSON::Array jsarr( true );
    p->getMarkerInfo( jsarr, *nms );
    return strdup( jsarr.dumpJSon().buf() );
}


void well_gettrack( hWell self, hAllocator allocator )
{
    auto* p = static_cast<odWell*>(self);
    if ( p )
	p->getTrack( allocator );
}


hStringSet well_d2tmodelnames( hWell self, bool checkshot )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p ) return nullptr;
    return p->getD2TModelNames( checkshot );
}


int well_actived2tmodel( hWell self, bool checkshot )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p ) return -1;
    return p->getActiveD2TModelIndex( checkshot );
}


void well_getd2tmodel( hWell self, hAllocator allocator, bool checkshot,
		       const char* name, int index )
{
    auto* p = static_cast<odWell*>(self);
    if ( p )
	p->getD2TModel( allocator, checkshot, name, index );
}


const char* well_getlogs( hWell self, hAllocator allocator,
			  const hStringSet lognms,
			  float zstep, bool upscale )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(lognms);
    if ( !p || !nms )
	return nullptr;

    OD::JSON::Object jsuomobj;
    p->getLogs( allocator, *nms, jsuomobj, zstep,
				upscale ? odWell::Upscale : odWell::Sample );
    return strdup( jsuomobj.dumpJSon().buf() );
}


const char* well_getlog( hWell self, hAllocator allocator, const char* lognm )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !lognm )
	return nullptr;

    OD::JSON::Object jsuomobj;
    p->getLog( allocator, lognm, jsuomobj );
    return strdup( jsuomobj.dumpJSon().buf() );
}

void well_putlog( hWell self, const char* lognm, const float* dah,
		  const float* logdata, uint32_t sz,
		  const char* uom, const char* mnem, bool overwrite )
{
    auto* p = static_cast<odWell*>(self);
    if  ( !p )
	return;

    p->putLog( lognm, dah, logdata, sz, uom, mnem, overwrite );
}


bool well_deletelogs( hWell self, const hStringSet lognms )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(lognms);
    if ( !p || !nms || !p->wd() ) return false;

    if ( nms->isEmpty() )
	return true;

    return Well::MGR().deleteLogs( p->wd()->multiID(), *nms );
}


void well_putmarkers( hWell self, const hStringSet markernms, const float* dah,
		      const hStringSet colors, bool overwrite )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(markernms);
    const auto* cols = static_cast<BufferStringSet*>(colors);
    if ( !p || !nms || !cols || !dah )
	return;

    p->putMarkers( *nms, dah, *cols, overwrite );
}


bool well_deletemarkers( hWell self, const hStringSet markernms )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(markernms);
    if ( !p || !nms || !p->wd() ) return false;

    if ( nms->isEmpty() )
	return true;

    return Well::MGR().deleteMarkers( p->wd()->multiID(), *nms );
}


void well_puttrack( hWell self, const float* dah, const float* tvdss,
		    const double* x, const double* y, uint32_t sz )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !dah || !tvdss || !x || !y )
	return;

    p->putTrack( dah, tvdss, x, y, sz );
}


void well_tvdss( hWell self, const float dah, float* tvdss )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !p->wd() ) return;

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    float dahstored = getConvertedValue( dah, zduom, zsuom );
    float tvdssstored = p->wd()->track().getPos( dahstored ).z_;
    *tvdss = getConvertedValue( tvdssstored, zsuom, zduom );
}


void well_tvd( hWell self, const float dah, float* tvd )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !p->wd() ) return;
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    float dahstored = getConvertedValue( dah, zduom, zsuom );
    float tvdstored = p->wd()->track().getPos( dahstored ).z_ +
						p->wd()->track().getKbElev();
    *tvd = getConvertedValue( tvdstored, zsuom, zduom );
}


hStringSet well_commonlognames( hSurvey surv, const hStringSet nms )
{
    auto* p = static_cast<odSurvey*>(surv);
    auto* fornms = static_cast<BufferStringSet*>(nms);
    if ( !p || !fornms ) return nullptr;
    return odWell::getCommonLogNames( *p, *fornms);
}


hStringSet well_commonmarkernames( hSurvey surv, const hStringSet nms )
{
    auto* p = static_cast<odSurvey*>(surv);
    auto* fornms = static_cast<BufferStringSet*>(nms);
    if ( !p || !fornms ) return nullptr;
    return odWell::getCommonMarkerNames( *p, *fornms);
}

