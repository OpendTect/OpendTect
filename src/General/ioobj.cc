/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 2-8-1994
-*/


#include "iox.h"
#include "iosubdir.h"
#include "dbman.h"
#include "iopar.h"
#include "dbdir.h"
#include "ascstream.h"
#include "filepath.h"
#include "file.h"
#include "conn.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "transl.h"
#include "keystrs.h"
#include "dateinfo.h"
#include "compoundkey.h"
#include "staticstring.h"
#include "uistrings.h"

static const int cKpTmpObjsInDays = 7;


class InvalidIOObj : public IOObj
{
public:

    virtual bool	isBad() const			{ return true; }
    virtual void	copyFrom(const IOObj&)		{}
    virtual bool	hasConnType(const char*) const	{ return false; }
    virtual const char*	connType() const		{ return ""; }
    virtual Conn*	getConn(bool) const		{ return 0; }
    virtual const char*	fullUserExpr(bool) const	{ return "<Undef>"; }
    virtual BufferString mainFileName() const		{ return "udf"; }
    virtual bool	implExists(bool) const		{ return false; }

protected:

    virtual bool	getFrom(ascistream&)		{ return false; }
    virtual bool	putTo(ascostream&) const	{ return false; }
    virtual bool	isEqTo( const IOObj& othioobj ) const
			{
			    mDynamicCastGet(const InvalidIOObj*,oth,&othioobj)
			    return oth;
			}

};

static InvalidIOObj theinst_;
const IOObj& IOObj::getInvalid() { return theinst_; }


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
    bool	canMake( const char* typ ) const
		{ return FixedString(typ)==XConn::sType(); }
    IOObj*	make( const char* nm, const DBKey& ky, bool fd ) const
		{ return new IOX(nm,ky,fd); }
};

int IOX::prodid = IOObj::addProducer( new IOXProducer );


int IOObj::addProducer( IOObjProducer* prod )
{
    if ( !prod ) return -1;
    ObjectSet<const IOObjProducer>& prods = getProducers();
    prods += prod;
    return prods.size();
}



IOObj::IOObj( const char* nm, DBKey ky )
	: NamedObject(nm)
	, key_(ky)
	, dirnm_("")
	, pars_(*new IOPar)
{
}


IOObj::IOObj( const IOObj& oth )
    : NamedObject(oth)
    , key_(oth.key_)
    , pars_(*new IOPar)
{
    copyClassData( oth );
}


IOObj::~IOObj()
{
    delete &pars_;
}


void IOObj::copyFrom( const IOObj& obj )
{
    setName( obj.name() );
    copyClassData( obj );
}


void IOObj::copyClassData( const IOObj& obj )
{
    setGroup( obj.group() );
    setTranslator( obj.translator() );
    setDirName( obj.dirName() );
    pars_ = obj.pars_;
}


bool IOObj::isEqualTo( const IOObj& oth ) const
{
    if ( key() != oth.key()
      || group() != oth.group()
      || translator() != oth.translator()
      || pars() != oth.pars() )
	return false;

    return isEqTo( oth );
}



IOObj* IOObj::get( ascistream& astream, const char* dirnm,
		    DirID dirid, bool rejectoldtmps )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    if ( *astream.keyWord() == '@' )
	return IOSubDir::get( astream, dirnm );

    BufferString nm( astream.keyWord() );
    FileMultiString fms = astream.value();
    const DBKey::ObjNrType objnr = fms.getIValue( 0 );
    DBKey objkey( dirid, DBKey::ObjID::get(objnr) );
    bool reject = dirid.getI() < 0 || objnr < 1;
    if ( rejectoldtmps && isTmpObjNr(objnr) )
    {
	const int dikey = fms.getIValue( 1 );
	if ( dikey < DateInfo().key()-cKpTmpObjsInDays )
	    reject = true;
    }
    astream.next();
    BufferString groupnm( astream.keyWord() );
    fms = astream.value();
    BufferString trlnm( fms[0] );
    BufferString objtyp( fms[1] );
    if ( objtyp.isEmpty() )
    {
	TranslatorGroup& grp = TranslatorGroup::getGroup( groupnm );
	if ( grp.groupName() != groupnm )
	    reject = true;
	else
	{
	    Translator* tr = grp.make( trlnm, true );
	    if ( !tr )
		reject = true;
	    else
	    {
		objtyp = tr->connType();
		delete tr;
	    }
	}
    }

    IOObj* objptr = produce( objtyp.str(), nm.str(), objkey, false );
    if ( objptr )
    {
	objptr->setGroup( groupnm );
	objptr->setTranslator( trlnm );

	astream.next();
	if ( *astream.keyWord() != '$' )
	    reject = true;
	else
	{
	    if ( !objptr->getFrom(astream) || objptr->isBad() )
		reject = true;
	    else
	    {
		while ( *astream.keyWord() == '#' )
		{
		    objptr->pars_.set( astream.keyWord()+1, astream.value() );
		    astream.next();
		}
	    }
	}
    }

    while ( !atEndOfSection(astream) )
	astream.next();

    if ( reject )
	{ delete objptr; objptr = 0; }
    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const DBKey& ky,
			bool gendef )
{
    if ( !typ || !*typ )
	return 0;

    if ( !nm || !*nm )
	nm = "?";
    if ( ky.isInvalid() )
	{ pFreeFnErrMsg( "IOObj : Empty key given"); return 0; }

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

    if ( key().isInvalid() )
	return 0;

    IOObj* ret = produce( connType(), name(), key(), false );
    if ( !ret )
	{ pErrMsg("Cannot 'produce' IOObj of my own type"); return 0; }

    ret->copyFrom( *this );
    return ret;
}


void IOObj::setKeyForNewEntry( DirID dirid )
{
    key_.setDirID( dirid );
    key_.setObjID( DBKey::ObjID::getInvalid() );
}




bool IOObj::isKey( const char* ky )
{
    return DBKey::isValidString( ky );
}


void IOObj::updateCreationPars() const
{
    pars().setStdCreationEntries();
}


bool IOObj::put( ascostream& astream ) const
{
    if ( !isSubdir() )
    {
	FileMultiString fms;
	if ( !isTmp() )
	    astream.put( name(), objID().getI() );
	else
	{
	    fms.set( objID().getI() );
	    fms.add( DateInfo().key() );
	    astream.put( name(), fms );
	}
	fms.set( translator() ).add( connType() );
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
    return astream.isOK();
}


bool IOObj::isProcTmp() const
{
    return name().startsWith( "~Proc" );
}


bool IOObj::isUserSelectable( bool forread ) const
{
    if ( objID().getI() < 2 || isSubdir() || isTmp() )
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

    SI().setDefaultPar( defaultkey.buf(), key().toString(), true );
}


bool IOObj::isSurveyDefault( const DBKey& ky )
{
    IOPar* dpar = SI().getDefaultPars().subselect( sKey::Default() );
    bool ret = false;
    if ( dpar && !dpar->isEmpty() )
	ret = dpar->findKeyFor( ky.toString() );
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


uiRetVal IOObj::commitChanges() const
{
    if ( !key().isValid() )
	return mINTERNAL( "Empty DB Key" );
    else if ( name().isEmpty() )
	return mINTERNAL( "Empty IOObj name" );
    else if ( !DBM().setEntry(*this) )
	return phrCannotWriteToDB();
    return uiRetVal();
}


bool IOObj::removeFromDB() const
{
    return DBM().removeEntry( key() );
}


static void mkStd( DBKey& ky )
{
    if ( ky.isInvalid() )
	ky = DBKey( "0" );
    else if ( ky.objID().getI() == 1 )
	ky.setInvalidObj();
}


bool equalIOObj( const DBKey& ky1, const DBKey& ky2 )
{
    DBKey k1( ky1 ); DBKey k2( ky2 );
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
    const DBKey readky = DBKey( strm.keyWord() + 1 );
    ret->key_.setObjID( DBKey::ObjID::get(readky.dirID().getI()) );
    ret->key_.setDirID( DBKey::DirID::get( 0 ) );
    ret->dirnm_ = dirnm;
    ret->isbad_ = !File::isDirectory( ret->dirName() );
    strm.next(); return ret;
}


const char* IOSubDir::fullUserExpr( bool ) const
{
    File::Path fp( dirnm_ );
    const BufferString fnm( fp.fileName() );
    const BufferString mynm( name() );
    if ( fnm == mynm )
	return dirnm_;

    static BufferString ret;
    ret = fp.add( mynm ).fullPath();
    return ret.buf();
}


bool IOSubDir::putTo( ascostream& stream ) const
{
    if ( isBad() ) return false;
    const BufferString str( "@", objID().getI() );
    stream.put( str, name() );
    return true;
}


bool IOSubDir::isEqTo( const IOObj& othioobj ) const
{
    mDynamicCastGet( const IOSubDir*, oth, &othioobj )
    if ( !oth )
	return false;

    const BufferString mydirnm( fullUserExpr(true) );
    const BufferString othdirnm( oth->fullUserExpr(true) );
    return mydirnm == othdirnm;
}


IOX::IOX( const char* nm, const DBKey ky, bool )
	: IOObj(nm,ky)
{
}


IOX::~IOX()
{
}


void IOX::setOwnKey( const DBKey& ky )
{
    ownkey_ = ky;
}


const char* IOX::connType() const
{
    return XConn::sType();
}


bool IOX::isBad() const
{
    return ownkey_.isInvalid();
}


void IOX::copyFrom( const IOObj& obj )
{
    IOObj::copyFrom( obj );
    mDynamicCastGet(const IOX*,trobj,&obj)
    if ( trobj )
	ownkey_ = trobj->ownkey_;
}


const char* IOX::fullUserExpr( bool forread ) const
{
    IOObj* ioobj = ownkey_.getIOObj();
    if ( !ioobj )
	return "<invalid>";
    mDeclStaticString( ret );
    ret = ioobj->fullUserExpr( forread );
    delete ioobj;
    return ret.buf();
}


BufferString IOX::mainFileName() const
{
    IOObj* ioobj = ownkey_.getIOObj();
    if ( !ioobj )
	return BufferString( "invalid" );
    const BufferString ret = ioobj->mainFileName();
    delete ioobj;
    return ret;
}


bool IOX::implExists( bool forread ) const
{
    return ::implExists( ownkey_ );
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
    return ownkey_.getIOObj();
}


bool IOX::isEqTo( const IOObj& othioobj ) const
{
    mDynamicCastGet( const IOX*, oth, &othioobj )
    if ( !oth )
	return false;

    return true;
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
    stream.put( "ID", ownkey_.toString() );
    return true;
}


// Beware! used *during* copy of DBDir's, hence you cannot use getIOObj()
const char* IOX::dirName() const
{
    if ( ownkey_.isInvalid() )
	return "";

    const_cast<IOX*>(this)->dirnm_
		= DBM().getDirectoryNameOf( ownkey_.dirID(), true );
    return dirnm_.buf();
}
