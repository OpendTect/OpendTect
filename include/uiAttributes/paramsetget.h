#ifndef paramsetget_h
#define paramsetget_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: paramsetget.h,v 1.5 2005-08-29 08:49:35 cvsnanne Exp $
________________________________________________________________________

-*/

/*!\brief used to set parameters*/

#define mSetFloatInterval( str, newval ) \
{ \
    mDynamicCastGet(Attrib::ZGateParam*,param,desc.getParam(str)) \
    const Interval<float> oldval( param->getfValue(0), param->getfValue(1) ); \
    if ( chtr.set(oldval,newval) ) param->setValue( newval ); \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const float oldval = param->getfValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetInt( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const int oldval = param->getIntValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetBool( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const bool oldval = param->getBoolValue(); \
    if ( chtr.set(oldval,newval) ) \
	param->setValue( newval ); \
}

#define mSetEnum( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    const int oldval = param->getIntValue(); \
    if ( chtr.set(oldval,newval) ) \
	param->setValue( newval ); \
}

#define mSetBinID( str, newval ) \
{ \
    mDynamicCastGet(Attrib::BinIDParam*,param,desc.getValParam(str)) \
    const BinID oldval = param->getValue(); \
    if ( chtr.set(oldval,newval) ) \
    { param->setValue( newval.inl, 0 ); param->setValue( newval.crl, 1 ); } \
}


#define mSetString( str, newval ) \
{ \
    Attrib::ValParam* param = desc.getValParam( str ); \
    BufferString oldval = param->getStringValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}


// Get macros

#define mIfGetBool( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ const bool var = desc.getValParam(str)->getBoolValue(0); setfunc; }

#define mIfGetFloat( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ const float var = desc.getValParam(str)->getfValue(0); setfunc; }

#define mIfGetInt( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ const int var = desc.getValParam(str)->getIntValue(0); setfunc; }

#define mIfGetEnum( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ const int var = desc.getValParam(str)->getIntValue(0); setfunc; }

#define mIfGetString( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ const char* var = desc.getValParam(str)->getStringValue(0); \
    setfunc; }

#define mIfGetBinID( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ \
    BinID var; \
    var.inl = desc.getValParam(str)->getIntValue(0); \
    var.crl = desc.getValParam(str)->getIntValue(1); \
    setfunc; \
}

#define mIfGetFloatInterval( str, var, setfunc ) \
if ( desc.getValParam(str) ) \
{ \
    Interval<float> var; \
    var.start = desc.getValParam(str)->getfValue(0); \
    var.stop = desc.getValParam(str)->getfValue(1); \
    setfunc; \
}


#endif
