/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "iostrm.h"
#include "iosubdir.h"
#include "ioman.h"
#include "iopar.h"
#include "iodir.h"
#include "ascstream.h"
#include "filepath.h"
#include "file.h"
#include "conn.h"
#include "errh.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "transl.h"
#include "keystrs.h"
#include "staticstring.h"

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
	, dirnm_("")
	, pars_(*new IOPar)
{
}


IOObj::IOObj( const IOObj& oth )
	: key_(oth.key_)
	, pars_(*new IOPar)
{
    copyStuffFrom( oth );
}


IOObj::~IOObj()
{
    delete &pars_;
}


void IOObj::copyStuffFrom( const IOObj& obj )
{
    setGroup( obj.group() );
    setTranslator( obj.translator() );
    setName( obj.name() );
    setDirName( obj.dirName() );
    pars_ = obj.pars_;
}


void IOObj::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    copyStuffFrom( *obj );
}


static FileMultiString fms;

IOObj* IOObj::get( ascistream& astream, const char* dirnm, const char* dirky )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    if ( *astream.keyWord() == '@' )
	return IOSubDir::get( astream, dirnm );

    BufferString nm( astream.keyWord() );
    fms = astream.value();
    MultiID objkey( dirky ); objkey += fms[0];
    astream.next();
    BufferString groupnm( astream.keyWord() );
    fms = astream.value();
    BufferString trlnm( fms[0] );
    BufferString objtyp( fms[1] );
    if ( objtyp.isEmpty() )
    {
	TranslatorGroup& grp = TranslatorGroup::getGroup( groupnm, true );
	if ( grp.userName() != groupnm )
	    return 0;
	Translator* tr = grp.make( trlnm );
	if ( !tr )
	    return 0;

	objtyp = tr->connType();
	delete tr;
    }

    IOObj* objptr = produce( objtyp, nm, objkey, false );
    if ( !objptr ) return 0;

    objptr->setGroup( groupnm );
    objptr->setTranslator( trlnm );

    astream.next();
    if ( *astream.keyWord() != '$' )
    	{ delete objptr; objptr = 0; }
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


Translator* IOObj::createTranslator() const
{
    if ( isSubdir() ) return 0;

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
    if ( isSubdir() )
	return new IOSubDir( *((IOSubDir*)this) );

    IOObj* ret = produce( connType(), name(), key(), false );
    if ( !ret )
	{ pErrMsg("Cannot 'produce' IOObj of my own type"); return 0; }
    ret->copyFrom( this );
    return ret;
}


void IOObj::acquireNewKey()
{ key_ = IOM().dirPtr()->newKey(); }


bool IOObj::isKey( const char* ky )
{ return IOM().isKey(ky); }


bool IOObj::put( ascostream& astream ) const
{
    if ( !isSubdir() )
    {
	astream.put( name(), myKey() );
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
    if ( myKey() < 2 || isSubdir() ) return false;
    PtrMan<Translator> tr = createTranslator();
    if ( !tr ) return false;

    bool isrddef = tr->isReadDefault();
    return isrddef;
}


void IOObj::setSurveyDefault( const char* subsel ) const
{
    CompoundKey defaultkey = sKey::Default().str();
    PtrMan<Translator> tr = createTranslator();
    defaultkey += tr->group()->getSurveyDefaultKey( this );
    if ( subsel )
	defaultkey += subsel;
    
    SI().getPars().set( defaultkey.buf(), key() );
    SI().savePars();
}


bool IOObj::isSurveyDefault( const MultiID& ky )
{
    IOPar* dpar = SI().pars().subselect( sKey::Default() );
    bool ret = false;
    if ( dpar && !dpar->isEmpty() )
	ret = dpar->findKeyFor( ky );
    delete dpar;
    return ret;
}


bool IOObj::isInCurrentSurvey() const
{
    FilePath cursurvfp( IOM().rootDir() ); cursurvfp.makeCanonical();
    FilePath orgfp( fullUserExpr(true) ); orgfp.makeCanonical();
    return orgfp.isSubDirOf(cursurvfp);
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
    if ( ioobj.isSubdir() ) return false;
    
    PtrMan<Translator> tr = ioobj.createTranslator();
    return tr ? tr->implRemove( &ioobj ) : ioobj.implRemove();
}


IOSubDir::IOSubDir( const char* subdirnm )
    : IOObj(subdirnm)
    , isbad_(false)
{
}


IOSubDir::IOSubDir( const IOSubDir& oth )
    : IOObj(oth)
    , isbad_(oth.isbad_)
{
}


IOSubDir* IOSubDir::get( ascistream& strm, const char* dirnm )
{
    IOSubDir* ret = new IOSubDir( strm.value() );
    ret->key_ = strm.keyWord() + 1;
    ret->dirnm_ = dirnm;
    ret->isbad_ = !File::isDirectory( ret->dirName() );
    strm.next(); return ret;
}


const char* IOSubDir::fullUserExpr( bool ) const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = FilePath(dirnm_,name()).fullPath();
    return ret;
}


bool IOSubDir::putTo( ascostream& stream ) const
{
    if ( bad() ) return false;
    const BufferString str( "@", myKey() );
    stream.put( str, name() );
    return true;
}
