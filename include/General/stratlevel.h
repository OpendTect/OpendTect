#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "color.h"
#include "integerid.h"
#include "namedobj.h"
#include "objectset.h"
#include "repos.h"

class ascistream;
class BufferStringSet;


namespace Strat
{

using LevelID = IntegerID<od_int32>;

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

    typedef int		ChangeType;

			Level(const char* nm,const OD::Color&,
			      LevelID =LevelID::udf());
			Level(const Level&);
    virtual		~Level();

    Level&		operator =(const Level&);
    bool		operator ==(const Level&) const;
    bool		operator !=(const Level&) const;
    bool		isDifferentFrom(const Level&) const;

    LevelID		id() const		{ return id_; }
    OD::Color		color() const		{ return color_; }
    const IOPar&	pars() const		{ return pars_; }

    Level&		setID(LevelID);
    void		setName(const char*) override;
    Level&		setColor(OD::Color);
    Level&		setPars(const IOPar&);

    CNotifier<Level,ChangeType> changed;

    static const Level&	undef();
    bool		isUndef() const;
    static Level&	dummy();

    static ChangeType	cEntireChange() { return -1; }
    static ChangeType	cNameChange()	{ return 1; }
    static ChangeType	cColChange()	{ return 2; }
    static ChangeType	cParsChange()	{ return 3; }

protected:
			Level(const Level&,int idasint);

    const LevelID	id_;
    OD::Color		color_;
    IOPar&		pars_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    friend class	LevelSet;

};


mExpClass(General) RegMarker : public Level
{
public:
			RegMarker(const char* nm,const OD::Color&,
				  LevelID =LevelID::udf());
			RegMarker(const RegMarker&);
			~RegMarker();

    RegMarker&		setID(LevelID);

    static bool		isRegMarker(const Level&);
    static bool		isRegMarker(const LevelID);
    static const int	minID();

private:

    friend class	RegMarkerSet;
};


/*!\brief Set of Stratigraphic levels

  Manages not only the units, but also their IDs. The set will not allow
  adding levels with the same name; the result is that you simply set the
  existing one to new values.

*/


mExpClass(General) LevelSet : public NamedCallBacker //TODO: mk shared
{
public:
    typedef int		ChangeType;

			LevelSet(int idstartidsat=0);
			LevelSet(const LevelSet&);
    virtual		~LevelSet();
    LevelSet&		operator =(const LevelSet&);
    bool		operator ==(const LevelSet&) const;

    bool		isEmpty() const			{ return size() < 1; }
    int			size() const;
    void		setEmpty();

    bool		isPresent(LevelID) const;
    bool		isPresent(const char*) const;
    LevelID		getIDByName(const char*) const;
    LevelID		getIDByIdx(int) const;
    int			indexOf(LevelID) const;
    int			indexOf(const char*) const;
    void		getNames(BufferStringSet&) const;
    BufferString	nameOf(LevelID) const;
    OD::Color		colorOf(LevelID) const;
    IOPar		parsOf(LevelID) const;

    Level		get(LevelID) const;
    Level		getByIdx(int) const;
    Level		getByName(const char*) const;
    Level		first() const;

    LevelID		add(const char*,const OD::Color&);
    void		remove(LevelID);

    LevelID		set(const Level&);
			//!< copy stuff, but new ID/name
    LevelID		add(const Level& lvl)	    { return set( lvl ); }
    void		add(const BufferStringSet&,const TypeSet<OD::Color>&);

    Notifier<LevelSet>			setChanged;
    CNotifier<LevelSet,LevelID>		levelAdded;
    CNotifier<LevelSet,LevelID>		levelToBeRemoved;

    bool		readFrom(const char*);
    bool		writeTo(const char*) const;
    bool		write() const;

    static void		getStdNames(BufferStringSet&);
    static LevelSet*	createStd(const char*);
    static LevelSet*	read(const MultiID&);
    static bool		write(const LevelSet&,const MultiID&);

    void		removeAllRegMarkers();

protected:

    ObjectSet<Level>		    lvls_;
    mutable Threads::Atomic<int>    curlevelid_;
    MultiID			    dbky_;

    void		addRegMarkers();
    void		regMarkerAdded(CallBacker*);
    void		regMarkerRemoved(CallBacker*);
    int			gtIdxOf(const char*,LevelID) const;
    Level		gtLvl(int) const;
    void		doSetEmpty();
    LevelID		doSet(const Level&,bool* isnew=nullptr);

    void		getFromStream(ascistream&,bool);
    Repos::Source	readOldRepos();

public:

    friend class	LevelSetMgr;

};


mExpClass(General) RegMarkerSet : public CallBacker
{
public:
    typedef int		ChangeType;

			RegMarkerSet();
			~RegMarkerSet();

    int			size() const;
    bool		isEmpty() const;
    void		setEmpty();

    const RegMarker&	get(const LevelID&) const;
    RegMarker&		get(const LevelID&);
    const RegMarker&	getByIdx(int) const;
    RegMarker&		getByIdx(int);
    const RegMarker&	getByName(const char*) const;
    RegMarker&		getByName(const char*);

    bool		isPresent(const LevelID&) const;
    bool		isPresent(const char* nm) const;
    int			indexOf(const LevelID&) const;
    int			indexOf(const char* nm) const;

    LevelID		add(const char*,const OD::Color&);
    void		remove(const LevelID&);

    LevelID		set(const RegMarker&);
			//!< copy stuff, but new ID/name
    LevelID		add(const RegMarker& regm)	{ return set( regm ); }
    void		add(const BufferStringSet&,
			    const TypeSet<OD::Color>&);

    void		reset();
    bool		save() const;

    CNotifier<RegMarkerSet,LevelID>		levelChanged;
    CNotifier<RegMarkerSet,LevelID>		levelAdded;
    CNotifier<RegMarkerSet,LevelID>		levelToBeRemoved;

private:

    ManagedObjectSet<RegMarker>     rgmlvls_;
    mutable Threads::Atomic<int>    curregmid_	    = Strat::RegMarker::minID();

    RegMarkerSet&	operator=(const RegMarkerSet&) = delete;
    int			gtIdxOf(const char* nm,const LevelID&) const;
    const RegMarker&	getRegMarker(int idx) const;
    LevelID		doSet(const RegMarker&,bool* isnew=nullptr);
    void		readRegMarkers();
    bool		writeRegMarkers() const;

    void		iomReadyCB(CallBacker*);
    void		surveyChangedCB(CallBacker*);
    void		applClosingCB(CallBacker*);
};


mGlobal(General) const LevelSet& LVLS();
inline LevelSet& eLVLS()	{ return const_cast<LevelSet&>(LVLS()); }
inline BufferString levelNameOf( const LevelID& id )
				{ return LVLS().nameOf(id); }

mGlobal(General) const RegMarkerSet& RGMLVLS();
inline RegMarkerSet& eRGMLVLS() { return const_cast<RegMarkerSet&>(RGMLVLS()); }

// From here: do not use, you will not need it.
// Needless to say that if you push, make sure you pop (so afterwards the real
// default levels are restored

mExpClass(General) LevelSetMgr : public CallBacker
{
public:
			LevelSetMgr();
			~LevelSetMgr();

    const LevelSet&	curSet() const;
    const LevelSet&	unpushedLVLS() const;

    void		pushLevelSet(LevelSet*);
    void		popLevelSet();
    void		setLVLS(LevelSet*);

    Notifier<LevelSetMgr> curChanged;

private:

    LevelSet&		curSet();
    void		ensurePresent(LevelSet&);
    void		createSet();
    void		iomReadyCB(CallBacker*);
    void		surveyChangedCB(CallBacker*);

    ManagedObjectSet<LevelSet>	lss_;
    mutable Threads::Lock lock_;
};

mGlobal(General) LevelSetMgr& lvlSetMgr();
mGlobal(General) BufferString getStdFileName(
					const char* inpnm,const char* basenm);
//!< example: getStdFileName("North Sea","Levels")

} // namespace Strat
