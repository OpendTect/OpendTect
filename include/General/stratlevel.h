#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
________________________________________________________________________

-*/

#include "generalmod.h"
#include "undefval.h"
#include "namedobj.h"
#include "ranges.h"
#include "color.h"
#include "repos.h"
#include "objectset.h"
class ascistream;
class BufferStringSet;


namespace Strat
{
class LevelSet;


/*!\brief Stratigraphic level

  To store extra details, use the pars_.

  The ID cannot be assigned, and it is unique. It is managed by the LevelSet.
  Therefore:
  * operator ==() : checks only ID
  * isDifferentFrom() : checks all but ID

*/

mExpClass(General) Level : public NamedCallBacker
{
public:

    typedef int		ID;

    bool		operator ==(const Level&) const;
    bool		isDifferentFrom(const Level&) const;

    ID			id() const		{ return id_; }
    OD::Color		color() const		{ return color_; }
    const IOPar&	pars() const		{ return pars_; }

    void		setName(const char*);
    void		setColor(OD::Color);
    void		setPars(const IOPar&);

    Notifier<Level>	changed;
    Notifier<Level>	toBeRemoved;

    static const Level&	undef();

protected:

			Level(const char* nm,const LevelSet*);
			Level(const Level&);

    ID			id_;
    OD::Color		color_;
    IOPar&		pars_;
    const LevelSet*	lvlset_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    friend class	LevelSet;

public:

    virtual		~Level();

    const char*		checkName(const char*) const;
			//!< returns null if object can be renamed to it,
			//!< otherwise an error message

};


/*!\brief Set of Stratigraphic levels

  Manages not only the units, but also their IDs. The set will not allow
  adding levels with the same name; the result is that you simply set the
  existing one to new values.

*/


mExpClass(General) LevelSet : public CallBacker
{
public:

			LevelSet();
			LevelSet(Level::ID startat);
			LevelSet(const LevelSet&);
    virtual		~LevelSet();
    LevelSet&		operator =(const LevelSet&);

    bool		isEmpty() const		{ return lvls_.isEmpty(); }
    int			size() const		{ return lvls_.size(); }
    const ObjectSet<Level>& levels() const	{ return lvls_; }
    void		setEmpty()		{ lvls_.erase(); }

    inline bool	isPresent( const char* nm ) const
			{ return gtIdxOf(nm,-1) >= 0; }
    inline bool	isPresent( Level::ID id ) const
			{ return gtIdxOf(0,id) >= 0; }
    inline int		indexOf( const char* nm ) const
			{ return gtIdxOf(nm,-1); }
    inline int		indexOf( Level::ID id ) const
			{ return gtIdxOf(0,id); }

    Level*		get( const char* nm )		{ return gtLvl(nm,-1); }
    const Level*	get( const char* nm ) const	{ return gtLvl(nm,-1); }
    Level*		get( Level::ID id )		{ return gtLvl(0,id); }
    const Level*	get( Level::ID id ) const	{ return gtLvl(0,id); }

    Level*		add( const char* lvlnm, const OD::Color& c )
						{ return set(lvlnm,c,-1); }
    Level*		insert(
			    const char* lvlnm, const OD::Color& c , int idx=-1 )
						{ return set(lvlnm,c,idx); }
    Level*		set( const char* lvlnm, const OD::Color& c )
						{ return set(lvlnm,c,-1); }
    void		remove(Level::ID);

    Level*		add(const Level&); //!< copy stuff, but new ID/name
    void		add(const BufferStringSet&,const TypeSet<OD::Color>&);

    Notifier<LevelSet>	levelAdded;
    Notifier<LevelSet>	levelChanged;
    Notifier<LevelSet>	levelToBeRemoved;
    int			notifLvlIdx() const	{ return notiflvlidx_; }
			//!< if < 0 then more than one level have changed

    bool		readFrom(const char*);
    bool		writeTo(const char*) const;
    bool		needStore() const		{ return ischanged_; }

    static void		getStdNames(BufferStringSet&);
    static LevelSet*	createStd(const char*);
    static LevelSet*	read(const MultiID&);
    static bool		write(const LevelSet&,const MultiID&);

    inline const Level&	getLevel( int idx ) const
			{ return idx<size() ? *lvls_[idx] : Level::undef(); }
    int			levelID( int idx ) const
			{ return getLevel(idx).id(); }
    OD::Color		color( int idx ) const
			{ return getLevel(idx).color(); }
    void		getNames(BufferStringSet&) const;

protected:

    ObjectSet<Level>	lvls_;

    mutable int         notiflvlidx_;
    mutable Level::ID	lastlevelid_;
    bool		ischanged_;

    Level*		getNew(const Level* lvl=0) const;
    Level*		set(const char*,const OD::Color&,int);
    int			gtIdxOf(const char*,Level::ID) const;
    Level*		gtLvl(const char*,Level::ID) const;
    void		addLvl(Level*);
    void		getLevelsFrom(const LevelSet&);
    void		makeMine(Level&);
    void		readPars(ascistream&,bool);

    void		lvlChgCB( CallBacker* cb )	{ notif(cb,true); }
    void		lvlRemCB( CallBacker* cb )	{ notif(cb,false); }
    void		notif(CallBacker*,bool);

    static bool		haveCurSet();

public:

    Repos::Source	readOldRepos();
    friend class	LevelSetMgr;

    bool		store(Repos::Source) const;
    bool		read(Repos::Source);
};

mGlobal(General) const LevelSet& LVLS();
inline LevelSet& eLVLS()	{ return const_cast<LevelSet&>(LVLS()); }

// Needless to say that if you push, make sure you pop (so afterwards the real
// default levels are restored
mGlobal(General) void pushLevelSet(LevelSet*);
mGlobal(General) void popLevelSet();
mGlobal(General) const LevelSet& unpushedLVLS();

mGlobal(General) void setLVLS(LevelSet*);

mGlobal(General) BufferString getStdFileName(
					const char* inpnm,const char* basenm);
//!< example: getStdFileName("North Sea","Levels")

} // namespace Strat

