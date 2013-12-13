/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";



#include "uiwelllogattrib.h"
#include "welllogattrib.h"

#include "attribdesc.h"
#include "attribparam.h"

#include "uiattribfactory.h"
#include "uimultiwelllogsel.h"

using namespace Attrib;


mInitAttribUI(uiWellLogAttrib,WellLog,"WellLog",sKeyBasicGrp())


uiWellLogAttrib::uiWellLogAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.3")

{
    uiMultiWellLogSel::Setup su; su.singlelog(true).withextractintime(false);

    wellfld_ = new uiMultiWellLogSel( this, su );
    setHAlignObj( wellfld_ );
}


bool uiWellLogAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    const ValParam* par = desc.getValParam( WellLog::logName() );
    if ( par )
    {
	BufferStringSet lognms;
	lognms.add( par->getStringValue(0) );
	wellfld_->setSelLogNames( lognms );
    }

    par = desc.getValParam( WellLog::keyStr() );
    if ( par )
    {
	const FileMultiString fms = par->getStringValue( 0 );
	BufferStringSet wellids;
	for ( int idx=0; idx<fms.size(); idx++ )
	    wellids.add( fms[idx] );
	wellfld_->setSelWellIDs( wellids );
    }

    return true;
}


bool uiWellLogAttrib::setInput( const Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::setOutput( const Attrib::Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    BufferStringSet selwellids;
    wellfld_->getSelWellIDs( selwellids );
    FileMultiString fms; fms.add( selwellids );
    mSetString( WellLog::keyStr(), fms.buf() );

    BufferStringSet sellognms;
    wellfld_->getSelLogNames( sellognms );
    mSetString( WellLog::logName(), sellognms.get(0) );

    return true;
}


bool uiWellLogAttrib::getInput( Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::getOutput( Attrib::Desc& desc )
{
    return true;
}


void uiWellLogAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
}
