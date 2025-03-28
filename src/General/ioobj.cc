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


// OD::DataSetKey

OD::DataSetKey::DataSetKey()
    : Uuid()
{}


OD::DataSetKey::DataSetKey( const char* dsidstr )
    : Uuid(dsidstr)
{}


OD::DataSetKey::DataSetKey( bool undef )
    : Uuid(undef)
{}


OD::DataSetKey::~DataSetKey()
{}


void OD::DataSetKey::fillPar( IOPar& iop ) const
{
    if ( isUdf() )
	iop.removeWithKey( sKeyID() );
    else
	iop.set( sKeyID(), toString(false) );
}


BufferString OD::DataSetKey::create( bool withbraces )
{
    return Uuid::create( withbraces );
}


OD::DataSetKey OD::DataSetKey::getFrom( const IOPar& iop )
{
    const BufferString dsidstr = iop.find( sKeyID() );
    if ( dsidstr.isEmpty() )
	return OD::DataSetKey::udf();

    return OD::DataSetKey( dsidstr.buf() );
}


const OD::DataSetKey& OD::DataSetKey::udf()
{
    static DataSetKey udfdsid( true );
    return udfdsid;
}


const char* OD::DataSetKey::sKeyID()
{
    static BufferString ret( IOPar::compKey(sKey::DataSet(),sKey::ID()) );
    return ret.str();
}


// OwnedProducerList

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
		      const DBKey& ky, bool fd ) const override
		{ return new IOX(nm,ky,fd); }
};


int IOX::prodid = IOObj::addProducer( new IOXProducer );


// IOObj::IOObj

int IOObj::addProducer( IOObjProducer* prod )
{
    if ( !prod )
	return -1;

    ObjectSet<const IOObjProducer>& prods = getProducers();
    prods += prod;
    return prods.size();
}


IOObj::IOObj( const char* nm, const DBKey& ky )
    : NamedObject(nm)
    , key_(ky)
    , pars_(*new IOPar)
{
}


IOObj::IOObj( const IOObj& oth )
    : key_(oth.key_)
    , pars_(*new IOPar)
{
    copyStuffFrom( oth );
}


IOObj::IOObj( const char* nm, const MultiID& ky )
    : NamedObject(nm)
    , key_(ky)
    , pars_(*new IOPar)
{
}


IOObj::IOObj( const char* nm, const char* ky )
    : NamedObject(nm)
    , pars_(*new IOPar)
{
    key_.fromString( ky );
}


IOObj::~IOObj()
{
    delete &pars_;
    delete dskey_;
}


void IOObj::copyStuffFrom( const IOObj& obj )
{
    setGroup( obj.group() );
    setTranslator( obj.translator() );
    setName( obj.name() );
    setDirName( obj.dirName() );
    pars_ = obj.pars_;
    delete dskey_;
    dskey_ = obj.dskey_ ? new OD::DataSetKey( *obj.dskey_ ) : nullptr;
}


void IOObj::copyFrom( const IOObj* obj )
{
    if ( !obj )
	return;

    copyStuffFrom( *obj );
}


IOObj* IOObj::get( ascistream& astream, const char* dirnm, int groupid )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return nullptr;
    if ( *astream.keyWord() == '@' )
	return IOSubDir::get( astream, dirnm );

    const BufferString nm( astream.keyWord() );
    FileMultiString fms = astream.value();
    const MultiID objkey( groupid, fms.getIValue(0) );
    PtrMan<DBKey> objky;
    if ( groupid == MultiID::udf().groupID() )
    {
	const SurveyDiskLocation sdl( nullptr, dirnm );
	objky = new DBKey( objkey, sdl );
    }
    else
    {
	FilePath dirfp( dirnm );
	if ( groupid > 0 )
	    dirfp.setFileName( nullptr );

	const SurveyDiskLocation sdl( dirfp.fileName(), dirfp.pathOnly() );
	objky = new DBKey( objkey, sdl );
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
	    return nullptr;

	Translator* tr = grp.make( trlnm, true );
	if ( !tr )
	    return nullptr;

	objtyp = tr->connType();
	delete tr;
    }

    IOObj* objptr = produce( objtyp, nm, *objky.ptr(), false );
    if ( !objptr )
	return nullptr;

    objptr->setGroup( groupnm );
    objptr->setTranslator( trlnm );

    astream.next();
    if ( *astream.keyWord() != '$' )
	deleteAndNullPtr( objptr );
    else
    {
	if ( !objptr->getFrom(astream) || objptr->isBad() )
	    deleteAndNullPtr( objptr );
	else
	{
	    while ( *astream.keyWord() == '#' )
	    {
		objptr->pars_.set( astream.keyWord()+1, astream.value() );
		astream.next();
	    }

	    if ( !objptr->pars_.isEmpty() )
	    {
		const OD::DataSetKey dsky =
				OD::DataSetKey::getFrom( objptr->pars_ );
		if ( !dsky.isUdf() )
		    objptr->setDSKey( dsky );
	    }
	}
    }

    while ( !atEndOfSection(astream) )
	astream.next();

    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const DBKey& keyin,
			bool gendef )
{
    if ( !nm || !*nm )
	nm = "?";

    if ( keyin.isUdf() )
    {
	pFreeFnErrMsg( "IOObj : Empty key given");
	return nullptr;
    }

    for ( const auto* prod : getProducers() )
	if ( prod->canMake(typ) )
	    return prod->make( nm, keyin, gendef );

    return nullptr;
}


Translator* IOObj::createTranslator() const
{
    if ( isSubdir() )
	return nullptr;

    TranslatorGroup& grp = TranslatorGroup::getGroup( group() );
    if ( grp.groupName() != group() )
	return nullptr;

    Translator* tr = grp.make( translator(), true );
    if ( !tr )
	return nullptr;

    if ( pars_.size() )
	tr->usePar( pars_ );

    return tr;
}


IOObj* IOObj::clone() const
{
    if ( isSubdir() )
	return new IOSubDir( *((IOSubDir*)this) );

    if ( key().isUdf() )
	return nullptr;

    IOObj* ret = produce( connType(), name(), key(), false );
    if ( !ret )
	{ pErrMsg("Cannot 'produce' IOObj of my own type"); return nullptr; }

    ret->copyFrom( this );
    return ret;
}



bool IOObj::hasDSKey() const
{
    return dskey_ && !dskey_->isUdf();
}


void IOObj::setDSKey( const OD::DataSetKey& dskey )
{
    if ( dskey_ && *dskey_ != dskey )
    {
	pErrMsg("DataSetKey should never be reassigned");
	return;
    }

    deleteAndNullPtr( dskey_ );
    if ( dskey.isUdf() )
    {
	dskey.fillPar( pars_ );
	return;
    }

    dskey_ = new OD::DataSetKey( dskey );
    dskey_->fillPar( pars_ );
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
	FileMultiString fms = translator();
	fms += connType();
	astream.put( group(), fms );
    }

    if ( !putTo(astream) )
	return false;

    BufferStringSet sortedkeys;
    pars_.getKeys( sortedkeys );
    sortedkeys.sort();
    for ( const auto* key : sortedkeys )
    {
	astream.stream() << '#';
	const char* keystr = key->buf();
	astream.put( keystr, pars_.find(keystr).buf() );
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


int IOObj::nrImpls() const
{
    PtrMan<Translator> tr = createTranslator();
    return tr->nrImpls( this );
}


void IOObj::implFileNames( BufferStringSet& fnames ) const
{
    PtrMan<Translator> tr = createTranslator();
    return tr->implFileNames( this, fnames );
}


void IOObj::setSurveyDefault() const
{
    PtrMan<Translator> tr = createTranslator();
    if ( !tr )
	return;

    const BufferString defaultkey( tr->group()->getSurveyDefaultKey( this ) );

    SI().getPars().set( defaultkey.buf(), key() );
    SI().savePars();
}


bool IOObj::isSurveyDefault( const MultiID& ky )
{
    IOPar* dpar = SI().pars().subselect( sKey::Default() );
    bool ret = false;
    if ( dpar && !dpar->isEmpty() )
	ret = !dpar->findKeyFor( ky.toString() ).isEmpty();

    delete dpar;
    return ret;
}


#define mQuotedName toUiString(name()).quote(true)

uiString IOObj::phrCannotOpenObj() const
{
    return uiStrings::phrCannotOpen( toUiString( name() ).quote( true ) );
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
    FilePath cursurvfp( IOM().rootDir().fullPath() ); cursurvfp.makeCanonical();
    FilePath orgfp( fullUserExpr(true) ); orgfp.makeCanonical();
    return orgfp.isSubDirOf( cursurvfp );
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


// IOSubDir

IOSubDir::IOSubDir( const char* subdirnm )
    : IOObj(subdirnm,DBKey(MultiID::udf()))
    , isbad_(false)
{
}


IOSubDir::IOSubDir( const IOSubDir& oth )
    : IOObj(oth)
    , isbad_(oth.isbad_)
{
}


IOSubDir::~IOSubDir()
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
    if ( isBad() )
	return false;

    const BufferString str( "@", myKey() );
    stream.put( str, name() );
    return true;
}


// IOX

IOX::IOX( const char* nm, const DBKey& ky, bool /* mkdefs */ )
    : IOObj(nm,ky)
{
}


IOX::IOX( const char* nm, const MultiID& ky, bool mkdefs )
    : IOX(nm,DBKey(ky),mkdefs)
{
}


mStartAllowDeprecatedSection

IOX::IOX( const char* nm, const char* ky, bool mkdefs )
    : IOX(nm,DBKey(MultiID(ky)),mkdefs)
{
}

mStopAllowDeprecatedSection


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
    if ( !obj )
	return;

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOX*,trobj,obj)
    if ( trobj )
	ownkey_ = trobj->ownkey_;
}


const char* IOX::fullUserExpr( bool forread ) const
{
    PtrMan<IOObj> ioobj = IOM().get( ownkey_ );
    if ( !ioobj )
	return "<invalid>";

    const char* s = ioobj->fullUserExpr( forread );
    return s;
}


bool IOX::implExists( bool forread ) const
{
    PtrMan<IOObj> ioobj = IOM().get( ownkey_ );
    if ( !ioobj )
	return false;

    const bool yn = ioobj->implExists( forread );
    return yn;
}


Conn* IOX::getConn( bool forread ) const
{
    PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj )
	return nullptr;

    auto* ret = new XConn;
    ret->conn_ = ioobj->getConn( forread );
    ret->setLinkedTo( key() );

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
    if ( !ioobj )
	return dirnm_.buf();

    getNonConst(*this).dirnm_ = ioobj->dirName();
    delete ioobj;
    return dirnm_.buf();
}
