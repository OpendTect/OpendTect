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
#include <string.h>


class IOObj;
class SurveyInfo;

class odSurveyObject {
public:
    odSurveyObject(const odSurvey&, const char*, const char*);
    odSurveyObject(const odSurvey&, const char*, const char*, bool,
		   const char* fmt=nullptr);
    virtual ~odSurveyObject();

    odSurveyObject(const odSurveyObject&) = delete;
    odSurveyObject& operator= (const odSurveyObject&) = delete;

    bool		isOK() const	{ return errmsg_.isEmpty(); }
    BufferString	errMsg() const	{ return errmsg_; }
    void		setErrMsg(const char* msg) const { errmsg_ = msg; }

    BufferString	getName() const;
    virtual void	getInfo(OD::JSON::Object&) const = 0;
    virtual void	getFeature(OD::JSON::Object&, bool towgs=true) const;
    virtual void	getPoints(OD::JSON::Array&, bool towgs) const = 0;

    const IOObj&	ioobj() const	{ return *ioobj_; }
    const odSurvey&	survey() const	{ return survey_; }
    bool		isReadOnly() const 	{ return readonly_; }
    bool		canRead() const;
    bool		canWrite() const;
    bool		zIsTime() const		{ return zistime_; }

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
    template<typename T>
    static void		removeObjects(const odSurvey& survey,
				      const BufferStringSet&);

protected:
    const odSurvey&		survey_;
    BufferString		name_;
    PtrMan<IOObj>		ioobj_;
    bool			overwrite_ = false;
    bool			readonly_ = true;
    bool			zistime_;
    mutable BufferString	errmsg_;

};


template<typename T>
bool odSurveyObject::isPresent( const odSurvey& survey, const char* objname )
{
    return survey.isObjPresent( objname, T::translatorGrp() );
}


template<typename T>
BufferStringSet* odSurveyObject::getNames( const odSurvey& survey )
{
    return survey.getObjNames( T::translatorGrp() );
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


template<typename T>
void odSurveyObject::removeObjects( const odSurvey& survey,
				    const BufferStringSet& objnames  )
{
    for ( const auto* name : objnames )
	survey.removeObj( *name, T::translatorGrp() );
}


//
// Use mDeclareBaseBindings(Horizon3D, horizon3d) in the .h file
//
// should produce
//	mExternC(ODBind) hHorizon3D horizon3d_newin(hSurvey, const char* name);
//	mExternC(ODBind) void horizon3d_del(hHorizon3D);
//	etc
#define mDeclareBaseBindings(classnm, bindnm) \
    typedef void* h##classnm; \
    mExternC(ODBind) h##classnm bindnm##_newin(hSurvey, const char* name); \
    mExternC(ODBind) void bindnm##_del(h##classnm); \
    mExternC(ODBind) const char* bindnm##_errmsg(h##classnm); \
    mExternC(ODBind) const char* bindnm##_feature(h##classnm); \
    mExternC(ODBind) const char* bindnm##_features(hSurvey, const hStringSet); \
    mExternC(ODBind) const char* bindnm##_info(h##classnm); \
    mExternC(ODBind) const char* bindnm##_infos(hSurvey, const hStringSet); \
    mExternC(ODBind) bool bindnm##_isok(h##classnm); \
    mExternC(ODBind) hStringSet bindnm##_names(hSurvey); \
    mExternC(ODBind) void bindnm##_removeobjs(hSurvey, const hStringSet); \
    mExternC(ODBind) bool bindnm##_zistime(h##classnm);

//
// Use mDefineBaseBindings(Horizon3D, horizon3d) in the .cc file
//
#define mDefineBaseBindings(classnm, bindnm) \
    h##classnm bindnm##_newin( hSurvey survey, const char* name ) \
    { \
	const auto* surv = reinterpret_cast<odSurvey*>(survey); \
	return surv && name ? new od##classnm( *surv, name ) : nullptr; \
    } \
    void bindnm##_del( h##classnm self ) \
    { \
	auto* p = reinterpret_cast<od##classnm*>(self); \
	if ( p ) delete p; \
    } \
    const char* bindnm##_errmsg( h##classnm self ) \
    { \
	const auto* p = reinterpret_cast<od##classnm*>(self); \
	return p ? strdup( p->errMsg().buf() ) : nullptr; \
    } \
    const char* bindnm##_feature( h##classnm self ) \
    { \
	const auto* p = reinterpret_cast<od##classnm*>(self); \
	OD::JSON::Object jsobj; \
	if ( !p || !p->canRead() ) return nullptr; \
	p->getFeature( jsobj ); \
	return strdup( jsobj.dumpJSon().buf() ); \
    } \
    const char* bindnm##_features( hSurvey survey, const hStringSet fornms ) \
    { \
	const auto* surv = reinterpret_cast<odSurvey*>(survey); \
	const auto* nms = reinterpret_cast<BufferStringSet*>(fornms); \
	if ( !surv || !nms ) return nullptr; \
	OD::JSON::Object jsobj; \
	od##classnm::getFeatures<od##classnm>( jsobj, *surv, *nms ); \
	return strdup( jsobj.dumpJSon().buf() ); \
    } \
    const char* bindnm##_info( h##classnm self ) \
    { \
	const auto* p = reinterpret_cast<od##classnm*>(self); \
	if ( !p ) return nullptr; \
	OD::JSON::Object jsobj; \
	p->getInfo( jsobj ); \
	return strdup( jsobj.dumpJSon().buf() ); \
    } \
    const char* bindnm##_infos( hSurvey survey, const hStringSet fornms ) \
    { \
	const auto* surv = reinterpret_cast<odSurvey*>(survey); \
	const auto* nms = reinterpret_cast<BufferStringSet*>(fornms); \
	if ( !surv || !nms ) return nullptr; \
	OD::JSON::Array jsarr( true ); \
	od##classnm::getInfos<od##classnm>( jsarr, *surv, *nms ); \
	return strdup( jsarr.dumpJSon().buf() ); \
    } \
    bool bindnm##_isok( h##classnm self ) \
    { \
	const auto* p = reinterpret_cast<od##classnm*>(self); \
	return p ? p->isOK() : false; \
    } \
    bool bindnm##_zistime( h##classnm self ) \
    { \
	const auto* p = reinterpret_cast<od##classnm*>(self); \
	return p ? p->zIsTime() : false; \
    } \
    hStringSet bindnm##_names( hSurvey survey ) \
    { \
	const auto* p = reinterpret_cast<odSurvey*>(survey); \
	if ( !p ) return nullptr; \
	return od##classnm::getNames<od##classnm>( *p ); \
    } \
    void bindnm##_removeobjs( hSurvey survey, const hStringSet objnms ) \
    { \
	const auto* p = reinterpret_cast<odSurvey*>(survey); \
	const auto* nms = reinterpret_cast<BufferStringSet*>(objnms); \
	if ( !p || !nms ) return; \
	od##classnm::removeObjects<od##classnm>( *p, *nms ); \
    }

