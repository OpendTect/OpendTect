#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "volprocstep.h"


namespace VolProc
{


/*!\brief a Step that suppresses undefined values in the input data. */

mExpClass(VolumeProcessing) UdfReplacer : public Step
{ mODTextTranslationClass(UdfReplacer)
public:
			mDefaultFactoryInstantiation( Step, UdfReplacer,
				"UdfReplacer", tr("Undefined Values Replacer") )

			UdfReplacer();
			~UdfReplacer();

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyPadTraces();

private:

    virtual bool	prepareWork(int=1);
    virtual ReportingTask*	createTask();

    virtual bool	needsFullVolume() const		{ return false; }
    virtual bool	canInputAndOutputBeSame() const;
    virtual bool	areSamplesIndependent() const	{ return true; }
    virtual bool	canHandle2D() const		{ return true; }

    virtual int		getNrOutComponents(OutputSlotID,Pos::GeomID) const;
    virtual bool	copyComponentsSel(const InputSlotID,
					  OutputSlotID&) const;

    virtual od_int64	extraMemoryUsage(OutputSlotID,
					 const TrcKeyZSampling&) const
			{ return 0; }

    float		replval_ = 0.f;
    bool		padmissingtraces_ = false;

    TypeSet<int>	comps_;
			/* Work only done on selected components. The others
			   gets blunted copied without change.*/

};

} // namespace VolProc
