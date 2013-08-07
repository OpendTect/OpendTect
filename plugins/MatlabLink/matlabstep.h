#ifndef matlabstep_h
#define matlabstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "matlablinkmod.h"
#include "volprocchain.h"

namespace VolProc
{

mExpClass(MatlabLink) MatlabStep : public Step
{
public:
			mDefaultFactoryInstantiation( Step,
				MatlabStep,
				"MatlabStep",
				"MATLAB" );

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		needsInput() const 		{ return true; }
    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		needsFullVolume() const		{ return true; }
    bool		areSamplesIndependent() const	{ return false; }
    const char*		errMsg() const		{ return errmsg_.str(); }

    Task*		createTask();

    void		setSharedLibFileName(const char*);
    const char*		sharedLibFileName() const;

    void		setParameters(const BufferStringSet& nms,
				      const BufferStringSet& vals);
    void		getParameters(BufferStringSet& nms,
				      BufferStringSet& vals) const;

protected:

			MatlabStep();
			~MatlabStep();

    FixedString		errmsg_;
    BufferString	sharedlibfnm_;

    BufferStringSet	parnames_;
    BufferStringSet	parvalues_;
};

} // namespace VolProc

#endif

