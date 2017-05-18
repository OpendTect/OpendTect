#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		April 2017
________________________________________________________________________

-*/

#include "crsmod.h"
#include "integerid.h"
#include "latlong.h"
#include "manobjectset.h"
#include "uistring.h"

class BufferStringSet;

namespace Coords
{

mDefIntegerIDType(int,ProjectionID);

mExpClass(CRS) Projection
{ mODTextTranslationClass(Projection);
public:

				Projection(ProjectionID id,const char* usernm,
					   const char* defstr);
				~Projection();

    ProjectionID		id() const		{ return id_; }
    BufferString		userName() const	{ return usernm_; }
    BufferString		defStr() const		{ return defstr_; }

    virtual bool		isOK() const;

    virtual bool		isOrthogonal() const;
    virtual bool		isLatLong() const	{ return false; }
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    static void			getAll( TypeSet<ProjectionID>&,
					BufferStringSet& names,
					bool orthogonalonly=false );
    static const Projection*	getByID(ProjectionID);
    static const Projection*	getByName(const char*);

protected:

    void			init();
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

    ProjectionID		id_;
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

    const Projection*		getByID(ProjectionID) const;
    const Projection*		getByName(const char*) const;

    static void					addRepos(ProjectionRepos*);
    static const ProjectionRepos*		getRepos(const char* key);
    static const ObjectSet<ProjectionRepos>&	reposSet()
						{ return reposset_; }

protected:

    BufferString		key_;
    uiString			desc_;

    static ObjectSet<ProjectionRepos>	reposset_;

};

}; //namespace
