/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: ioobj.cc,v 1.33 2010-10-14 09:58:06 cvsbert Exp $";

#include "ascstream.h"
#include "conn.h"
#include "errh.h"
#include "iodir.h"
#include "iolink.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "transl.h"

#include <stdlib.h>





static ObjectSet<const IOObjProducer>& getProducers()
{
    static ObjectSet<const IOObjProducer>* prods = 0;
    if ( !prods ) prods = new ObjectSet<const IOObjProducer>;
    return *prods;
}


int IOObj::addProducer( IOObjProducer* prod )
{
    if ( !prod ) return -1;
    ObjectSet<const IOObjProducer>& prods = getProducers();
    prods += prod;
    return prods.size();
}


IOObj::IOObj( const char* nm, const char* ky )
	: NamedObject(nm)
	, key_(ky)
	, dirname_(0)
	, pars_(*new IOPar)
{
}


IOObj::IOObj( IOObj* l, const char* ky )
	: NamedObject(l)
	, key_(ky)
	, dirname_(0)
	, pars_(*new IOPar)
{
}


IOObj::~IOObj()
{
    delete dirname_;
    delete &pars_;
}


void IOObj::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    setParentKey( obj->parentKey() );
    setGroup( obj->group() );
    setTranslator( obj->translator() );
    setName( obj->name() );
    pars_ = obj->pars_;
}


IOObj* IOObj::getParent() const
{
    return IODir::getObj( parentKey() );
}


const char* IOObj::dirName() const
{
    if ( dirname_ ) return *dirname_;
    return IOM().curDir();
}


static FileMultiString fms;

IOObj* IOObj::get( ascistream& astream, const char* dirnm, const char* dirky )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    MultiID mykey( dirky );
    if ( *astream.keyWord() == '@' )
    {
	IOLink* ln = IOLink::get( astream, dirnm );
	if ( !ln ) return 0;
	mykey += ln->key();
	ln->setKey( mykey );
	return ln;
    }

    UserIDString _name( astream.keyWord() );
    fms = astream.value();
    mykey += fms[0];
    MultiID parkey( fms[1] );
    astream.next();
    UserIDString _group( astream.keyWord() );
    fms = astream.value();
    UserIDString _trl( fms[0] );
    UserIDString _typ( fms[1] );
    if ( ! *(const char*)_typ )
    {
	TranslatorGroup& grp = TranslatorGroup::getGroup( _group, true );
	if ( grp.userName() != _group )
	    return 0;

	Translator* tr = grp.make( _trl );
	if ( !tr ) return 0;

	_typ = tr->connType();
	delete tr;
    }

    IOObj* objptr = produce( _typ, _name, mykey, false );
    if ( !objptr ) return 0;

    objptr->setParentKey( parkey );
    objptr->setGroup( _group );
    objptr->setTranslator( _trl );

    astream.next();
    if ( *astream.keyWord() != '$' )	{ delete objptr; objptr = 0; }
    else
    {
	if ( !objptr->getFrom(astream) || objptr->bad() )
	    { delete objptr; objptr = 0; }
	else
	{
	    while ( *astream.keyWord() == '#' )
	    {
		objptr->pars_.set( astream.keyWord()+1, astream.value() );
		astream.next();
	    }
	}
    }

    while ( !atEndOfSection(astream) ) astream.next();
    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const char* keyin,
			bool gendef )
{
    IOObj* objptr = 0;

    if ( !nm || !*nm ) nm = "?";
    MultiID ky( keyin );
    if ( ky.isEmpty() && IOM().dirPtr() )
	ky = IOM().dirPtr()->newKey();

    const ObjectSet<const IOObjProducer>& prods = getProducers();
    for ( int idx=0; idx<prods.size(); idx++ )
    {
	const IOObjProducer& prod = *prods[idx];
	if ( prod.canMake(typ) )
	    return prod.make( nm, ky, gendef );
    }
    return 0;
}


Translator* IOObj::getTranslator() const
{
    TranslatorGroup& grp = TranslatorGroup::getGroup( group(), true );
    if ( grp.userName() != group() )
	return 0;

    Translator* tr = grp.make( translator() );
    if ( !tr ) return 0;

    if ( pars_.size() ) tr->usePar( pars_ );
    return tr;
}


IOObj* IOObj::clone() const
{
    const IOObj* dataobj = isLink() ? ((IOLink*)this)->link() : this;
    IOObj* newioobj = produce( dataobj->connType(), name(), dataobj->key(),
	    			false);
    if ( !newioobj )
	{ pErrMsg("Cannot produce IOObj of my own type"); return 0; }
    newioobj->copyFrom( dataobj );
    newioobj->setStandAlone( dataobj->dirName() );
    return newioobj;
}


void IOObj::acquireNewKey()
{
    key_ = IOM().dirPtr()->newKey();
}


bool IOObj::isKey( const char* ky )
{
    if ( !ky || !*ky || !isdigit(*ky) ) return false;

    bool digitseen = false;
    while ( *ky )
    {
	if ( isdigit(*ky) )
	    digitseen = true;
	else if ( *ky == '|' )
	    return digitseen;
	else if ( *ky != '.' )
	    return false;
	ky++;
    }

    return true;
}


void IOObj::setStandAlone( const char* dirname )
{
    if ( !dirname )
    {
	delete dirname_;
	dirname_ = 0;
	return;
    }
    if ( dirname_ )	*dirname_ = dirname;
    else		dirname_ = new FileNameString( dirname );
}


bool IOObj::put( ascostream& astream ) const
{
    if ( !isLink() )
    {
	if ( parentKey().isEmpty() )
	    astream.put( name(), myKey() );
	else
	{
	    fms = "";
	    fms += myKey();
	    fms += parentKey();
	    astream.put( name(), fms );
	}
	fms = translator();
	fms += connType();
	astream.put( group(), fms );
    }

    if ( !putTo( astream ) )
	return false;
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	astream.stream() << '#';
	astream.put( pars_.getKey(idx), pars_.getValue(idx) );
    }
    astream.newParagraph();
    return true;
}


int IOObj::myKey() const
{
    return toInt( key_.key( key_.nrKeys()-1 ) );
}


bool IOObj::isReadDefault() const
{
    if ( myKey() < 2 ) return false;
    Translator* tr = getTranslator();
    if ( !tr ) return false;

    bool isrddef = tr->isReadDefault();
    delete tr;
    return isrddef;
}


bool IOObj::isSurveyDefault( const MultiID& ky )
{
    IOPar* dpar = SI().pars().subselect( "Default" );
    bool ret = false;
    if ( dpar && !dpar->isEmpty() )
	ret = dpar->findKeyFor( ky );
    delete dpar;
    return ret;
}


bool areEqual( const IOObj* o1, const IOObj* o2 )
{
    if ( !o1 && !o2 ) return true;
    if ( !o1 || !o2 ) return false;

    return equalIOObj(o1->key(),o2->key());
}


static void mkStd( MultiID& ky )
{
    if ( ky.ID(ky.nrKeys()-1) == 1 ) ky = ky.upLevel();
    if ( ky.isEmpty() ) ky = "0";
}


bool equalIOObj( const MultiID& ky1, const MultiID& ky2 )
{
    MultiID k1( ky1 ); MultiID k2( ky2 );
    mkStd( k1 ); mkStd( k2 );
    return k1 == k2;
}


bool fullImplRemove( const IOObj& ioobj )
{
    PtrMan<Translator> tr = ioobj.getTranslator();
    return tr ? tr->implRemove( &ioobj ) : ioobj.implRemove();
}
