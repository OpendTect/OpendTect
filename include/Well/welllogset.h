#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "mnemonics.h"
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
			~LogSet();

	    // Available without actual log data
    int			size() const		{ return logs_.size(); }
    bool		isEmpty() const		{ return logs_.isEmpty(); }
    bool		isPresent(const char*) const;
    bool		isLoaded(const char*) const;
    int			nrLoaded() const;
    bool		validIdx(int idx) const { return logs_.validIdx(idx); }
    int			indexOf(const char*) const;
    bool		areAllLoaded() const;

    void		setEmpty(bool withdelete=true);
    void		swap(int idx0,int idx1) { logs_.swap( idx0, idx1 ); }

    const char*		getLogNameByIdx(int) const;
    const char*		getLogNameFor(const Mnemonic&) const;
			/*<! default if default is present, otherwise first
			     of suitable logs !>*/
    const char*		getDefaultLogName(const Mnemonic&) const;
    Interval<float>	getDahRangeForLog(const char*) const;
    Interval<float>	getValueRangeForLog(const char*) const;

    bool		hasDefaultFor(const Mnemonic&) const;
    bool		setDefaultMnemLog(const Mnemonic&,const char* lognm);
    bool		removeDefault(const Mnemonic&);
    void		getDefaultLogs(BufferStringSet&,
				       bool onlyloaded=false) const;
    bool		isDefaultLog(const char* lognm) const;
    void		renameDefaultLog(const char* oldnm,const char* newnm);
			//<! make sure the log is renamed first
    const char*		getMnemonicLblOfLog(const char* nm) const;
    const Mnemonic*	getMnemonicOfLog(const char* nm,
					 bool setifnull=false) const;
    const char*		getUnitOfMeasureLblOfLog(const char* nm) const;
    const UnitOfMeasure* getUnitOfMeasureOfLog(const char* nm) const;

    void		setMnemonicOfLog(const char* lognm,const Mnemonic&);
    void		setUnitOfMeasureOfLog(const char* lognm,
					      const UnitOfMeasure*);
    const IOPar*	getParsOfLog(const char* lognm) const;

    const Log*		getLogInfos(const char* lognm) const;

    Interval<float>	dahInterval() const	{ return dahintv_; }
						//!< not def if start == undef

    void		getNames(BufferStringSet&, bool onlyloaded=false) const;
    void		getAllAvailMnems(MnemonicSelection&) const;
    TypeSet<int>	getLogsWithNoMnemonics() const;
    TypeSet<int>	getSuitable(const Mnemonic&) const;
    TypeSet<int>	getSuitable(Mnemonic::StdType,
				    const PropertyRef* altpr=nullptr,
				    BoolTypeSet* isalt=nullptr) const;
    void		defaultLogUsePar(const IOPar&);
			//-> to be used only in Well::Reader/Writer class.
			//-> Use access functions above instead.
    void		defaultLogFillPar(IOPar&) const;
			//-> to be used only in Well::Reader/Writer class.
			//-> Use access functions above instead.

	    //Only available when the actual log data is loaded:
    const Log*		getLog(const char* nm) const;
    const Log*		getLog(const OD::String&) const;
    const Log&		getLogByIdx(int) const;
    const Log&		first() const;
    const Log&		last() const;
    const Log*		getLog(const Mnemonic&) const;
			/*<! default if default is present, otherwise first
			     of suitable logs !>*/
    const Log*		getDefaultLog(const Mnemonic&) const;
			//<! returns null if default is not assigned.
    Log*		getLog(const char* nm);
    Log*		getLog(const OD::String&);
    Log&		getLogByIdx(int);
    Log&		first();
    Log&		last();

    void		updateDahIntvs();
    void		removeTopBottomUdfs();

    void		add(Log*);		//!< becomes mine
    void		add(const LogSet&);	//!< copies all logs
    Log*		remove(int);		//!< becomes yours

    Notifier<LogSet>	logAdded;
    Notifier<LogSet>	logRemoved;

    static const char*	sKeyDefMnem();

protected:

    ObjectSet<Log>	logs_;
    Interval<float>	dahintv_;

    ObjectSet<std::pair<const Mnemonic&,BufferString>> defaultlogs_;

    void		init()	{ dahintv_.setUdf(); }

    void		mnemonicRemovedCB(CallBacker*);

    void		updateDahIntv(const Well::Log&);

    const Log*		gtLogByIdx(int,bool needjustinfo) const;
    const Log*		gtLog(const char* nm,bool needjustinfo) const;

    Log*		gtLogByIdx(int,bool needjustinfo);
    Log*		gtLog(const char* nm,bool needjustinfo);

private:
			mOD_DisableCopy(LogSet);

public:
			mDeprecated("Use getLogByIdx")
    Log&		getLog( int idx )	{ return getLogByIdx(idx); }
			mDeprecated("Use getLogByIdx")
    const Log&		getLog( int idx ) const { return getLogByIdx(idx); }

};

} // namespace Well
