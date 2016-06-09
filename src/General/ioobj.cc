/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/


#include "iox.h"
#include "iosubdir.h"
#include "ioman.h"
#include "iopar.h"
#include "iodir.h"
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
#include "perthreadrepos.h"

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
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
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



IOObj* IOObj::get( ascistream& astream, const char* dirnm, const char* dirky,
       		   bool rejectoldtmps )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    if ( *astream.keyWord() == '@' )
	return IOSubDir::get( astream, dirnm );

    BufferString nm( astream.keyWord() );
    FileMultiString fms = astream.value();
    const int dirid = toInt( dirky );
    const int leafid = fms.getIValue( 0 );
    MultiID objkey( dirid, leafid );
    bool reject = dirid < 0 || leafid < 1;
    if ( rejectoldtmps && isTmpLeafID(leafid) )
    {
	const int dikey = fms.getIValue( 1 );
	if ( dikey < DateInfo().key()-1 )
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


IOObj* IOObj::produce( const char* typ, const char* nm, const MultiID& ky,
			bool gendef )
{
    if ( !typ || !*typ )
	return 0;

    if ( !nm || !*nm )
	nm = "?";
    if ( ky.isEmpty() )
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

    if ( key().isEmpty() )
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




bool IOObj::isKey( const char* ky )
{ return IOM().isKey(ky); }


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
	    astream.put( name(), leafID() );
	else
	{
	    fms.set( leafID() );
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
    if ( leafID() < 2 || isSubdir() || isTmp() )
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
    if ( ky.isEmpty() )
	ky.set( "0" );
    else if ( ky.leafID() == 1 )
	ky.set( ky.upLevel() );
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
    mDeclStaticString( ret );
    ret = FilePath(dirnm_,name()).fullPath();
    return ret;
}


bool IOSubDir::putTo( ascostream& stream ) const
{
    if ( isBad() ) return false;
    const BufferString str( "@", leafID() );
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
    return ownkey_ == "";
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
    if ( !ioobj ) return "<invalid>";
    const char* s = ioobj->fullUserExpr(i);
    delete ioobj;
    return s;
}


bool IOX::implExists( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return false;
    bool yn = ioobj->implExists(i);
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
    return ownkey_ == "" ? 0 : IOM().get( ownkey_ );
}


bool IOX::getFrom( ascistream& stream )
{
    ownkey_ = stream.value();
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
