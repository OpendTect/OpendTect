#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		April 2017
________________________________________________________________________

-*/

#include "crsmod.h"
#include "latlong.h"
#include "manobjectset.h"
#include "uistring.h"

class BufferStringSet;

namespace Coords
{

typedef int ProjectionID;

mExpClass(CRS) AuthorityCode
{
public:
			AuthorityCode(const char* auth,ProjectionID i)
			    : authority_(auth),id_(i) {}
			AuthorityCode(const AuthorityCode& oth)
			    : authority_(oth.authority_),id_(oth.id_) {}

    BufferString	authority() const { return authority_; }
    ProjectionID	id() const	  { return id_; }

    bool		operator ==(const AuthorityCode&) const;

    static AuthorityCode	fromString(const char*);
    BufferString	toString() const;

protected:

    BufferString	authority_;
    ProjectionID	id_;
};


mExpClass(CRS) Projection
{ mODTextTranslationClass(Projection);
public:

				Projection(AuthorityCode,const char* usernm,
					   const char* defstr);
				~Projection();

    AuthorityCode		authCode() const	{ return authcode_; }
    BufferString		userName() const	{ return usernm_; }
    BufferString		defStr() const		{ return defstr_; }

    virtual bool		isOK() const;
    virtual bool		getReady() const	{ return true; }

    virtual bool		isOrthogonal() const;
    virtual bool		isLatLong() const	{ return false; }
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    static void			getAll( TypeSet<AuthorityCode>&,
					BufferStringSet& names,
					BufferStringSet& defstrs,
					bool orthogonalonly=false );
    static const Projection*	getByAuthCode(AuthorityCode);
    static const Projection*	getByName(const char*);
    static BufferString		getInfoText(const char* defstr);

protected:

    virtual LatLong		toGeographic(const Coord&,
					     bool wgs84=false) const;
    virtual Coord		fromGeographic(const LatLong&,
					       bool wgs84=false) const;

    BufferString		defstr_;

private:

    virtual Coord		transformTo(const Projection& target,
					    LatLong) const;
    virtual LatLong		transformTo(const Projection& target,
					    Coord) const;

    AuthorityCode		authcode_;
    BufferString		usernm_;

    friend class ProjectionBasedSystem;
};


mExpClass(CRS) ProjectionRepos : public ManagedObjectSet<Projection>
{ mODTextTranslationClass(ProjectionRepos);
public:

				ProjectionRepos(const char* key,
						uiString desc);

    const char*			key() const		{ return key_.buf(); }
    uiString			description() const	{ return desc_; }

    bool			readFromFile(const char* fnm);

    const Projection*		getByAuthCode(AuthorityCode) const;
    const Projection*		getByName(const char*) const;

    static void					addRepos(ProjectionRepos*);
    static const ProjectionRepos*		getRepos(const char* key);
    static const ObjectSet<ProjectionRepos>&	reposSet()
						{ return reposset_; }
    static void					getAuthKeys(BufferStringSet&);

protected:

    BufferString		key_;
    uiString			desc_;

    static ObjectSet<ProjectionRepos>	reposset_;

};

}; //namespace
