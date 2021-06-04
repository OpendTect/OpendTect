#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Apr 2002
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "ranges.h"
#include "bufstringset.h"
#include "typeset.h"
#include "od_iosfwd.h"


/*!
\brief Interface for providing parameter files for jobs (job descriptions). Two
implementations are pre-cooked: KeyReplace-JDP and InlineSplit-JDP.
*/

mExpClass(MMProc) JobDescProv
{
public:

    			JobDescProv(const IOPar&);
    virtual		~JobDescProv();

    virtual int		nrJobs() const			= 0;
    virtual void	getJob(int,IOPar&) const	= 0;
    virtual const char*	objType() const			= 0;
    virtual const char*	objName(int) const		= 0;
    virtual void	dump(od_ostream&) const		= 0;

    const IOPar&	pars() const			{ return inpiopar_; }

protected:

    IOPar&		inpiopar_;
    mutable BufferString objnm_;

};


/*!\brief Simple implementation of JobDescProv based on replacing a value
in the IOPar with another.
*/

mExpClass(MMProc) KeyReplaceJobDescProv : public JobDescProv
{
public:
    			KeyReplaceJobDescProv(const IOPar&,const char* key,
						int nrjobs);

    virtual int		nrJobs() const		{ return nrjobs_; }
    virtual void	getJob(int,IOPar&) const;
    virtual const char*	objType() const		{ return objtyp_.buf(); }
    virtual const char*	objName(int) const;
    virtual void	dump(od_ostream&) const;

    BufferString	objtyp_;

protected:

    const int		nrjobs_;
    BufferString	key_;
    virtual const char*	gtObjName(int) const	= 0;

};


/*!\brief KeyReplaceJobDescProv where the values are in a string set. */

mExpClass(MMProc) StringKeyReplaceJobDescProv : public KeyReplaceJobDescProv
{
public:
    			StringKeyReplaceJobDescProv(const IOPar&,const char* ky,
					      const BufferStringSet& nms);
			    //!< IOPar, key and bufstringset will be copied

protected:

    BufferStringSet	names_;
    virtual const char*	gtObjName(int) const;

};


/*!\brief KeyReplaceJobDescProv where the values taken from a range of IDs. */

mExpClass(MMProc) IDKeyReplaceJobDescProv : public KeyReplaceJobDescProv
{
public:
    			IDKeyReplaceJobDescProv(const IOPar&,const char* ky,
					      const StepInterval<int>& idrg);
			    //!< IOPar, key and bufstringset will be copied

    virtual void	dump(od_ostream&) const;

protected:

    const StepInterval<int> idrg_;
    virtual const char*	gtObjName(int) const;

};


/*!\brief Implementation of JobDescProv based upon splitting the inlines in the
IOPar.

The keying is either:
1) Standard style with the keys in keystrs.h sKey::FirstInl() etc.
2) Single key FileMultiString type first`last`step
*/

mExpClass(MMProc) InlineSplitJobDescProv : public JobDescProv
{
public:
    			InlineSplitJobDescProv(const IOPar&);
    			InlineSplitJobDescProv(const IOPar&,
						const TypeSet<int>&);
			~InlineSplitJobDescProv();

    virtual int		nrJobs() const;
    virtual void	getJob(int,IOPar&) const;
    virtual const char*	objType() const			{ return "inline"; }
    virtual const char*	objName(int) const;
    virtual void	dump(od_ostream&) const;

    void		getRange(StepInterval<int>&) const;
    int			getNrInlsPerJob()		{ return ninlperjob_; }
    void		setNrInlsPerJob(const int nr)	{ ninlperjob_ = nr; }

    static const char*	sKeyMaxInlRg(); //!< absolute limit - will override
    static const char*	sKeyMaxCrlRg(); //!< absolute limit - will override

    static int		defaultNrInlPerJob();
    static void		setDefaultNrInlPerJob(int);

protected:

    StepInterval<int>	inlrg_;
    TypeSet<int>*	inls_;

    int			ninlperjob_;
    TypeSet<int>	jobs_;

    int			firstInlNr(int) const;
    int			lastInlNr(int) const;

};


/*!\brief IOPar driven implementation of JobDescProv where splitting is based
on IOPar subselection with a particular key (subselkey). For instance, an IOPar
with entries:
\code
  SplitterKey.0.firstparameter: firstvalue0
  SplitterKey.0.secondparameter: secondvalue0
  SplitterKey.1.firstparameter: firstvalue1
  SplitterKey.1.secondparameter: secondvalue1
\endcode
can be split into two jobs based on subselkey 'SplitterKey'.
*/

mExpClass(MMProc) ParSubselJobDescProv : public JobDescProv
{
public:
			ParSubselJobDescProv(const IOPar&,
					     const char* subselkey);

    virtual int		nrJobs() const		{ return subselpars_.size(); }
    virtual void	getJob(int,IOPar&) const;
    virtual void	dump(od_ostream&) const;

protected:

    BufferString	subselkey_;
    ManagedObjectSet<IOPar>	subselpars_;

};


/*!\brief Special case of ParSubselJobDescProv where each job refers to a
2D Line. The subselkey in this case is "Output.Subsel.Line". */

mExpClass(MMProc) Line2DSubselJobDescProv : public ParSubselJobDescProv
{
public:
			Line2DSubselJobDescProv(const IOPar&);

    const char*		objType() const		{ return "Line"; }
    const char*		objName(int) const;
};

