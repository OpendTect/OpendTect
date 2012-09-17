#ifndef paramsetget_h
#define paramsetget_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: paramsetget.h,v 1.10 2009/07/22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

/*!\brief used to set parameters*/

#define mSetFloatInterval( str, newval ) \
{ \
    mDynamicCastGet(Attrib::FloatGateParam*,param,desc.getParam(str)) \
    const Interval<float> oldval( param->getfValue(0), param->getfValue(1) ); \
    if ( chtr_.set(oldval,newval) ) param->setValue( newval ); \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const float oldval = param->getfValue(); \
    if ( chtr_.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetInt( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const int oldval = param->getIntValue(); \
    if ( chtr_.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetBool( str, newval ) \
{ \
    mDynamicCastGet(Attrib::BoolParam*,param,desc.getValParam(str)) \
    const bool oldval = param->getBoolValue(); \
    if ( chtr_.set(oldval,newval) ) \
	param->setValue( newval ); \
    else\
    	param->setSet();\
}

#define mSetEnum( str, newval ) \
{ \
    mDynamicCastGet(Attrib::EnumParam*,param,desc.getValParam(str)) \
    const int oldval = param->getIntValue(); \
    if ( chtr_.set(oldval,newval) ) \
	param->setValue( newval ); \
    else\
    	param->setSet();\
}

#define mSetBinID( str, newval ) \
{ \
    mDynamicCastGet(Attrib::BinIDParam*,param,desc.getValParam(str)) \
    const BinID oldval = param->getValue(); \
    if ( chtr_.set(oldval,newval) ) \
    { param->setValue( newval.inl, 0 ); param->setValue( newval.crl, 1 ); } \
}


#define mSetString( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    BufferString oldval = param->getStringValue(); \
    if ( chtr_.set(oldval,newval) ) \
        param->setValue( newval ); \
}


// Get macros

#define mIfGetBool( str, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(str));\
mDynamicCastGet(Attrib::BoolParam*,boolparam##var,valparam##var);\
if ( boolparam##var ) \
{\
   bool var;\
   if ( boolparam##var->isSet() )\
       var = boolparam##var->getBoolValue(0);\
    else\
	var = boolparam##var->getDefaultBoolValue(0);\
    setfunc;\
}

#define mIfGetFloat( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{\
    float var = desc.getValParam(str)->getfValue(0);\
    if ( mIsUdf(var) )\
	var = desc.getValParam(str)->getDefaultfValue(0);\
    setfunc;\
}

#define mIfGetInt( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{\
    int var = desc.getValParam(str)->getIntValue(0);\
    if ( mIsUdf(var) )\
	var = desc.getValParam(str)->getDefaultIntValue(0);\
    setfunc;\
}

#define mIfGetEnum( str, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(str));\
mDynamicCastGet(Attrib::EnumParam*,enumparam##var,valparam##var);\
if ( enumparam##var ) \
{\
    int var;\
    if ( enumparam##var->isSet() )\
	var = enumparam##var->getIntValue(0);\
    else\
    	var = enumparam##var->getDefaultIntValue(0);\
    setfunc;\
}

#define mIfGetString( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ \
    BufferString var = desc.getValParam(str)->getStringValue(0); \
    if ( !strcmp( var, "" ) )\
	var = desc.getValParam(str)->getDefaultStringValue(0); \
    setfunc;\
}

#define mIfGetBinID( str, var, setfunc ) \
Attrib::ValParam* valparam##var = \
	const_cast<Attrib::ValParam*>(desc.getValParam(str));\
mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
if ( binidparam##var ) \
{ \
    BinID var; \
    var = binidparam##var->getValue(); \
    if ( mIsUdf(var.inl) || mIsUdf(var.crl) )\
	var = binidparam##var->getDefaultBinIDValue();\
    setfunc; \
}

#define mIfGetFloatInterval( str, var, setfunc ) \
Attrib::ValParam* valparam##var =\
	const_cast<Attrib::ValParam*>(desc.getValParam(str));\
if ( valparam##var ) \
{ \
    Interval<float> var; \
    var.start = valparam##var->getfValue(0); \
    var.stop = valparam##var->getfValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	mDynamicCastGet(Attrib::FloatGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	    var = gateparam##var->getDefaultGateValue();\
    }\
    setfunc; \
}


#endif
