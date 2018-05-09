#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
 RCS:		$Id: volprocregionfiller.h 36409 2014-09-13 21:11:44Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "dbkey.h"

namespace EM { class Region3D; }

namespace VolProc
{

/*!\brief Region filler */

mExpClass(VolumeProcessing) RegionFiller : public Step
{ mODTextTranslationClass(RegionFiller);
public:

				mDefaultFactoryInstantiation( Step,
				    RegionFiller, "RegionFiller",
				    tr("Region painter") )

				RegionFiller();
				~RegionFiller();
    virtual void		releaseData();

    void			setInsideValue(float);
    float			getInsideValue() const;
    void			setOutsideValue(float);
    float			getOutsideValue() const;

    float			getStartValue() const;
    const DBKey&		getStartValueHorizonID() const;
    int				getStartValueAuxDataIdx() const;
    float			getGradientValue() const;
    const DBKey&		getGradientHorizonID() const;
    int				getGradientAuxDataIdx() const;

    EM::Region3D&		region();
    const EM::Region3D&		region() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

private:

    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const	{ return false;}
    virtual bool		areSamplesIndependent() const	{ return true;}
    virtual bool		needsInput() const		{ return false;}
    virtual bool		isInputPrevStep() const		{ return true; }
    virtual bool		prefersBinIDWise() const	{ return true; }

    virtual bool		prepareComp(int nrthreads);
    virtual bool		computeBinID(const BinID&,int);
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						const TrcKeyZSampling&) const;

    EM::Region3D&		region_;
    float			insideval_;
    float			outsideval_;

    float			startval_;
    float			gradval_;
    DBKey			startvalkey_;
    DBKey			gradvalkey_;
    int				startvalauxidx_;
    int				gradvalauxidx_;

};

} // namespace VolProc
