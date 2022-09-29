/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "transl.h"

#include "ctxtioobj.h"
#include "debug.h"
#include "file.h"
#include "filepath.h"
#include "stringview.h"
#include "genc.h"
#include "iostrm.h"
#include "iopar.h"
#include "keystrs.h"
#include "perthreadrepos.h"
#include "preloads.h"
#include "streamconn.h"
#include "strmprov.h"
#include "uistrings.h"


uiString Translator::sNoIoobjMsg()
{ return toUiString("Internal error: No object to store set in data base."); }

uiString Translator::sBadConnection()
{ return toUiString("Internal error: bad connection"); }


uiString Translator::sSelObjectIsWrongType()
{
    return tr("Selected object is not a %1").arg( userName() );
}



TranslatorGroup::TranslatorGroup( const char* clssnm )
    : clssname_(clssnm)
{
}


int defaultSelector( const char* m, const char* typ )
{
    StringView mytyp( m );
    return mytyp==typ ? mObjSelMatch : mObjSelUnrelated;
}


TranslatorGroup::~TranslatorGroup()
{
    delete selhist_;
    for ( int idx=0; idx<templs_.size(); idx++ )
	delete const_cast<Translator*>( templs_[idx] );

    while ( getGroups().isPresent(this) )
	getGroups() -= this;
}


ObjectSet<TranslatorGroup>& TranslatorGroup::getGroups()
{
    mDefineStaticLocalObject( ObjectSet<TranslatorGroup>, allgrps, );
    return allgrps;
}


bool TranslatorGroup::add( Translator* tr )
{
    if ( !tr )
	return false;

    bool res = false;

    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	const Translator* oldtr = templs_[idx];
	if ( oldtr->userName() == tr->userName() )
	{
	    if ( DBG::isOn(DBG_IO) )
	    {
		BufferString msg( "Translator already in list: '" );
		msg += tr->userName();
		msg += "' - replacing old.";
		DBG::message( msg );
	    }

	    res = true;
	    templs_ -= oldtr;
	    break;
	}
    }

    tr->setGroup( this );
    templs_ += tr;

    return res;
}


class EmptyTrGroup : public TranslatorGroup
{
public:
EmptyTrGroup()
    : TranslatorGroup(""), ctxt(0,"")
{}

const IOObjContext& ioCtxt() const override
{
    return ctxt;
}

int objSelector( const char* ) const override
{
    return mObjSelUnrelated;
}

StringView groupName() const override
{
    return StringView::empty();
}

uiString typeName(int) const override
{
    return uiString::emptyString();
}

   IOObjContext ctxt;

};


static TranslatorGroup* findGroup( const ObjectSet<TranslatorGroup>& grps,
				   const char* nm, bool iserr )
{
    if ( !nm || !*nm )
	return nullptr;

    for ( int idx=0; idx<grps.size(); idx++ )
    {
	if ( grps[idx]->groupName() == nm )
	    return const_cast<TranslatorGroup*>( grps[idx] );
    }

    if ( iserr )
    {
	const StringView fsspace = firstOcc( nm, ' ' );
	if ( fsspace.startsWith(" directory",OD::CaseInsensitive) )
	    return nullptr;
    }

    return nullptr;
}

static PtrMan<EmptyTrGroup>* emptytrgroup = nullptr;

static void deleteEmptyGroup()
{
    if ( emptytrgroup )
	emptytrgroup->erase();
}

PtrMan<EmptyTrGroup>& emptyTrGroupMgr_()
{
    static PtrMan<EmptyTrGroup> ret = nullptr;
    if ( !ret )
    {
	auto* newemptygroup = new EmptyTrGroup;
	if ( ret.setIfNull(newemptygroup,true) )
	    NotifyExitProgram( &deleteEmptyGroup );
    }
    return ret;
}

EmptyTrGroup& emptyTrGroup()
{
    if ( !emptytrgroup )
	emptytrgroup = &emptyTrGroupMgr_();
    return *(*emptytrgroup).ptr();
}


TranslatorGroup& TranslatorGroup::getGroup( const char* nm )
{
    TranslatorGroup* ret = findGroup( groups(), nm, true );
    if ( !ret )
	return emptyTrGroup();

    return *ret;
}


bool TranslatorGroup::hasGroup( const char* nm )
{
    return findGroup( groups(), nm, false );
}


void TranslatorGroup::clearSelHists()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    for ( int idx=0; idx<grps.size(); idx++ )
	const_cast<TranslatorGroup*>(grps[idx])->clearSelHist();
}


TranslatorGroup& TranslatorGroup::addGroup( TranslatorGroup* newgrp )
{
    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Translator group to be added: '" );
	if ( !newgrp )
	    msg += "(null)' . Major problem!";
	else
	{
	    msg += newgrp->groupName();
	    msg += "' = class ";
	    msg += newgrp->clssName();
	}
	DBG::message( msg );
    }

    if ( !newgrp )
	return emptyTrGroup();

    TranslatorGroup* grp = findGroup( getGroups(), newgrp->groupName(), false );
    if ( grp )
    {
	if ( DBG::isOn(DBG_IO) )
	{
	    BufferString msg( "Not adding '" ); msg += newgrp->groupName();
	    msg += "' again";
	    DBG::message( msg );
	}

	newgrp->ref(); newgrp->unRef();

	return *grp;
    }

    getGroups() += newgrp;
    return *newgrp;
}


bool TranslatorGroup::hasConnType( const char* c ) const
{
    StringView ct( c );
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( ct == templs_[idx]->connType() )
	    return true;
    }

    return false;
}


static const OD::String& gtNm( const Translator* tr, bool user )
{
    return user ? tr->userName() : tr->typeName();
}


Translator* TranslatorGroup::make( const char* nm, bool usr ) const
{
    const Translator* tr = getTemplate( nm, usr );
    return tr ? tr->getNew() : 0;
}


const Translator* TranslatorGroup::getTemplate( const char* nm, bool usr ) const
{
    if ( !nm || !*nm )
	return nullptr;

    // Direct match is OK - just return it
    const Translator* tr = nullptr;
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( gtNm(templs_[idx],usr) == nm )
	    return templs_[idx];
    }

    // Now try to match only given string - may be part of full name
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( gtNm(templs_[idx],usr).startsWith(nm) )
	{
	    if ( tr ) // more than one match
		return nullptr;

	    tr = templs_[idx];
	}
    }

    return tr;
}


const char* TranslatorGroup::getSurveyDefaultKey(const IOObj*) const
{
    return IOPar::compKey( sKey::Default(), groupName() );
}

const char* TranslatorGroup::translationApplication() const
{
    return uiString::sODLocalizationApplication();
}


extern "C" void od_Basic_initStdClasses();

Translator::Translator( const char* nm, const char* unm )
    : typname_(nm)
    , usrname_(unm)
{
    mDefineStaticLocalObject( bool, init_done, = false );
    if ( !init_done )
    {
	od_Basic_initStdClasses();
	init_done = true;
    }
}


const char* Translator::connType() const
{
    return StreamConn::sType();
}


const char* Translator::defExtension() const
{
    return group_ ? group_->defExtension() : nullptr;
}


IOObj* Translator::createWriteIOObj( const IOObjContext& ctxt,
				     const MultiID& ky ) const
{
    return ctxt.crDefaultWriteObj( *this, ky );
}


const char* Translator::getDisplayName() const
{
    mDeclStaticString( ret );
    ret.set( userName() ).add( " [" ).add( group()->groupName() ).add( "]" );
    return ret.str();
}


const Translator* Translator::getTemplateInstance( const char* displayname )
{
    BufferString trnm( displayname );
    char* grpptr = trnm.find( ' ' );
    if ( !grpptr || !*(grpptr+1) )
	return nullptr;

    *grpptr = '\0';
    BufferString grpnm( grpptr+1 );
    grpnm.unEmbed( '[', ']' );
    trnm.trimBlanks(); grpnm.trimBlanks();
    if ( trnm.isEmpty() || grpnm.isEmpty() )
	return nullptr;

    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    const TranslatorGroup* trgrp = 0;
    for ( int idx=0; idx<grps.size(); idx++ )
    {
	if ( grpnm == grps[idx]->groupName() )
	{
	    trgrp = grps[idx];
	    break;
	}
    }

    if ( !trgrp )
	return nullptr;

    const ObjectSet<const Translator>& tpls = trgrp->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
	if ( trnm == tpls[idx]->userName() )
	    return tpls[idx];

    return nullptr;
}


IOPar& TranslatorGroup::selHist()
{
    if ( !selhist_ )
    {
	BufferString parnm = groupName();
	parnm += " selection history";
	selhist_ = new IOPar( parnm );
    }

    return *selhist_;
}


void TranslatorGroup::clearSelHist()
{
    if ( selhist_ )
	selhist_->setEmpty();
}


bool Translator::implIsLink( const IOObj* ioobj ) const
{
    return ioobj && ioobj->implIsLink();
}


bool Translator::implExists( const IOObj* ioobj, bool forread ) const
{
    return ioobj && ioobj->implExists( forread );
}


bool Translator::implReadOnly( const IOObj* ioobj ) const
{
    return ioobj && ioobj->implReadOnly();
}


bool Translator::implRemove( const IOObj* ioobj, bool deep ) const
{
    return ioobj && ioobj->implRemove();
}


bool Translator::implRename( const IOObj* ioobj, const char* newnm ) const
{
    if ( !ioobj )
	return false;

    PtrMan<IOObj> ioobjcopy = ioobj->clone();
    mDynamicCastGet(IOStream*,iostrm,ioobjcopy.ptr())
    if ( !iostrm )
	return true;

    if ( !iostrm->implExists(true) )
    {
	iostrm->genFileName();
	return true;
    }

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    FilePath fp( iostrm->fileSpec().fileName() );
    chiostrm.setExt( defExtension() );

    BufferString cleannm( chiostrm.name() );
    cleannm.clean( BufferString::NoFileSeps );
    chiostrm.setName( cleannm );
    chiostrm.genFileName();
    chiostrm.setName( newnm );

    FilePath deffp( chiostrm.fileSpec().fileName() );
    fp.setFileName( deffp.fileName() );
    chiostrm.fileSpec().setFileName( fp.fullPath() );

    const bool newfnm = StringView(chiostrm.fileSpec().fileName())
				    != iostrm->fileSpec().fileName();
    if ( newfnm && !doReloc(*iostrm,chiostrm) )
    {
	cleannm = newnm;
	if ( !cleannm.contains('/') && !cleannm.contains('\\') )
	    return false;

	cleannm.clean( BufferString::AllowDots );
	chiostrm.setName( cleannm );
	chiostrm.genFileName();
	deffp.set( chiostrm.fileSpec().fileName() );
	fp.setFileName( deffp.fileName() );
	chiostrm.fileSpec().setFileName( fp.fullPath() );
	chiostrm.setName( iostrm->name() );
	if (!doReloc(*iostrm,chiostrm))
	    return false;
    }

    iostrm->copyFrom( &chiostrm );
    return true;
}


bool Translator::implReloc( const IOObj* ioobj, const char* newdir ) const
{
    if ( !ioobj )
	return false;

    PtrMan<IOObj> ioobjcopy = ioobj->clone();
    mDynamicCastGet(IOStream*,iostrm,ioobjcopy.ptr())
    BufferString oldfnm( iostrm->fullUserExpr() );
    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    if ( !File::isDirectory(newdir) )
	return false;

    FilePath fp( oldfnm );
    fp.setPath( newdir );
    chiostrm.fileSpec().setFileName( fp.fullPath() );
    return doReloc( *iostrm,chiostrm );
}


bool Translator::doReloc( IOStream& iostrm, IOStream& chiostrm ) const
{
    const BufferString newfname( chiostrm.fullUserExpr() );
    bool succeeded = true;
    if ( implExists(&iostrm,true) )
    {
	if ( implExists(&chiostrm,true) && !implRemove(&iostrm) )
	    return false;

	succeeded = implRename( &iostrm, newfname );
    }

    if ( succeeded )
	iostrm.fileSpec().setFileName( newfname );

    return succeeded;
}


bool Translator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    return ioobj && ioobj->implSetReadOnly( yn );
}


bool Translator::getConfirmRemoveMsg( const IOObj* ioobj, uiString& msg,
		uiString& canceltxt, uiString& yestxt, uiString& notxt ) const
{
    if ( !ioobj || !ioobj->implExists(true) )
	return false;

    msg = tr("Database entry for %1 '%2' will be permanently removed "
	    "along with the associated data.")
	    .arg(userName()).arg(ioobj->name());
    canceltxt = uiStrings::sCancel();
    yestxt = uiStrings::sRemove();
    notxt = uiString::empty();
    return true;
}


BufferString Translator::getAssociatedFileName( const char* fnm,
						const char* ext )
{
    FilePath fp( fnm );
    fp.setExtension( ext );
    return fp.fullPath();
}


BufferString Translator::getAssociatedFileName( const IOObj& ioobj,
						const char* ext )
{
    return getAssociatedFileName( ioobj.mainFileName(), ext );
}


bool Translator::renameAssociatedFile( const char* fnm, const char* ext,
					const char* newnm )
{
    if ( !newnm || !*newnm )
	return false;

    const BufferString assocoldfnm( getAssociatedFileName(fnm,ext) );
    if ( !File::exists(assocoldfnm) )
	return true;

    FilePath fpnew( newnm );
    fpnew.setExtension( ext );
    if ( !fpnew.isAbsolute() )
	fpnew.setPath( FilePath(assocoldfnm).pathOnly() );
    const BufferString assocnewfnm( fpnew.fullPath() );
    return File::rename( assocoldfnm, assocnewfnm );
}


bool Translator::renameLargeFile( const char* fnm, const char* newnm,
				  const CallBack* cb )
{
    if ( !fnm || !*fnm || !newnm || !*newnm || !File::exists(fnm) )
	return false;

    const BufferString orgdir = FilePath(fnm).pathOnly();
    StreamProvider sp( fnm );
    StreamProvider spnew( newnm );
    spnew.addPathIfNecessary( FilePath(fnm).pathOnly() );
    return sp.rename( spnew.fileName(), cb );
}


bool Translator::removeAssociatedFile( const char* fnm, const char* ext )
{
    const BufferString assosfnm( getAssociatedFileName(fnm,ext) );
    if ( !File::exists(assosfnm) )
	return true;

    return File::remove( assosfnm );
}


bool Translator::setPermAssociatedFile( const char* fnm, const char* ext,
			bool setwritable )
{
    const BufferString assosfnm( getAssociatedFileName(fnm,ext) );
    if ( !File::exists(assosfnm) )
	return true;

    return File::makeWritable( assosfnm, setwritable, false );
}
