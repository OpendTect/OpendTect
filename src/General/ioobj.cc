/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ioobj.h"

#include "ascstream.h"
#include "compoundkey.h"
#include "conn.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "iopar.h"
#include "iosubdir.h"
#include "iox.h"
#include "keystrs.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "transl.h"
#include "uistrings.h"


class OwnedProducerList : public ObjectSet<const IOObjProducer>
{
    public:
	~OwnedProducerList() { deepErase( *this ); }
};


static ObjectSet<const IOObjProducer>& getProducers()
{
    mDefineStaticLocalObject( PtrMan<ObjectSet<const IOObjProducer> >, prods,
			      = new OwnedProducerList );
    return *prods;
}


class IOXProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const override
		{ return StringView(typ)==XConn::sType(); }
    IOObj*	make( const char* nm,
		      const MultiID& ky, bool fd ) const override
		{ return new IOX(nm,ky.toString(),fd); }
};

int IOX::prodid = IOObj::addProducer( new IOXProducer );


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


IOObj::IOObj( const char* nm, const MultiID& ky )
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

IOObj* IOObj::get( ascistream& astream, const char* dirnm, int groupid )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    if ( *astream.keyWord() == '@' )
	return IOSubDir::get( astream, dirnm );

    BufferString nm( astream.keyWord() );
    fms = astream.value();
    const MultiID objkey( groupid, fms.getIValue(0) );

    astream.next();
    BufferString groupnm( astream.keyWord() );
    fms = astream.value();
    BufferString trlnm( fms[0] );
    BufferString objtyp( fms[1] );
    if ( objtyp.isEmpty() )
    {
	TranslatorGroup& grp = TranslatorGroup::getGroup( groupnm );
	if ( grp.groupName() != groupnm )
	    return 0;
	Translator* tr = grp.make( trlnm, true );
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
	deleteAndZeroPtr( objptr );
    else
    {
	if ( !objptr->getFrom(astream) || objptr->isBad() )
	    deleteAndZeroPtr( objptr );
	else
	{
	    while ( *astream.keyWord() == '#' )
	    {
		objptr->pars_.set( astream.keyWord()+1, astream.value() );
		astream.next();
	    }
	}
    }

    while ( !atEndOfSection(astream) )
	astream.next();

    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const MultiID& keyin,
			bool gendef )
{
    if ( !nm || !*nm )
	nm = "?";

    if ( keyin.isUdf() )
    {
	pFreeFnErrMsg( "IOObj : Empty key given");
	return nullptr;
    }

    const ObjectSet<const IOObjProducer>& prods = getProducers();
    for ( int idx=0; idx<prods.size(); idx++ )
    {
	const IOObjProducer& prod = *prods[idx];
	if ( prod.canMake(typ) )
	    return prod.make( nm, keyin, gendef );
    }

    return 0;
}


Translator* IOObj::createTranslator() const
{
    if ( isSubdir() ) return 0;

    TranslatorGroup& grp = TranslatorGroup::getGroup( group() );
    if ( grp.groupName() != group() )
	return 0;

    Translator* tr = grp.make( translator(), true );
    if ( !tr ) return 0;

    if ( pars_.size() ) tr->usePar( pars_ );
    return tr;
}


IOObj* IOObj::clone() const
{
    if ( isSubdir() )
	return new IOSubDir( *((IOSubDir*)this) );

    if ( key().isUdf() )
	return 0;

    IOObj* ret = produce( connType(), name(), key(), false );
    if ( !ret )
	{ pErrMsg("Cannot 'produce' IOObj of my own type"); return 0; }
    ret->copyFrom( this );
    return ret;
}


void IOObj::acquireNewKeyIn( const MultiID& dirky )
{
    key_ = IOM().createNewKey( dirky );
}


bool IOObj::implIsLink() const
{
    bool ret = false;
    pars().getYN( sKey::IsLink(), ret );
    return ret;
}


bool IOObj::isKey( const char* ky )
{
    return IOM().isKey(ky);
}


void IOObj::updateCreationPars() const
{
    pars().setStdCreationEntries();
}


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

    IOParIterator iter( pars_ );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	astream.stream() << '#';
	astream.put( key, val );
    }

    astream.newParagraph();
    return astream.isOK();
}


int IOObj::myKey() const
{
    return key_.objectID();
}


bool IOObj::isProcTmp() const
{
    return name().startsWith( "~Proc" );
}


bool IOObj::isUserSelectable( bool forread ) const
{
    if ( myKey() < 2 || isSubdir() )
	return false;

    PtrMan<Translator> tr = createTranslator();
    return tr ? tr->isUserSelectable( forread ) : false;
}


void IOObj::setSurveyDefault( const char* subsel ) const
{
    PtrMan<Translator> tr = createTranslator();
    if ( !tr )
	return;

    CompoundKey defaultkey = tr->group()->getSurveyDefaultKey( this );
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
	ret = dpar->hasKey( ky.toString() );

    delete dpar;
    return ret;
}


#define mQuotedName toUiString(name()).quote(true)

uiString IOObj::phrCannotOpenObj() const
{
    return uiStrings::phrCannotOpen( mQuotedName );
}


uiString IOObj::phrCannotReadObj() const
{
    return uiStrings::phrCannotRead( mQuotedName );
}


uiString IOObj::phrCannotLoadObj() const
{
    return uiStrings::phrCannotLoad( mQuotedName );
}


uiString IOObj::phrCannotWriteObj() const
{
    return uiStrings::phrCannotWrite( mQuotedName );
}


uiString IOObj::phrCannotWriteToDB() const
{
    return uiStrings::phrCannotWriteDBEntry( mQuotedName );
}


bool IOObj::isInCurrentSurvey() const
{
    FilePath cursurvfp( IOM().rootDir() ); cursurvfp.makeCanonical();
    FilePath orgfp( fullUserExpr(true) ); orgfp.makeCanonical();
    return orgfp.isSubDirOf(cursurvfp);
}


bool areEqual( const IOObj* o1, const IOObj* o2 )
{
    if ( !o1 && !o2 )
	return true;

    if ( !o1 || !o2 )
	return false;

    return o1->key() == o2->key();
}


bool equalIOObj( const MultiID& ky1, const MultiID& ky2 )
{
    return ky1 == ky2;
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
    auto* ret = new IOSubDir( strm.value() );
    ret->key_.setGroupID( toInt(strm.keyWord()+1) );
    ret->key_.setObjectID( 0 );
    ret->dirnm_ = dirnm;
    ret->isbad_ = !File::isDirectory( ret->dirName() );
    strm.next(); return ret;
}


const char* IOSubDir::fullUserExpr( bool ) const
{
    mDeclStaticString( ret );
    ret = FilePath(dirnm_,name()).fullPath();
    return ret;
}


bool IOSubDir::putTo( ascostream& stream ) const
{
    if ( isBad() ) return false;
    const BufferString str( "@", myKey() );
    stream.put( str, name() );
    return true;
}


IOX::IOX( const char* nm, const char* ky, bool )
	: IOObj(nm,ky)
	, ownkey_("")
{
}


IOX::~IOX()
{
}


void IOX::setOwnKey( const MultiID& ky )
{
    ownkey_ = ky;
}


const char* IOX::connType() const
{
    return XConn::sType();
}


bool IOX::isBad() const
{
    return ownkey_.isUdf();
}


void IOX::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOX*,trobj,obj)
    if ( trobj )
	ownkey_ = trobj->ownkey_;
}


const char* IOX::fullUserExpr( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj )
	return "<invalid>";

    const char* s = ioobj->fullUserExpr(i);
    delete ioobj;
    return s;
}


bool IOX::implExists( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj )
	return false;

    const bool yn = ioobj->implExists(i);
    delete ioobj;
    return yn;
}


Conn* IOX::getConn( bool forread ) const
{
    XConn* ret = 0;

    IOObj* ioobj = getIOObj();
    if ( ioobj )
    {
	ret = new XConn;
	ret->conn_ = ioobj->getConn( forread );
	delete ioobj;
	ret->setLinkedTo( key() );
    }

    return ret;
}


IOObj* IOX::getIOObj() const
{
    return ownkey_.isUdf() ? nullptr : IOM().get( ownkey_ );
}


bool IOX::getFrom( ascistream& stream )
{
    ownkey_.fromString( stream.value() );
    stream.next();
    return true;
}


bool IOX::putTo( ascostream& stream ) const
{
    stream.stream() << '$';
    stream.put( "ID", ownkey_ );
    return true;
}


const char* IOX::dirName() const
{
    IOObj* ioobj = getIOObj();
    if ( !ioobj ) return dirnm_;
    const_cast<IOX*>(this)->dirnm_ = ioobj->dirName();
    delete ioobj;
    return dirnm_;
}
