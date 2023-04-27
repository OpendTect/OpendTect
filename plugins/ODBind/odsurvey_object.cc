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
#include "odsurvey_object.h"

#include "ioman.h"
#include "ioobj.h"
#include "odjson.h"
#include "ranges.h"
//#include "segydirecttr.h"
#include "trckeyzsampling.h"

#include <string.h>
#include <stdexcept>

odSurveyObject::odSurveyObject( const odSurvey& thesurvey, const char* name,
				const char* tgname )
    : survey_(thesurvey)
    , name_(name)
{
    errmsg_.setEmpty();
    survey_.activate();
    ioobj_ = IOM().get( name, tgname );
    if ( !ioobj_ )
	errmsg_ = "IO object read error: ";
    else
	name_ = ioobj_->name();
}


odSurveyObject::odSurveyObject( const odSurvey& thesurvey, const char* name,
				const char* tgname, bool overwrite,
				const char* fmt )
    : survey_(thesurvey)
    , name_(name)
    , overwrite_(overwrite)
{
    errmsg_.setEmpty();
    survey_.activate();
    ioobj_ = survey_.createObj( name, tgname, fmt, overwrite, errmsg_);
    if ( !ioobj_ || !errmsg_.isEmpty() )
	errmsg_.insertAt( 0, "IO object creation error: " );
    else
	name_ = ioobj_->name();
}


odSurveyObject::~odSurveyObject()
{}


odSurveyObject::odSurveyObject( const odSurveyObject& oth )
    : survey_(oth.survey_)
    , name_(oth.name_)
    , overwrite_(oth.overwrite_)
{
    errmsg_.setEmpty();
    survey_.activate();
    ioobj_ = oth.ioobj_->clone();
}


BufferString odSurveyObject::getName() const
{
    return name_;
}

// py::tuple odSurveyObject::tkzsToTuple( const TrcKeyZSampling& tkzs,
// 				       const StepInterval<float>& ztempl )
// {
//     const StepInterval<int> linerg = tkzs.hsamp_.lineRange();
//     py::slice iln( linerg.start, linerg.stop, linerg.step );
//     const StepInterval<int> trcrg = tkzs.hsamp_.trcRange();
//     py::slice xln( trcrg.start, trcrg.stop, trcrg.step );
//     const StepInterval<float> zrg = tkzs.zsamp_;
//     py::slice z( ztempl.getIndex(zrg.start),
// 		 ztempl.getIndex(zrg.stop), 1 );
//
//     return make_tuple( iln, xln, z );
// }
//
//
// TrcKeyZSampling odSurveyObject::tkzsFromTuple( const py::tuple& pos,
// 					       const TrcKeyZSampling& templ )
// {
//     TrcKeyZSampling tkzs( templ );
//
//     const int nslice = pos.size();
//     if ( nslice>=1 )
//     {
// 	py::slice iln = py::cast<py::slice>( pos[0] );
// 	const StepInterval<int> linerg = templ.hsamp_.lineRange();
// 	StepInterval<py::ssize_t> inprg;
// 	PySlice_Unpack( (PYBIND11_SLICE_OBJECT *) iln.ptr(), &inprg.start,
// 			&inprg.stop, &inprg.step );
// 	inprg.limitTo_( linerg );
// 	tkzs.hsamp_.setLineRange( StepInterval<int>(inprg.start, inprg.stop,
// 						    inprg.step) );
//     }
//     if ( nslice>=2 )
//     {
// 	py::slice crl = py::cast<py::slice>( pos[1] );
// 	const StepInterval<int> trcrg = templ.hsamp_.trcRange();
// 	StepInterval<py::ssize_t> inprg;
// 	PySlice_Unpack( (PYBIND11_SLICE_OBJECT *) crl.ptr(), &inprg.start,
// 			&inprg.stop, &inprg.step );
// 	inprg.limitTo_( trcrg );
// 	tkzs.hsamp_.setTrcRange( StepInterval<int>(inprg.start,
// 						   inprg.stop,
// 						   inprg.step) );
//     }
//     if ( nslice>=3 )
//     {
// 	py::slice z = py::cast<py::slice>( pos[2] );
// 	const StepInterval<float> zrg = templ.zsamp_;
// 	StepInterval<py::ssize_t> inprg;
// 	PySlice_Unpack( (PYBIND11_SLICE_OBJECT *) z.ptr(), &inprg.start,
// 			&inprg.stop, &inprg.step );
//	tkzs.zsamp_.set( templ.zAtIndex(inprg.start),
//			templ.zAtIndex(inprg.stop), zrg.step );
// 	tkzs.zsamp_.limitTo( zrg );
//     }
//     tkzs.normalize();
//
//     return tkzs;
// }


void odSurveyObject::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
{
    jsobj.set( "type", "Feature" );
    auto* info = new OD::JSON::Object;
    getInfo( *info );
    jsobj.set( "properties", info );
    auto* geom = new OD::JSON::Object;
    geom->set( "type", "Polygon" );
    auto* rings = new OD::JSON::Array( false ) ;
    auto* coords = new OD::JSON::Array( false );
    getPoints( *coords, towgs );
    rings->add( coords );
    geom->set( "coordinates", rings );
    jsobj.set( "geometry", geom );
}
