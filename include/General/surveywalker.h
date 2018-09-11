#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Sep 2018
________________________________________________________________________

-*/


#include "generalmod.h"

#include "bufstringset.h"
#include "executor.h"

class IOPar;


/*!
\brief Sets DBMan to execute a function in a set of OpendTect surveys
*/

namespace Survey
{

mExpClass(General) Walker : public Executor
{ mODTextTranslationClass(Walker)
public:

    virtual			~Walker();

    virtual uiString		message() const;
    virtual uiString		nrDoneText() const;

    virtual od_int64		nrDone() const		{ return nrdone_; }
    virtual od_int64		totalNr() const;

protected:
				Walker(const char* execnm,
				       const char* dataroot =0);
				Walker(const char* execnm,
				       const BufferStringSet& dataroots);

    void			addSurveys(const char* dataroot);
				//!< Null: current dataroot is used

    virtual bool		init()	{ return true; }
				//!< The list of surveys has been constructed

    virtual bool		handleSurvey(uiRetVal&)			= 0;
				/*!< The current survey is set and can be used.
				     Return false to abort without trying
				     the next survey, true otherwise
				 */

private:
    virtual bool		goImpl(od_ostream*,bool,bool,int) final;
    virtual int			nextStep() final;

    BufferStringSet		allfullpaths_;

    od_int64			nrdone_ = 0;
    mutable uiRetVal		errmsgs_;
};



/*!
\brief Collects Survey Information from one or several dataroots. Information
       stays in memory and can be output to files.
*/

mExpClass(General) SICollector : public Walker
{
public:
    enum CollectorType		{ IOParDump, FileSummary };

				SICollector(const char* dataroot =0,
					    CollectorType=IOParDump);
				SICollector(const BufferStringSet& dataroots,
					    CollectorType=IOParDump);
    virtual			~SICollector();

    void			setType( CollectorType typ )	{ type_ = typ; }

    void			setOutputFolder(const char* dirpath,
						uiString* errmsg =0);

    bool			fillPar(ObjectSet<IOPar>&) const;

private:
    void			cleanup();
    virtual bool		init();
    virtual bool		handleSurvey(uiRetVal&);

    CollectorType		type_ = IOParDump;
    BufferString		outputpath_;
    ObjectSet<IOPar>		pars_;
};

}; //namespace Survey
