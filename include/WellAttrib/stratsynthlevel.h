#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "stratlevel.h"
#include "typeset.h"


namespace StratSynth
{

mExpClass(WellAttrib) Level : public ObjectWithName
{
public:

    typedef Strat::LevelID ID;

			Level( ID lvlid )
			  : id_(lvlid)		{}

    int			size() const		{ return zvals_.size(); }
    ID			id() const		{ return id_; }
    const name_type&	name() const override;
    OD::Color		color() const;

    const ID		id_;
    TypeSet<float>	zvals_; //!< one for each model/synthetic

    static const Level& undef();
    static Level&	dummy();

};


mExpClass(WellAttrib) LevelSet
{
public:

    typedef Strat::LevelID ID;

			LevelSet()			{}
			LevelSet( const LevelSet& oth ) { *this = oth; }
			~LevelSet()			{ setEmpty(); }
    LevelSet&		operator =(const LevelSet&);

    bool		isEmpty() const		{ return lvls_.isEmpty(); }
    void		setEmpty()		{ deepErase( lvls_ ); }

    Level&		add(ID); //!< if ID already present returns that one

    int			size() const			{ return lvls_.size(); }
    int			indexOf(ID) const;
    int			indexOf(const char*) const;
    bool		isPresent( ID id ) const
			{ return indexOf(id) >= 0; }
    bool		isPresent( const char* nm ) const
			{ return indexOf(nm) >= 0; }

    Level&		getByIdx( int idx )		{ return *lvls_[idx]; }
    const Level&	getByIdx( int idx ) const	{ return *lvls_[idx]; }
    Level&		get(ID);
    const Level&	get(ID) const;
    Level&		getByName(const char*);
    const Level&	getByName(const char*) const;

    const ObjectSet<Level>& levels() const		{ return lvls_; }


private:

    ObjectSet<Level>	lvls_;

};

} // namespace StratSynth
