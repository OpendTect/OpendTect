/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribdescset.cc,v 1.2 2005-02-03 15:35:02 kristofer Exp $";

#include "attribdescset.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "bufstringset.h"
#include "iopar.h"

namespace Attrib
{

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


int DescSet::getID(const Desc& desc) const
{
    const int idx=descs.indexOf(&desc);
    return idx==-1 ? -1 : ids[idx];
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


void DescSet::fillPar(IOPar& par) const
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

	    BufferString key = inputPrefixStr();
	    key += input;

	    apar.set( key, getID( *descs[idx]->getInput(input)) );
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

    for ( int id=0; id<=maxid; id++ )
    {
	const BufferString idstr( id );
	PtrMan<IOPar> descpar = par.subselect(idstr);
	if ( !descpar ) continue;

	//Look for type (old format)
	const char* typestr = descpar->find("Type");
	if ( typestr && !strcmp(typestr,"Stored" ) )
	{
	    //TODO: Convert old format
	}
	else if ( typestr && strcmp(typestr,"Calculated") )
	{
	    //TODO: Convert old format
	}

	const char* defstring = descpar->find(definitionStr());
	if ( !defstring )
	    mHandleParseErr("No attribute definition string specified");

	BufferString attribname;
	if ( !Desc::getAttribName( defstring, attribname ) )
	    mHandleParseErr("Cannot find attribute name");

	RefMan<Desc> desc = PF().createDescCopy(attribname);
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
    }

    for ( int idx=0; idx<descs.size(); idx++ )
    {
	const BufferString idstr( ids[idx] );
	PtrMan<IOPar> descpar = par.subselect(idstr);

	for ( int input=0; input<descs[idx]->nrInputs(); input++ )
	{
	    BufferString key = inputPrefixStr();
	    key += input;

	    int inpid;
	    if ( !descpar->get( key, inpid ) ) continue;

	    Desc* inpdesc = getDesc( inpid );
	    if ( !inpdesc ) continue;

	    descs[idx]->setInput( input, inpdesc );
	}

	if ( descs[idx]->isSatisfied()!=0 )
	{
	    //Do something
	}
    }

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

}; //namespace
