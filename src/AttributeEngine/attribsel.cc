/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: attribsel.cc,v 1.4 2005-08-01 07:32:59 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "ctxtioobj.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "bufstringset.h"
#include "keystrs.h"
#include "linekey.h"
#include "globexpr.h"

namespace Attrib
{

const DescID SelSpec::otherAttrib = DescID(-1,true);
const DescID SelSpec::noAttrib = DescID(-2,true);
const DescID SelSpec::attribNotSel = DescID(-3,true);

const char* SelSpec::refstr = "Attrib Reference";
const char* SelSpec::idstr = "Attrib ID";
const char* SelSpec::isnlastr = "Is attrib NLA Model";
const char* SelSpec::objrefstr = "Object Reference";
static const char* isnnstr = "Is attrib NN"; // for backward compatibility

bool SelSpec::operator==( const SelSpec& ss ) const
{
    return id()==ss.id() && isNLA()==ss.isNLA() && 
	   ss.ref_==ref_ && ss.objref_==objref_;
}


bool SelSpec::operator!=( const SelSpec& ss ) const
{
    return !(*this==ss);
}


void SelSpec::set( const Desc& desc )
{
    isnla_ = false;
    id_ = desc.id();
    ref_ = desc.userRef();
    setDiscr( *desc.descSet() );
}


void SelSpec::set( const NLAModel& nlamod, int nr )
{
    isnla_ = true;
    id_ = DescID(nr,true);
    setRefFromID( nlamod );
}


void SelSpec::setDiscr( const NLAModel& nlamod )
{
    if ( !nlamod.design().doclass ) 
	discrspec_ = StepInterval<int>(0,0,0);
    else
    {
	discrspec_.start = discrspec_.step = 1;
	discrspec_.stop = nlamod.design().hiddensz;
    }
}


void SelSpec::setDiscr( const DescSet& ds )
{
    discrspec_ = StepInterval<int>(0,0,0);
}


void SelSpec::setIDFromRef( const NLAModel& nlamod )
{
    isnla_ = true;
    const int nrout = nlamod.design().outputs.size();
    id_ = noAttrib;
    for ( int idx=0; idx<nrout; idx++ )
    {
	if ( ref_ == *nlamod.design().outputs[idx] )
	    { id_ = DescID(idx,true); break; }
    }
    setDiscr( nlamod );
}


void SelSpec::setIDFromRef( const DescSet& ds )
{
    isnla_ = false;
    id_ = ds.getID( ref_, true );
    setDiscr( ds );
}


void SelSpec::setRefFromID( const NLAModel& nlamod )
{
    ref_ = id_.asInt() >= 0 ? nlamod.design().outputs[id_.asInt()]->buf() : "";
    setDiscr( nlamod );
}


void SelSpec::setRefFromID( const DescSet& ds )
{
    const Desc* desc = ds.getDesc( id_ );
    ref_ = desc ?  desc->userRef() : "";
    setDiscr( ds );
}


void SelSpec::fillPar( IOPar& par ) const
{
    par.set( refstr, ref_ );
    par.set( idstr, id_.asInt() );
    par.setYN( isnlastr, isnla_ );
    par.set( objrefstr, objref_ );
}


bool SelSpec::usePar( const IOPar& par )
{
    ref_ = ""; 		par.get( refstr, ref_ );
    id_ = noAttrib;	par.get( idstr, id_.asInt() );
    isnla_ = false; 	par.getYN( isnlastr, isnla_ );
    			par.getYN( isnnstr, isnla_ );
    objref_ = "";	par.get( objrefstr, objref_ );
    		
    return true;
}



SelInfo::SelInfo( const DescSet* attrset, const NLAModel* nlamod, 
		  Pol2D pol, const DescID& ignoreid )
{
    pol2d_ = pol;
    fillStored();

    if ( attrset )
    {
	for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	{
	    const DescID descid = attrset->getID( idx );
	    const Desc* desc = attrset->getDesc( descid );
	    if ( !desc || 
		 !strcmp(desc->attribName(),StorageProvider::attribName()) || 
		 attrset->getID(*desc) == ignoreid || desc->isHidden() )
		continue;

	    attrids += descid;
	    attrnms.add( desc->userRef() );
	}
    }

    if ( nlamod )
    {
	const int nroutputs = nlamod->design().outputs.size();
	for ( int idx=0; idx<nroutputs; idx++ )
	{
	    BufferString nm( *nlamod->design().outputs[idx] );
	    if ( IOObj::isKey(nm) )
		nm = IOM().nameOf( nm, false );
	    nlaoutnms.add( nm );
	}
    }
}


void SelInfo::fillStored( const char* filter )
{
    ioobjnms.deepErase(); ioobjids.deepErase();

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    GlobExpr* ge = filter && *filter ? new GlobExpr( filter ) : 0;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	const bool is2d = !strcmp(ioobj.translator(),"2D");
	const bool iscbvs = !strcmp(ioobj.translator(),"CBVS");
	if ( (is2d && pol2d_==No2D) || (iscbvs && pol2d_==Only2D) ||
	     (!is2d && !iscbvs) ) continue;

	const char* psstring = "Pre-Stack Seismics";
	if ( !strcmp(ioobj.group(),psstring) ) continue;

	const char* res = ioobj.pars().find( "Type" );
	if ( res && !strcmp(res,sKey::Steering) ) continue;

	const char* ioobjnm = ioobj.name().buf();
	if ( ge && !ge->matches(ioobjnm) )
	    continue;

	ioobjnms.add( ioobjnm );
	ioobjids.add( (const char*)ioobj.key() );
	if ( ioobjnms.size() > 1 )
	{
	    for ( int icmp=ioobjnms.size()-2; icmp>=0; icmp-- )
	    {
		if ( ioobjnms.get(icmp) > ioobjnms.get(icmp+1) )
		{
		    ioobjnms.swap( icmp, icmp+1 );
		    ioobjids.swap( icmp, icmp+1 );
		}
	    }
	}
    }
}


SelInfo::SelInfo( const SelInfo& asi )
	: ioobjnms(asi.ioobjnms)
	, ioobjids(asi.ioobjids)
	, attrnms(asi.attrnms)
	, attrids(asi.attrids)
	, nlaoutnms(asi.nlaoutnms)
	, pol2d_(asi.pol2d_)
{
}


SelInfo& SelInfo::operator=( const SelInfo& asi )
{
    ioobjnms = asi.ioobjnms;
    ioobjids = asi.ioobjids;
    attrnms = asi.attrnms;
    attrids = asi.attrids;
    nlaoutnms = asi.nlaoutnms;
    pol2d_ = asi.pol2d_;
    return *this;
}


bool SelInfo::is2D( const char* defstr )
{
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    return ioobj ? SeisTrcTranslator::is2D(*ioobj,true) : false;
}


void SelInfo::getAttrNames( const char* defstr, BufferStringSet& nms )
{
    nms.erase();
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    if ( !ioobj ) return;

    Seis2DLineSet ls( ioobj->fullUserExpr(true) );
    ls.getAvailableAttributes( nms );
    int idx = nms.indexOf( sKey::Steering );
    if ( idx >= 0 )
	nms.remove( idx );
}


const char* ColorSelSpec::refstr = "Color-Attrib Reference";
const char* ColorSelSpec::idstr = "Color-Attrib ID";
const char* ColorSelSpec::isnlastr = "Is color-Attrib NLA Model";
const char* ColorSelSpec::datatypestr = "Color-Attrib datatype";

void ColorSelSpec::fillPar( IOPar& par ) const
{
    par.set( refstr, as.userRef() );
    par.set( idstr, as.id().asInt() );
    par.setYN( isnlastr, as.isNLA() );
    par.set( datatypestr, datatype );
}


bool ColorSelSpec::usePar( const IOPar& par )
{
    BufferString ref; DescID id(-1,true); bool isnla;
    if ( !par.get(refstr,ref) ) return false;
    if ( !par.get(idstr,id.asInt()) ) return false;
    if ( !par.getYN(isnlastr,isnla) ) return false;
    as.set( ref.buf(), id, isnla, "" );
    datatype = 0;
    par.get( datatypestr, datatype );

    return true;
}

}; // namespace Attrib
