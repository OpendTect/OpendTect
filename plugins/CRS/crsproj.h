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

#include "projects.h"
#include "proj_api.h"


namespace Coords
{

mDefIntegerIDType(od_uint16,ProjectionID);

mExpClass(CRS) Projection
{ mODTextTranslationClass(Projection);
public:

    				Projection(ProjectionID id,const char* usernm,
					   const char* defstr);
				~Projection();

    ProjectionID		id() const		{ return id_; }
    BufferString		userName() const	{ return usernm_; }
    BufferString		defStr() const		{ return defstr_; }    

    bool			isOK() const;

    LatLong			toGeographicWGS84(const Coord&) const;
    Coord			fromGeographicWGS84(const LatLong&) const;

    bool			isFeet() const;
    bool			isMeter() const;

    static const Projection*	getByID(ProjectionID);
    static const Projection*	getByName(const char*);

    static Coord		transform(const Projection& from,
	    				  const Projection& to,
					  Coord pos);

protected:

    void			init();

    ProjectionID		id_;
    BufferString		usernm_;
    BufferString		defstr_;
    projPJ			proj_;
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
