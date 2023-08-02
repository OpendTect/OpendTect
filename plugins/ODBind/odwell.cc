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
    along with this program. If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/


#include "odsurvey.h"
#include "odwell.h"

#include "ioobj.h"
#include "latlong.h"
#include "position.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"


odWell::odWell( const odSurvey& thesurvey, const char* name )
    : odSurveyObject(thesurvey, name, translatorGrp())
{
    if ( !ioobj_ )
	return;

    wd_ = Well::MGR().get( ioobj_->key(), Well::LoadReqs::AllNoLogs() );
}


const Well::Data* odWell::wd() const
{
    survey_.activate();
    return wd_.ptr();
}


BufferStringSet* odWell::getLogNames() const
{
    survey_.activate();
    if ( !wd_ )
	return nullptr;

    auto* names = new BufferStringSet;
    const Well::LogSet& ls = wd_->logs();
    for ( int il=0; il<ls.size(); il++ )
    {
	const Well::Log& log = ls.getLog( il );
	names->add( log.name() );
    }
    return names;
}


void odWell::getLogInfo( OD::JSON::Array& jsarr,
			 const BufferStringSet& fornames ) const
{
    survey_.activate();
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getLogNames();
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    jsarr.setEmpty();
    if ( !wd_ )
	return;

    const Well::LogSet& ls = wd_->logs();
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    for ( const auto* nm : nms )
    {
	const auto* log = ls.getLog( nm->buf() );
	if ( !log )
	    continue;

	const auto dahstored = log->dahRange();
	const auto dahrange = Interval<float>(
			    getConvertedValue(dahstored.start, zsuom, zduom),
			    getConvertedValue(dahstored.stop, zsuom, zduom) );
	OD::JSON::Object loginfo;
	loginfo.set( "name", log->name() );
	loginfo.set( "mnemonic", log->mnemonicLabel() );
	loginfo.set( "uom", log->unitMeasLabel() );
	loginfo.set( "dah_range", dahrange );
	loginfo.set( "log_range", log->valueRange() );
	jsarr.add( loginfo.clone() );
    }
}


BufferStringSet* odWell::getMarkerNames() const
{
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
    survey_.activate();
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getMarkerNames();
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    jsarr.setEmpty();
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
	*tvdss_data++ = getConvertedValue( pos.z, zsuom, zduom );
	*x_data++ = pos.x;
	*y_data++ = pos.y;
    }
}


void odWell::getLogs( hAllocator allocator, const BufferStringSet& lognms,
		      OD::JSON::Array& jsarr, float zstep,
		      SampleMode samplemode )
{
    survey_.activate();
    jsarr.setEmpty();
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

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    StepInterval<float> dahrg;
    dahrg.setUdf();
    dahrg.step = getConvertedValue( zstep, zduom, zsuom );
    for ( const auto* lognm : nms )
    {
	auto* log = wd_->logs().getLog( lognm->buf() );
	if (log)
	    dahrg.include(log->dahRange());
    }

    const int ndim = 1;
    int dims[ndim];
    bool first = true;
    for ( const auto* lognm : nms )
    {
	const auto* log = wd_->getLog( lognm->buf() );
	if ( log )
	{
	    PtrMan<Well::Log> outlog;
	    if (samplemode==Upscale)
		outlog = log->upScaleLog( dahrg );
	    else
		outlog = log->sampleLog( dahrg );

	    dims[0] = outlog->size();
	    if ( first )
	    {
		float* dah_data = static_cast<float*>(
						allocator(ndim, dims, 'f') );
		for ( int idx=0; idx<outlog->size(); idx++ )
		    *dah_data++ = getConvertedValue( outlog->dah(idx), zsuom,
						     zduom );

		first = false;
		OD::JSON::Object loginfo;
		loginfo.set( "dah",
			     UnitOfMeasure::surveyDefDepthUnit()->getLabel() );
		jsarr.add( loginfo.clone() );
	    }

	    float* log_data = static_cast<float*>(
						allocator(ndim, dims, 'f') );
	    for ( int idx=0; idx<outlog->size();idx++ )
		*log_data++ = mIsUdf(outlog->value(idx)) ? nanf("") :
							    outlog->value(idx);

	    OD::JSON::Object loginfo;
	    loginfo.set( lognm->buf(), log->unitMeasLabel() );
	    jsarr.add( loginfo.clone() );
	}
    }
}


void odWell::putLog( const char* lognm, const float* dah, const float* logdata,
		     uint32_t sz, const char* uom, const char* mnem,
		     bool overwrite )
{
    survey_.activate();
    if ( !wd_ )
    {
	errmsg_ = "odWell::putLog - invalid welldata object.";
	return;
    }

    if ( !overwrite && wd_->logs().isPresent(lognm) )
	return;
    else if ( wd_->logs().isPresent(lognm) )
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
#ifdef __win__
	if ( isnan(logval) )
#else
	if ( std::isnan(logval) )
#endif
	    logval = mUdf(float);

	outlog->addValue( dep, logval );
    }
    outlog->updateAfterValueChanges();

    if ( !Well::MGR().writeAndRegister(wd_->multiID(), outlog) )
	errmsg_ = "odWell::putLog - saving log failed";
}


void odWell::getInfo( OD::JSON::Object& jsobj ) const
{
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
    jsobj.set( "x", cd.x );
    jsobj.set( "y", cd.y );
    jsobj.set( "kb", wd_->track().getKbElev() );
    jsobj.set( "td", wd_->track().td() );
    jsobj.set( "replacement_velocity", wd_->info().replvel_ );
    jsobj.set( "ground_elevation", wd_->info().groundelev_ );
}


void odWell::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
{
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
    survey_.activate();
    if ( !wd_ )
	return;

    auto coord = wd_->info().surfacecoord_;
    if ( towgs )
    {
	ConstRefMan<Coords::CoordSystem> coordsys =
						survey_.si().getCoordSystem();
	const LatLong ll( LatLong::transform(coord, true, coordsys) );
	jsarr.add( strdup(toString(ll.lng_, 6)) );
	jsarr.add( strdup(toString(ll.lat_, 6)) );
    }
    else
    {
	jsarr.add( coord.x );
	jsarr.add( coord.y );
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


const char* well_getlogs( hWell self, hAllocator allocator,
			  const hStringSet lognms,
			  float zstep, bool upscale )
{
    auto* p = static_cast<odWell*>(self);
    const auto* nms = static_cast<BufferStringSet*>(lognms);
    if ( !p || !nms )
	return nullptr;

    OD::JSON::Array jsarr( true );
    p->getLogs( allocator, *nms, jsarr, zstep,
				upscale ? odWell::Upscale : odWell::Sample );
    return strdup( jsarr.dumpJSon().buf() );
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


void well_tvdss( hWell self, const float dah, float* tvdss )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !p->wd() ) return;

    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    float dahstored = getConvertedValue( dah, zduom, zsuom );
    float tvdssstored = p->wd()->track().getPos( dahstored ).z;
    *tvdss = getConvertedValue( tvdssstored, zsuom, zduom );
}


void well_tvd( hWell self, const float dah, float* tvd )
{
    auto* p = static_cast<odWell*>(self);
    if ( !p || !p->wd() ) return;
    const UnitOfMeasure* zduom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    float dahstored = getConvertedValue( dah, zduom, zsuom );
    float tvdstored = p->wd()->track().getPos( dahstored ).z +
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

