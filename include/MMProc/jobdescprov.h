#ifndef jobdescprov_h
#define jobdescprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Apr 2002
 RCS:		$Id: jobdescprov.h,v 1.1 2004-10-25 07:26:20 bert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "bufstringset.h"

class IOPar;


/*!\brief Interface for providing parameter files for jobs (job descriptions).
 
  Two implementations are pre-coocked: KeyReplace-JDP and InlineSplit-JDP.

 */

class JobDescProv
{
public:

    			JobDescProv(const IOPar&);
    virtual		~JobDescProv();

    virtual int		nrJobs() const			= 0;
    virtual void	getJob(int,IOPar&) const	= 0;
    virtual const char*	objName(int) const		= 0;
    virtual void	dump(std::ostream&) const	= 0;

protected:

    IOPar&		inpiopar_;
    mutable BufferString objnm_;

};


/*!\brief Simple implementation of JobDescProv based upon replacing a value
in the IOPar for one of the strings from a BufferStringSet.

 */

class KeyReplaceJobDescProv : public JobDescProv
{
public:
    			KeyReplaceJobDescProv(const IOPar&,const char* key,
					      const BufferStringSet& nms);
			//!< IOPar, key and bufstringset will be copied

    virtual int		nrJobs() const		{ return names_.size(); }
    virtual void	getJob(int,IOPar&) const;
    virtual const char*	objName(int) const;
    virtual void	dump(std::ostream&) const;

protected:

    BufferString	key_;
    BufferStringSet	names_;

};


/*!\brief Implementation of JobDescProv based upon splitting the
inlines in the IOPar.

The keying is either:
1) Standard style with the keys in keystrs.h sKey::FirstInl etc.
2) Single key FileMultiString type first`last`step

 */

class InlineSplitJobDescProv : public JobDescProv
{
public:
    			InlineSplitJobDescProv(const IOPar&,
						const char* single_key=0);
    			InlineSplitJobDescProv(const IOPar&,const TypeSet<int>&,
						const char* single_key=0);
			~InlineSplitJobDescProv();

    virtual int		nrJobs() const
			{ return inls_ ? inls_->size() : inlrg_.nrSteps() + 1; }
    virtual void	getJob(int,IOPar&) const;
    virtual const char*	objName(int) const;
    virtual void	dump(std::ostream&) const;

    void		getRange(StepInterval<int>&) const;

protected:

    const BufferString	singlekey_;
    StepInterval<int>	inlrg_;
    TypeSet<int>*	inls_;

    int			inlNr(int) const;

};


#endif
