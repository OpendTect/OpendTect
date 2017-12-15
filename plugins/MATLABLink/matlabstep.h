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

#define mMATLABLinkPackage "MatLab Access"


namespace VolProc
{

mExpClass(MATLABLink) MatlabStep : public Step
{ mODTextTranslationClass(MatlabStep);
public:
			mDefaultFactoryInstantiation( Step,
				MatlabStep,
				"MatlabStep",
				toUiString("MATLAB") )

    void		setSharedLibFileName(const char*);
    const char*		sharedLibFileName() const;

    int			getNrInputs() const;
    void		setNrInputs(int);
    void		setParameters(const BufferStringSet& nms,
				      const BufferStringSet& vals);
    void		getParameters(BufferStringSet& nms,
				      BufferStringSet& vals) const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    virtual bool	needsFullVolume() const		{ return true; }
    virtual bool	canInputAndOutputBeSame() const	{ return true; }
    virtual bool	areSamplesIndependent() const	{ return true; }
    virtual bool	needsInput() const		{ return true; }
    virtual bool	isInputPrevStep() const		{ return false; }

    virtual uiString	errMsg() const			{ return errmsg_; }

    virtual Task*	createTask();

protected:

			MatlabStep();
			~MatlabStep();

    virtual od_int64	extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const;

    BufferString	sharedlibfnm_;

    int			nrinputs_;
    BufferStringSet	parnames_;
    BufferStringSet	parvalues_;

};

} // namespace VolProc

#endif
