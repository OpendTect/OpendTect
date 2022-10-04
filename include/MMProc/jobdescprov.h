#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    virtual		~JobDescProv();

    virtual int		nrJobs() const			= 0;
    virtual void	getJob(int,IOPar&) const	= 0;
    virtual const char*	objType() const			= 0;
    virtual const char*	objName(int) const		= 0;
    virtual void	dump(od_ostream&) const		= 0;

    const IOPar&	pars() const			{ return inpiopar_; }

protected:
			JobDescProv(const IOPar&);

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
			~KeyReplaceJobDescProv();

    int			nrJobs() const override		{ return nrjobs_; }
    void		getJob(int,IOPar&) const override;
    const char*		objType() const override { return objtyp_.buf(); }
    const char*		objName(int) const override;
    void		dump(od_ostream&) const override;

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
			~StringKeyReplaceJobDescProv();

protected:

    BufferStringSet	names_;
    const char*		gtObjName(int) const override;

};


/*!\brief KeyReplaceJobDescProv where the values taken from a range of IDs. */

mExpClass(MMProc) IDKeyReplaceJobDescProv : public KeyReplaceJobDescProv
{
public:
			IDKeyReplaceJobDescProv(const IOPar&,const char* ky,
					      const StepInterval<int>& idrg);
			    //!< IOPar, key and bufstringset will be copied
			~IDKeyReplaceJobDescProv();

    void		dump(od_ostream&) const override;

protected:

    const StepInterval<int> idrg_;
    const char*		gtObjName(int) const override;

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

    int			nrJobs() const override;
    void		getJob(int,IOPar&) const override;
    const char*		objType() const override	{ return "inline"; }
    const char*		objName(int) const override;
    void		dump(od_ostream&) const override;

    void		getRange(StepInterval<int>&) const;
    int			getNrInlsPerJob()		{ return ninlperjob_; }
    void		setNrInlsPerJob(const int nr)	{ ninlperjob_ = nr; }

    static const char*	sKeyMaxInlRg(); //!< absolute limit - will override
    static const char*	sKeyMaxCrlRg(); //!< absolute limit - will override

    static int		defaultNrInlPerJob();
    static void		setDefaultNrInlPerJob(int);

protected:

    StepInterval<int>	inlrg_;
    TypeSet<int>*	inls_ = nullptr;

    int			ninlperjob_ = 1;
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
			~ParSubselJobDescProv();

    int			nrJobs() const override { return subselpars_.size(); }
    void		getJob(int,IOPar&) const override;
    void		dump(od_ostream&) const override;

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
			~Line2DSubselJobDescProv();

    const char*		objType() const override	{ return "Line"; }
    const char*		objName(int) const override;
};
