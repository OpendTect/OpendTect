#pragma once
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
#include "bufstring.h"
#include "odjson.h"
#include "ptrman.h"
#include "trckeyzsampling.h"

#include "odsurvey.h"

class IOObj;
class SurveyInfo;

class odSurveyObject {
public:
    odSurveyObject(const odSurvey&, const char*, const char*);
    odSurveyObject(const odSurvey&, const char*, const char*, bool,
		   const char* fmt=nullptr);
    virtual ~odSurveyObject();
    odSurveyObject(const odSurveyObject&);

    bool		isOK() const		{ return errmsg_.isEmpty(); }
    const char*		errMsg() const		{ return errmsg_.buf(); }

    const char*		getName() const;
    virtual void	getInfo(OD::JSON::Object&) const = 0;
    virtual void	getFeature(OD::JSON::Object&, bool towgs=true) const;
    virtual void	getPoints(OD::JSON::Array&, bool towgs) const = 0;

    const IOObj&	ioobj() const	{ return *ioobj_; }
    bool		forread() const	{ return forread_; }
    const odSurvey&	survey() const	{ return survey_; }

    template<typename T>
    static bool		isPresent(const odSurvey&, const char* objname);
    template<typename T>
    static BufferStringSet*	getNames(const odSurvey& survey);
    template<typename T>
    static void		getInfos(OD::JSON::Array&, const odSurvey& survey,
				 const BufferStringSet&);
    template<typename T>
    static void		getFeatures(OD::JSON::Object&, const odSurvey& survey,
				    const BufferStringSet&);

    static const char*	sKeyTranslatorGrp()	{ return "Survey Object"; }

    // static py::tuple	tkzsToTuple(const TrcKeyZSampling&,
				//     const StepInterval<float>&);
    // static TrcKeyZSampling	tkzsFromTuple(const py::tuple&,
				// 	      const TrcKeyZSampling&);

protected:
    const odSurvey&	survey_;
    BufferString	name_;
    PtrMan<IOObj>	ioobj_;
    bool		forread_;
    bool		overwrite_ = false;
    BufferString	errmsg_;

};


template<typename T>
bool odSurveyObject::isPresent( const odSurvey& survey, const char* objname )
{
    return survey.isObjPresent( objname, T::sKeyTranslatorGrp() );
}


template<typename T>
BufferStringSet* odSurveyObject::getNames( const odSurvey& survey )
{
    return survey.getObjNames( T::sKeyTranslatorGrp() );
}


template<typename T>
void odSurveyObject::getInfos( OD::JSON::Array& jsarr,  const odSurvey& survey,
			       const BufferStringSet& fornames )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames<T>( survey );
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    jsarr.setEmpty();
    for ( const auto* nm : nms )
    {
	T obj( survey, *nm );
	OD::JSON::Object info;
	obj.getInfo( info );
	if ( info.isEmpty() )
	    continue;

	jsarr.add( info.clone() );
    }
}


template<typename T>
void odSurveyObject::getFeatures( OD::JSON::Object& jsobj,
				  const odSurvey& survey,
				  const BufferStringSet& fornames  )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames<T>( survey );
    if ( fornames.isEmpty() )
 	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    jsobj.setEmpty();
    auto* features = new OD::JSON::Array( true );
    for ( const auto* nm : nms )
    {
        T obj( survey,  *nm );
	OD::JSON::Object info;
	obj.getFeature( info );
 	if ( info.isEmpty() )
 	    continue;

 	features->add( info.clone() );
     }

    jsobj.set( "type", "FeatureCollection" );
    jsobj.set( "features", features );
}

