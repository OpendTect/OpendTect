/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribdescset.cc,v 1.9 2005-06-09 13:36:42 cvshelene Exp $";

#include "attribdescset.h"
#include "attribstorprovider.h"
#include "attribparam.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "bufstringset.h"
#include "iopar.h"
#include "separstr.h"
#include "gendefs.h"

namespace Attrib
{

DescSet* DescSet::clone() const
{
    DescSet* descset = new DescSet();
    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	Desc* nd = descs[idx]->clone();
	nd->setDescSet( descset );
	descset->descs += nd;
	descset->ids += ids[idx];
    }

    return descset;
}


int DescSet::addDesc( Desc* nd )
{
    nd->setDescSet(this);
    nd->ref();
    descs += nd;
    const int id = getFreeID();
    ids += id;
    return id;
}


Desc* DescSet::getDesc(int id)
{
    const int idx = ids.indexOf(id);
    if ( idx==-1 ) return 0;
    return descs[idx];
}


const Desc* DescSet::getDesc(int id) const
{
    return const_cast<DescSet*>(this)->getDesc(id);
}


int DescSet::nrDescs() const { return descs.size(); }


int DescSet::getID( const Desc& desc ) const
{
    const int idx = descs.indexOf( &desc );
    return idx==-1 ? -1 : ids[idx];
}


int DescSet::getID( int descnr ) const
{
    if ( descnr < 0 || descnr >= ids.size() ) return -2;
    return ids[descnr];
}


void DescSet::getIds( TypeSet<int>& attribids ) const
{
    attribids = ids;
}


int DescSet::getID( const char* ref, bool isusrref ) const
{
    if ( !ref || !*ref ) return -1;

    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	if ( isusrref && !strcmp(ref,descs[idx]->userRef()) )
	    return ids[idx];
	else if ( !isusrref )
	{
	    BufferString defstr;
	    descs[idx]->getDefStr( defstr );
	    if ( defstr == ref )
		return ids[idx];
	}
    }

    return -1;
}


void DescSet::removeDesc( int id )
{
    const int idx = ids.indexOf(id);
    if ( idx==-1 ) return;

    if ( descs[idx]->descSet()==this )
	descs[idx]->setDescSet(0);

    descs[idx]->unRef();
    descs.remove(idx);
    ids.remove(idx);
}


void DescSet::removeAll()
{ while ( ids.size() ) removeDesc(ids[0]); }


void DescSet::fillPar( IOPar& par ) const
{
    int maxid = 0;

    for ( int idx=0; idx<descs.size(); idx++ )
    {
	IOPar apar;
	BufferString defstr;
	if ( !descs[idx]->getDefStr(defstr) ) continue;
	apar.set( definitionStr(), defstr );

	if ( descs[idx]->userRef() )
	    apar.set( userRefStr(), descs[idx]->userRef() );

	for ( int input=0; input<descs[idx]->nrInputs(); input++ )
	{
	    if ( !descs[idx]->getInput(input) ) continue;

	    const char* key = IOPar::compKey( inputPrefixStr(), input );
	    apar.set( key, getID( *descs[idx]->getInput(input) ) );
	}

	BufferString subkey = ids[idx];
	par.mergeComp( apar, subkey );

	if ( ids[idx]>maxid ) maxid = ids[idx];
    }

    par.set( highestIDStr(), maxid );
}


#define mHandleParseErr( str ) \
{ \
    errmsg = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg); \
    continue; \
}

bool DescSet::usePar( const IOPar& par, BufferStringSet* errmsgs )
{
    removeAll();
    if ( errmsgs ) deepErase( *errmsgs );

    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    ObjectSet<Desc> newsteeringdescs;
    IOPar copypar(par);

    for ( int id=0; id<=maxid; id++ )
    {
	const BufferString idstr( id );
	PtrMan<IOPar> descpar = par.subselect(idstr);
	if ( !descpar ) continue;

	//Look for type (old format)
	const char* typestr = descpar->find("Type");
	if ( typestr && !strcmp(typestr,"Stored" ) )
	{
	    const char* olddef = descpar->find(definitionStr());
	    if ( !olddef ) continue;
	    BufferString newdef = StorageProvider::attribName();
	    newdef += " ";
	    newdef += Attrib::StorageProvider::keyStr();
	    newdef += "=";
	    newdef +=olddef;
	    descpar->set(definitionStr(),newdef);
	}

	const IOPar* steeringpar = descpar->subselect("Steering");
	if ( steeringpar )
	{
	    const char* defstring = descpar->find(definitionStr());
	    if ( !defstring )
		mHandleParseErr("No attribute definition string specified");
	    if ( !createSteeringDesc(*steeringpar, defstring, newsteeringdescs))
	    {
	        BufferString err = "Cannot create steering desc ";
	        mHandleParseErr(err);
	    }
	}

	const char* defstring = descpar->find(definitionStr());
	if ( !defstring )
	    mHandleParseErr("No attribute definition string specified");

	BufferString attribname;
	if ( !Desc::getAttribName( defstring, attribname ) )
	    mHandleParseErr("Cannot find attribute name");
	

	RefMan<Desc> desc;
	desc = PF().createDescCopy(attribname);

	if ( !desc )
	{
	    BufferString err = "Cannot find factory-entry for ";
	    err += attribname;
	    mHandleParseErr(err);
	}

	if ( !desc->parseDefStr(defstring) )
	{
	    BufferString err = "Cannot parse: ";
	    err += defstring;
	    mHandleParseErr(err);
	}

	const char* userref = descpar->find(userRefStr());
	if ( userref ) desc->setUserRef(userref);

	int seloutpid;
	descpar->get( "Selected Attrib",seloutpid );
	desc->selectOutput(seloutpid);

	if ( steeringpar )
	{
	    for ( int idx=0; idx<desc->nrInputs(); idx++ )
	    {
		BufferString inputstr = IOPar::compKey("Input", idx);
		if ( !strcmp (descpar->find(inputstr),"-1") )
		{
		    BufferString newinput = id;
		    newinput = IOPar::compKey(newinput, inputstr);
		    copypar.set( newinput, maxid + newsteeringdescs.size() );
		}
	    }
	}

	addDesc(desc);
    }
    
    for( int idx=0 ; idx<newsteeringdescs.size() ; idx++ )
	addDesc( newsteeringdescs[idx] );

    for ( int idx=0; idx<descs.size(); idx++ )
    {
	if ( idx <= maxid )
	{
	    const BufferString idstr( ids[idx] );
	    PtrMan<IOPar> descpar = copypar.subselect(idstr);

	    for ( int input=0; input<descs[idx]->nrInputs(); input++ )
	    {
		const char* key = IOPar::compKey( inputPrefixStr(), input );

		int inpid;
		if ( !descpar->get(key,inpid) ) continue;

		Desc* inpdesc = getDesc( inpid );
		if ( !inpdesc ) continue;

		descs[idx]->setInput( input, inpdesc );
	    }
	}
	if ( descs[idx]->isSatisfied()!=0 )
	{
	    BufferString err = "inputs or parameters are not satisfied for ";
	    err += descs[idx]->attribName();
	    mHandleParseErr(err);
	}
    }

    return true;
}


#define mHandleSteeringParseErr( str ) \
{ \
    errmsg = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg); \
}


bool DescSet::createSteeringDesc( const IOPar& steeringpar, 
				  BufferString defstring,
				  ObjectSet<Desc>& newsteeringdescs,
				  BufferStringSet* errmsgs )
{
    BufferString steeringdef = steeringpar.find("Type");
    steeringdef += " ";
    steeringdef += "stepout=";
    const char* stepout = steeringpar.find("Stepout");
    if ( stepout )
	steeringdef += stepout;
    else
    {
	BufferString pos0val;
	BufferString stepoutval;
	if ( Desc::getParamString( defstring, "stepout",stepoutval ) )
	{
	    BinIDParam stept("stepout");
	    stept.setCompositeValue(stepoutval);
	    BinID steptbid;
	    steptbid.inl = stept.getIntValue(0);
	    steptbid.crl = stept.getIntValue(1);
	    steeringdef += abs(steptbid.inl); steeringdef += ",";
	    steeringdef += abs(steptbid.crl);
	}
	else if ( Desc::getParamString( defstring, "pos0",pos0val ) )
	{
	    BufferString pos1val;
	    Desc::getParamString( defstring, "pos1",pos1val );
	    BinIDParam pos0("pos0");
	    BinIDParam pos1("pos1");
	    pos0.setCompositeValue(pos0val);
	    pos1.setCompositeValue(pos1val);
	    BinID pos0bid, pos1bid;
	    pos0bid.inl = pos0.getIntValue(0);
	    pos0bid.crl = pos0.getIntValue(1);
	    pos1bid.inl = pos1.getIntValue(0);
	    pos1bid.crl = pos1.getIntValue(1);
	    
	    int outputinl = mMAX( abs(pos0bid.inl), abs(pos1bid.inl) );
	    int outputcrl = mMAX( abs(pos0bid.crl), abs(pos1bid.crl) );
	    steeringdef += outputinl; steeringdef += ","; 
	    steeringdef+= outputcrl;
	}
	else
	    steeringdef += "5,5";
    }
    steeringdef += " ";
    steeringdef += "phlock=";
    bool phaselock = false;
    steeringpar.getYN( "PhaseLock", phaselock );
    steeringdef += phaselock ? "Yes" : "No";
    steeringdef += " ";
    if ( phaselock )
    {
	steeringdef += "aperture=";
	const char* aperture = steeringpar.find("Aperture");
	steeringdef += aperture ? aperture : "-5,5";
    }
    BufferString attribname;
    if ( !Desc::getAttribName( steeringdef, attribname ) )
    mHandleSteeringParseErr("Cannot find attribute name");
    RefMan<Desc> stdesc;
    stdesc = PF().createDescCopy(attribname);
    if ( !stdesc )
    {
	BufferString err = "Cannot find factory-entry for ";
	err += attribname;
	mHandleSteeringParseErr(err);
    }

    if ( !stdesc->parseDefStr(steeringdef) )
    {
	BufferString err = "Cannot parse: ";
	err += steeringdef;
	mHandleSteeringParseErr(err);
    }

    BufferString usserefstr = "steering input ";
    usserefstr += newsteeringdescs.size();
    stdesc->setUserRef( (const char*)usserefstr );
    stdesc->setSteering(true);
    
    const char* inldipstr = steeringpar.find("InlDipID");
    const char* crldipstr = steeringpar.find("CrlDipID");
    if ( inldipstr )
    {
	int idipdescnr = atoi(inldipstr);
	stdesc->setInput(0, descs[idipdescnr] );
    }
    if ( crldipstr )
    {
	int cdipdescnr = atoi(crldipstr);
	stdesc->setInput(1, descs[cdipdescnr] );
    }	
//TODO see what's going on for the phase input	
    stdesc->ref();
    newsteeringdescs += stdesc;
    
    return true;
}


const char* DescSet::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }


int DescSet::getFreeID() const
{
    int id = 0;
    while ( ids.indexOf(id)!=-1 )
	id++;

    return id;
}


bool DescSet::is2D() const
{
    for ( int idx=0; idx<descs.size(); idx++ )
    {
	if ( descs[idx]->is2D() )
	    return true;
    }

    return false;
}


DescSet* DescSet::optimizeClone( int targetnode, int tn2 ) const
{
    TypeSet<int> needednodes;
    needednodes += targetnode;
    if ( tn2 >= 0 ) needednodes += tn2;
    const int targetdescnr = getID( targetnode );
    if ( targetdescnr < 0 )
	return clone();

    DescSet* res = new DescSet;
    while ( needednodes.size() )
    {
	const int needednode = needednodes[0];
	needednodes.remove( 0 );
	int descnr = getID( needednode );
	if ( descnr==-1 )
	{
	    delete res;
	    return 0;
	}

	Desc* nd = descs[descnr]->clone();
	res->descs += nd;
	res->ids += ids[descnr];

	for ( int idx=0; idx<nd->nrInputs(); idx++ )
	{
	    const int inputid = nd->inputId(idx);
	    if ( inputid!=-1 && res->getID(inputid)==-1)
	    {
		needednodes += inputid;
	    }
	}
    }

    if ( res->nrDescs() == 0 )
	{ delete res; res = clone(); }
    return res;
}


}; // namespace Attrib
