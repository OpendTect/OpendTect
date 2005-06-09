#ifndef paramsetget_h
#define paramsetget_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: paramsetget.h,v 1.2 2005-06-09 13:12:35 cvsnanne Exp $
________________________________________________________________________

-*/

/*!\brief used to set parameters*/

#define mSetFloatInterval( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const Interval<float> oldval( param->getfValue(0), param->getfValue(1) ); \
    if ( chtr.set(oldval,newval) ) \
    { param->setValue( newval.start, 0 ); param->setValue( newval.stop, 1 ); } \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const float oldval = param->getfValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetInt( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const int oldval = param->getIntValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}

#define mSetBool( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const bool oldval = param->getBoolValue(); \
    if ( chtr.set(oldval,newval) ) \
	param->setValue( newval ); \
}

#define mSetEnum( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const int oldval = param->getIntValue(); \
    if ( chtr.set(oldval,newval) ) \
	param->setValue( newval ); \
}

#define mSetBinID( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    const BinID oldval( param->getIntValue(0), param->getIntValue(1) ); \
    if ( chtr.set(oldval,newval) ) \
    { param->setValue( newval.inl, 0 ); param->setValue( newval.crl, 1 ); } \
}

#define mSetString( str, newval ) \
{ \
    Attrib::Param* param = desc.getParam( str ); \
    BufferString oldval = param->getStringValue(); \
    if ( chtr.set(oldval,newval) ) \
        param->setValue( newval ); \
}


// Get macros

#define mIfGetBool( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ const bool var = desc.getParam(str)->getBoolValue(0); setfunc; }

#define mIfGetFloat( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ const float var = desc.getParam(str)->getfValue(0); setfunc; }

#define mIfGetInt( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ const int var = desc.getParam(str)->getIntValue(0); setfunc; }

#define mIfGetEnum( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ const int var = desc.getParam(str)->getIntValue(0); setfunc; }

#define mIfGetString( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ const char* var = desc.getParam(str)->getStringValue(0); setfunc; }

#define mIfGetBinID( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ \
    BinID var; \
    var.inl = desc.getParam(str)->getIntValue(0); \
    var.crl = desc.getParam(str)->getIntValue(1); \
    setfunc; \
}

#define mIfGetFloatInterval( str, var, setfunc ) \
if ( desc.getParam(str) ) \
{ \
    Interval<float> var; \
    var.start = desc.getParam(str)->getfValue(0); \
    var.stop = desc.getParam(str)->getfValue(1); \
    setfunc; \
}


#endif
