/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

#include "transl.h"
#include "ioobj.h"
#include "iopar.h"
#include <iostream>

static const char* rcsID = "$Id: transl.cc,v 1.6 2001-07-26 09:44:51 windev Exp $";

DefineAbstractClassDef(Translator,"Translator");


void Translator::dumpGroups( ostream& strm )
{
    const UserIDObjectSet<Translator>& grps = Translator::groups();
    for ( int igrp=0; igrp<grps.size(); igrp++ )
    {
	const Translator* trg = grps[igrp];
	strm << trg->name() << endl;
	const ClassDefList& cdefs = trg->defs();
	for ( int idx=0; idx<cdefs.size(); idx++ )
	{
	    const ClassDef* cd = cdefs[idx];
	    Translator* tr = (Translator*)cd->getNew();
	    strm << "\t" << tr->connClassDef().name()
		 << " :\t" << cd->name() << endl;
	    delete tr;
	}
	strm << endl;
    }
}


Translator* Translator::produce( const char* grp, const char* nm )
{
    if ( !nm || !*nm ) return 0;

    const UserIDObjectSet<Translator>& grps = Translator::groups();
    for ( int igrp=0; igrp<grps.size(); igrp++ )
    {
	const Translator* trg = grps[igrp];
	if ( trg->name() == grp )
	    return trg->make( nm );
    }
    return 0;
}


Translator* Translator::trProd( const char* nm ) const
{
    if ( !nm || !*nm ) return 0;
    
    const ClassDefList& cds = defs();

    // Full match is always welcome
    ClassDef* cd = cds[nm];
    Translator* tr = (Translator*)(cd ? cd->getNew() : 0);
    if ( tr ) return tr;

    // Now try to match only given string - may be part of full name
    for ( int idx=0; idx<cds.size(); idx++ )
    {
	if ( matchString(nm,cds[idx]->name()) )
	{
	    if ( cd ) // more than one match
		return 0;
	    cd = cds[idx];
	}
    }
    return cd ? (Translator*)cd->getNew() : 0;
}


IOPar& Translator::mkSelHist( const char* nm )
{
    BufferString parnm = nm;
    parnm += " selection history";
    return *new IOPar( parnm );
}


int Translator::implExists( const IOObj* ioobj, int forread ) const
{
    if ( !ioobj ) return NO;
    return ioobj->implExists( forread );
}
int Translator::implRemovable( const IOObj* ioobj ) const
{
    if ( !ioobj ) return NO;
    return ioobj->implRemovable();
}
int Translator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return NO;
    return ioobj->implRemove();
}


int Translator::hasConnDef( const ClassDef& cd ) const
{
    for ( int itr=0; itr<defs().size(); itr++ )
    {
	const ClassDef* def = defs()[itr];
	Translator* tr = (Translator*)def->getNew();
	if ( tr->conndef_ == &cd ) { delete tr; return YES; }
	delete tr;
    }
    return NO;
}
