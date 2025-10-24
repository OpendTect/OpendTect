/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "externalattrib.h"

#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplFactory(ExtAttribCalc,ExtAttribCalc::factory)


ExtAttribCalc::ExtAttribCalc()
{}


ExtAttribCalc::~ExtAttribCalc()
{}


bool ExtAttribCalc::checkSelSpec( const SelSpec& spec ) const
{
    return sCheckSelSpec( spec, factoryKeyword() );
}


bool ExtAttribCalc::sCheckSelSpec( const SelSpec& spec, const char* factkw )
{
    if ( spec.id().asInt() != SelSpec::cOtherAttrib().asInt() )
	return false;

    if ( !factkw || !*factkw )
    {
	pFreeFnErrMsg( "factkw should not be empty" );
    }

    BufferString attribnm;
    const StringView defstr = spec.defString();
    Desc::getAttribName( defstr, attribnm );
    return attribnm == factkw || defstr == factkw;
}


ConstRefMan<RegularSeisDataPack>
ExtAttribCalc::createAttrib( const TrcKeyZSampling&, const RegularSeisDataPack*,
			     TaskRunner* )
{ return nullptr; }


ConstRefMan<RegularSeisDataPack>
ExtAttribCalc::createAttrib( const TrcKeyZSampling&, TaskRunner* )
{ return nullptr; }


bool ExtAttribCalc::createAttrib( ObjectSet<BinIDValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinIDValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }


ConstRefMan<RandomSeisDataPack>
ExtAttribCalc::createRdmTrcAttrib( const ZGate&, const RandomLineID&,
				   TaskRunner* )
{
    return nullptr;
}


ExtAttribCalc* ExtAttribCalc::createInstance( const SelSpec& spec,
					      uiString& errmsg )
{
    BufferString attribname;
    Desc::getAttribName( spec.defString(), attribname );
    PtrMan<ExtAttribCalc> res = factory().create( attribname.buf() );
    if ( !res )
    {
	errmsg = mToUiStringTodo( factory().errMsg() );
	for ( int idx=0; idx<factory().size(); idx++ )
	{
	    const BufferString& factnm = factory().getNames().get(idx);
	    PtrMan<ExtAttribCalc> legacyres = factory().create( factnm.buf() );
	    if ( legacyres && legacyres->checkSelSpec(spec) )
	    {
		res = legacyres.release();
		break;
	    }
	}

	if ( !res )
	    return nullptr;
    }

    if ( !res->checkSelSpec(spec) )
    {
	errmsg = od_static_tr( "ExtAttribCalc::createInstance",
			       "Attribute definition does not match" );
	return nullptr;
    }

    if ( !res->setTargetSelSpec(spec) )
    {
	errmsg = res->errmsg_;
	return nullptr;
    }

    return res.release();
}

} // namespace Attrib
