#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "mnemonics.h"
#include "position.h"
#include "ranges.h"

class BufferStringSet;
class PropertyRef;

namespace Well
{

class Log;

/*!
\brief Log set
*/

mExpClass(Well) LogSet : public CallBacker
{
public:

			LogSet();
    virtual		~LogSet();

    void		getNames(BufferStringSet&, bool onlyloaded=false) const;

    int			size() const		{ return logs_.size(); }
    Log&		getLog( int idx )	{ return *logs_[idx]; }
    const Log&		getLog( int idx ) const { return *logs_[idx]; }
    Log&		first()			{ return *logs_.first(); }
    const Log&		first() const		{ return *logs_.first(); }
    Log&		last()			{ return *logs_.last(); }
    const Log&		last() const		{ return *logs_.last(); }
    int			indexOf(const char*) const;
    bool		isLoaded(const char*) const;
    bool		isPresent(const char*) const;
    bool		hasDefaultFor(const Mnemonic&) const;
    bool		setDefaultMnemLog(const Mnemonic&,const char* lognm);
    bool		removeDefault(const Mnemonic&);
    void		getDefaultLogs(BufferStringSet&,
				       bool onlyloaded=false) const;
    bool		isDefaultLog(const char* lognm) const;
    void		renameDefaultLog(const char* oldnm, const char* newnm);
			//<! make sure the log is renamed first
    const Log*		getLog( const char* nm ) const	{ return gtLog(nm); }
    Log*		getLog( const char* nm )	{ return gtLog(nm); }

    const Log*		getLog(const Mnemonic&) const;
			/*<! default if default is present, otherwise first
			     of suitable logs !>*/
    const Log*		getDefaultLog(const Mnemonic&) const;
			//<! returns null if default is not assigned.

    Interval<float>	dahInterval() const	{ return dahintv_; }
						//!< not def if start == undef
    void		updateDahIntvs();
						//!< if logs changed
    void		removeTopBottomUdfs();

    void		add(Log*);		//!< becomes mine
    void		add(const LogSet&);	//!< copies all logs
    Log*		remove(int);		//!< becomes yours
    void		swap(int idx0,int idx1) { logs_.swap( idx0, idx1 ); }
    bool		validIdx(int idx) const { return logs_.validIdx(idx); }

    bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty(bool withdelete=true);
    void		defaultLogUsePar(const IOPar&);
			//-> to be used only in Well::Reader/Writer class.
			//-> Use access functions above instead.
    void		defaultLogFillPar(IOPar&) const;
			//-> to be used only in Well::Reader/Writer class.
			//-> Use access functions above instead.

    void		getAllAvailMnems(MnemonicSelection&) const;
    TypeSet<int>	getSuitable(const Mnemonic&) const;
    TypeSet<int>	getSuitable(Mnemonic::StdType,
				    const PropertyRef* altpr=nullptr,
				    BoolTypeSet* isalt=nullptr) const;

    const Mnemonic*	getMnemonicOfLog(const char* nm) const;

    Notifier<LogSet>	logAdded;
    Notifier<LogSet>	logRemoved;

protected:

    ObjectSet<Log>	logs_;
    Interval<float>	dahintv_;

    ObjectSet<std::pair<const Mnemonic&,BufferString>> defaultlogs_;

    void		init()
			{ dahintv_.start = mSetUdf(dahintv_.stop); }

    void		updateDahIntv(const Well::Log&);

    Log*		gtLog( const char* nm ) const
			{ const int idx = indexOf(nm);
			    return idx < 0 ? nullptr
					   : const_cast<Log*>(logs_[idx]); }

private:
			LogSet(const LogSet&)		= delete;
    LogSet&		operator= (const LogSet&)	= delete;

};

} // namespace Well
