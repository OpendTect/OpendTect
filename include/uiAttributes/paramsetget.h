#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/


#include "uiattrdesced.h"
#include "attribdesc.h"

namespace Attrib
{

template <class ParamT> inline
ParamT* getTParam( const char* keystr, Desc& desc )
{
    mDynamicCastGet(ParamT*,param,desc.getParam(keystr));
    if ( !param )
	{ pFreeFnErrMsg( "Wrong type" ); }
    return param;
}

template <class ValT,class ParamT> inline
bool setFromParam( ParamT* param, ::ChangeTracker& chtr,
		   const ValT& oldval, const ValT& newval )
{
    if ( param && chtr.set(oldval,newval) )
    {
	param->setValue( newval );
	return true;
    }
    return false;
}

} // namespace Attrib

#define mSetSingleFromValParam( keystr, typ, getfn, newval ) \
{ \
    Attrib::ValParam* param = Attrib::getTParam<Attrib::ValParam>( \
					keystr, desc ); \
    if ( param ) \
	Attrib::setFromParam<typ,Attrib::ValParam>( param, chtr_, \
						    param->getfn(), newval );\
}

#define mSetFloat( keystr, newval ) \
    mSetSingleFromValParam(keystr,float,getFValue,newval)
#define mSetDouble( keystr, newval ) \
    mSetSingleFromValParam(keystr,double,getDValue,newval)
#define mSetInt( keystr, newval ) \
    mSetSingleFromValParam(keystr,int,getIntValue,newval)
#define mSetBinID( keystr, newval ) \
    mSetSingleFromValParam(keystr,BinID,getIdxPairValue<BinID>,newval)
#define mSetString( keystr, newval ) \
    mSetSingleFromValParam(keystr,BufferString,getStringValue,newval)

#define mSetBool( keystr, newval ) \
{ \
    BoolParam* param = Attrib::getTParam<BoolParam>( keystr, desc ); \
    if ( !Attrib::setFromParam<bool,BoolParam>( param, chtr_, \
					param->getBoolValue(), newval ) ) \
	param->setSet(); \
}

#define mSetEnum( keystr, newval ) \
{ \
    Attrib::EnumParam* param = Attrib::getTParam<Attrib::EnumParam>( \
						keystr, desc ); \
    if ( !Attrib::setFromParam<int,Attrib::EnumParam>( param, chtr_, \
				       param->getIntValue(), newval ) ) \
	param->setSet(); \
}

#define mSetFloatInterval( keystr, newval ) \
{ \
    Attrib::FloatGateParam* param = Attrib::getTParam<Attrib::FloatGateParam>( \
						keystr, desc ); \
    if ( param ) \
    { \
	const Interval<float> oldv( param->getFValue(0), param->getFValue(1) );\
	if ( chtr_.set(oldv,newval) ) \
	    param->setValue( newval ); \
    } \
}


// Get macros

#define mIfGetBool( keystr, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(keystr));\
mDynamicCastGet(Attrib::BoolParam*,boolparam##var,valparam##var);\
if ( boolparam##var ) \
{\
    bool var;\
    if ( boolparam##var->isSet() )\
	var = boolparam##var->getBoolValue();\
    else\
	var = boolparam##var->getDefaultBoolValue();\
    setfunc;\
}

#define mIfGetFloat( keystr, var, setfunc ) \
if ( desc.getValParam(keystr) ) \
{\
    float var = desc.getValParam(keystr)->getFValue();\
    if ( mIsUdf(var) )\
	var = desc.getValParam(keystr)->getDefaultFValue();\
    setfunc;\
}

#define mIfGetDouble( keystr, var, setfunc ) \
if ( desc.getValParam(keystr) ) \
{\
    double var = desc.getValParam(keystr)->getDValue();\
    if ( mIsUdf(var) )\
	var = desc.getValParam(keystr)->getDefaultDValue();\
    setfunc;\
}

#define mIfGetInt( keystr, var, setfunc ) \
if ( desc.getValParam(keystr) ) \
{\
    int var = desc.getValParam(keystr)->getIntValue();\
    if ( mIsUdf(var) )\
	var = desc.getValParam(keystr)->getDefaultIntValue();\
    setfunc;\
}

#define mIfGetEnum( keystr, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(keystr));\
mDynamicCastGet(Attrib::EnumParam*,enumparam##var,valparam##var);\
if ( enumparam##var ) \
{\
    int var;\
    if ( enumparam##var->isSet() )\
	var = enumparam##var->getIntValue();\
    else\
	var = enumparam##var->getDefaultIntValue();\
    setfunc;\
}

#define mIfGetString( keystr, var, setfunc ) \
if ( desc.getValParam(keystr) ) \
{ \
    BufferString var = desc.getValParam(keystr)->getStringValue(); \
    if ( var.isEmpty() ) \
	var = desc.getValParam(keystr)->getDefaultStringValue(); \
    setfunc;\
}

#define mIfGetBinID( keystr, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(keystr));\
mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
if ( binidparam##var ) \
{ \
    BinID var; \
    var = binidparam##var->getValue(); \
    if ( mIsUdf(var.inl()) || mIsUdf(var.crl()) )\
	var = binidparam##var->getDefaultBinIDValue();\
    setfunc; \
}

#define mIfGetFloatInterval( keystr, var, setfunc ) \
Attrib::ValParam* valparam##var =\
	const_cast<Attrib::ValParam*>(desc.getValParam(keystr));\
if ( valparam##var ) \
{ \
    Interval<float> var; \
    var.start = valparam##var->getFValue(0); \
    var.stop = valparam##var->getFValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	mDynamicCastGet(Attrib::FloatGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	    var = gateparam##var->getDefaultGateValue();\
    }\
    setfunc; \
}
