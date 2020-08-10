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

mExpClass(MATLABLink) MatlabStep : public Step
{ mODTextTranslationClass(MatlabStep);
public:
			mDefaultFactoryInstantiation( Step,
				MatlabStep,
				"MatlabStep",
				toUiString("MATLAB") )

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		needsInput() const		{ return true; }
    int			getNrInputs() const;
    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		isInputPrevStep() const		{ return false; }

    bool		needsFullVolume() const		{ return true; }
    virtual uiString	errMsg() const			{ return errmsg_; }

    Task*		createTask();

    void		setSharedLibFileName(const char*);
    const char*		sharedLibFileName() const;

    void		setNrInputs(int);
    void		setParameters(const BufferStringSet& nms,
				      const BufferStringSet& vals);
    void		getParameters(BufferStringSet& nms,
				      BufferStringSet& vals) const;

    mDeprecatedDef virtual od_int64	getProcTimeExtraMemory() const
			{ return 0; }

    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

protected:

			MatlabStep();
			~MatlabStep();

    BufferString	sharedlibfnm_;

    int			nrinputs_;
    BufferStringSet	parnames_;
    BufferStringSet	parvalues_;
};

} // namespace VolProc

#endif
