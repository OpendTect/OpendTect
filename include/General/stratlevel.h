#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
________________________________________________________________________

-*/

#include "generalmod.h"
#include "sharedobject.h"
#include "undefval.h"
#include "ranges.h"
#include "color.h"
#include "repos.h"
#include "integerid.h"
#include "uistring.h"
class ascistream;
class BufferStringSet;


namespace Strat
{


/*!\brief Stratigraphic level

  To store extra details, use the pars_.

  The ID cannot be assigned, and it is unique. It is managed by the LevelSet.
  Therefore:
  * operator ==() : checks only ID
  * isDifferentFrom() : checks all but ID

*/

mExpClass(General) Level : public NamedMonitorable
{
public:

    mDefIntegerIDType(int,ID);

			Level(const char* nm,const Color&,ID =ID::getInvalid());

			mDeclMonitorableAssignment(Level);
    bool		operator ==(const Level&) const;
    bool		isDifferentFrom(const Level&) const;

    ID			id() const	{ return id_; }
    mImplSimpleMonitoredGetSet(inline,color,setColor,Color,color_,cColChange())
    mImplSimpleMonitoredGetSet(inline,pars,setPars,IOPar,pars_,cParsChange())

    static const Level&	undef();
    bool		isUndef() const;
    mDeclInstanceCreatedNotifierAccess(Level);

    static ChangeType	cColChange()	{ return 2; }
    static ChangeType	cParsChange()	{ return 3; }

protected:

			Level(const Level&,ID::IDType);
    const ID		id_;
    Color		color_;
    IOPar		pars_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    friend class	LevelSet;

public:

    virtual		~Level();

};


/*!\brief Set of Stratigraphic levels

  Manages not only the units, but also their IDs. The set will not allow
  adding levels with the same name; the result is that you simply set the
  existing one to new values.

*/


mExpClass(General) LevelSet : public SharedObject
{
public:
    mDefIntegerIDType(int,LVLID);

    typedef Level::ID			ID;
    typedef ObjectSet<Level>::size_type	size_type;
    typedef size_type			IdxType;


			LevelSet(int idstartidsat=0);
			mDeclMonitorableAssignment(LevelSet);

    bool		isEmpty() const		{ return size() < 1; }
    size_type		size() const;
    void		setEmpty();

    bool		isPresent(ID) const;
    bool		isPresent(const char*) const;
    ID			getIDByName(const char*) const;
    ID			getIDByIdx(IdxType) const;
    IdxType		indexOf(ID) const;
    IdxType		indexOf(const char*) const;
    void		getNames(BufferStringSet&) const;
    BufferString	levelName(ID) const;
    Color		levelColor(ID) const;
    IOPar		levelPars(ID) const;

    Level		get(ID) const;
    Level		getByIdx(int) const;
    Level		getByName(const char*) const;
    Level		first() const;

    ID			add(const char*,const Color&);
    void		remove(ID);

    ID			set(const Level&);
    ID			add( const Level& lvl )		{ return set( lvl ); }
    void		add(const BufferStringSet&,const TypeSet<Color>&);

    static ChangeType	cLevelAdded()			{ return 2; }
    static ChangeType	cLevelToBeRemoved()		{ return 3; }

    bool		store(Repos::Source) const;
    bool		read(Repos::Source);
    bool		readFrom(const char*);
    bool		writeTo(const char*) const;

    static void		getStdNames(BufferStringSet&);
    static LevelSet*	createStd(const char*);

protected:

    virtual		~LevelSet();

    ObjectSet<Level>	lvls_;
    mutable Threads::Atomic<ID::IDType> curlevelid_;

    int			gtIdxOf(const char*,ID) const;
    Level		gtLvl(int) const;
    void		doSetEmpty();
    ID			doSet(const Level&,bool* isnew=0);

    void		getFromStream(ascistream&,bool);
    Strat::Level::ID	lvlid_;

public:

    Repos::Source	readOldRepos();
    friend class	LevelSetMgr;

};

mGlobal(General) const LevelSet& LVLS();
inline LevelSet& eLVLS()	{ return const_cast<LevelSet&>(LVLS()); }

// From here: do not use, you will not need it.
// Needless to say that if you push, make sure you pop (so afterwards the real
// default levels are restored
mGlobal(General) void pushLevelSet(LevelSet*);
mGlobal(General) void popLevelSet();
mGlobal(General) const LevelSet& unpushedLVLS();
mGlobal(General) void setLVLS(LevelSet*);
mGlobal(General) BufferString getStdFileName(const char* inpnm,
					    const char* basenm);
//!< example: getStdFileName("North Sea","Levels")


}; //namespace
