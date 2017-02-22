/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "transl.h"
#include "preloads.h"
#include "streamconn.h"
#include "ctxtioobj.h"
#include "fixedstring.h"
#include "iostrm.h"
#include "iopar.h"
#include "debug.h"
#include "keystrs.h"
#include "perthreadrepos.h"

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
    , selhist_(0)
    , deftridx_(0)
{
}


int defaultSelector( const char* m, const char* typ )
{
    FixedString mytyp( m );
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
    if ( !tr ) return false;

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
		    templs_ -= oldtr; break;

	}
    }

    tr->setGroup( this );
    templs_ += tr;

    return res;
}


class EmptyTrGroup : public TranslatorGroup
{
public:

    EmptyTrGroup() : TranslatorGroup(""), ctxt(0,"")	{}
    const IOObjContext& ioCtxt() const		{ return ctxt; }
    int objSelector( const char* ) const	{ return mObjSelUnrelated; }
    FixedString groupName() const	{ return FixedString::empty(); }
    uiString typeName(int) const	{ return uiString::emptyString(); }

   IOObjContext ctxt;

};


static TranslatorGroup* findGroup( const ObjectSet<TranslatorGroup>& grps,
				   const char* nm, bool iserr )
{
    if ( !nm || !*nm )
	return 0;

    for ( int idx=0; idx<grps.size(); idx++ )
    {
	if ( grps[idx]->groupName() == nm )
	    return const_cast<TranslatorGroup*>( grps[idx] );
    }

    if ( iserr )
    {
	const FixedString fsspace = firstOcc( nm, ' ' );
	if ( fsspace.startsWith(" directory",CaseInsensitive) )
	    return 0;
    }
    return 0;
}

static EmptyTrGroup* emptytrgroup = 0;
#define mRetEG \
{ if ( !emptytrgroup ) emptytrgroup = new EmptyTrGroup; return *emptytrgroup; }

TranslatorGroup& TranslatorGroup::getGroup( const char* nm )
{
    TranslatorGroup* ret = findGroup( groups(), nm, true );
    if ( !ret ) mRetEG
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

    if ( !newgrp ) mRetEG

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
    FixedString ct( c );
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
    if ( !nm || !*nm ) return 0;

    // Direct match is OK - just return it
    const Translator* tr = 0;
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
		return 0;
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
{ return uiString::sODLocalizationApplication(); }


extern "C" void od_Basic_initStdClasses();

Translator::Translator( const char* nm, const char* unm )
    : typname_(nm)
    , usrname_(unm)
    , group_(0)
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
	return 0;

    *grpptr = '\0';
    BufferString grpnm( grpptr+1 );
    grpnm.unEmbed( '[', ']' );
    trnm.trimBlanks(); grpnm.trimBlanks();
    if ( trnm.isEmpty() || grpnm.isEmpty() )
	return 0;

    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    const TranslatorGroup* trgrp = 0;
    for ( int idx=0; idx<grps.size(); idx++ )
	if ( grpnm == grps[idx]->groupName() )
	    { trgrp = grps[idx]; break; }
    if ( !trgrp )
	return 0;

    const ObjectSet<const Translator>& tpls = trgrp->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
	if ( trnm == tpls[idx]->userName() )
	    return tpls[idx];

    return 0;
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
    if ( selhist_ ) selhist_->setEmpty();
}


bool Translator::implExists( const IOObj* ioobj, bool forread ) const
{
    if ( !ioobj ) return false;
    return ioobj->implExists( forread );
}


bool Translator::implReadOnly( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    return ioobj->implReadOnly();
}


bool Translator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    return ioobj->implRemove();
}


bool Translator::implManagesObjects( const IOObj* ioobj ) const
{
    return ioobj ? ioobj->implManagesObjects() : false;
}


bool Translator::implRename( const IOObj* ioobj, const char* newnm,
			     const CallBack* cb ) const
{
    if ( !ioobj ) return false;
    return const_cast<IOObj*>(ioobj)->implRename( newnm, cb );
}


bool Translator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    if ( !ioobj ) return false;
    return ioobj->implSetReadOnly( yn );
}
