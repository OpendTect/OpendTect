#ifndef volprocregionfiller_h
#define volprocregionfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
 RCS:		$Id: volprocregionfiller.h 36409 2014-09-13 21:11:44Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "volprocchain.h"
#include "arrayndimpl.h"
#include "coord.h"

namespace EM { class Region3D; }

namespace VolProc
{

/*!
\brief Region filler
*/

mExpClass(VolumeProcessing) RegionFiller : public Step
{ mODTextTranslationClass(RegionFiller);
public:
				mDefaultFactoryInstantiation( VolProc::Step,
				    RegionFiller, "RegionFiller",
				    tr("Region painter") )

				RegionFiller();
				~RegionFiller();

    bool			needsInput() const		{ return false;}
    bool			isInputPrevStep() const		{ return true; }
    bool			needsFullVolume() const		{ return false;}
    void			releaseData();

    void			setInsideValue(float);
    float			getInsideValue() const;
    void			setOutsideValue(float);
    float			getOutsideValue() const;

    float			getStartValue() const;
    const MultiID&		getStartValueHorizonID() const;
    int				getStartValueAuxDataIdx() const;
    float			getGradientValue() const;
    const MultiID&		getGradientHorizonID() const;
    int				getGradientAuxDataIdx() const;

    EM::Region3D&		region();
    const EM::Region3D&		region() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int nrthreads);
    bool			computeBinID(const BinID&,int);

    EM::Region3D&		region_;
    float			insideval_;
    float			outsideval_;

    float			startval_;
    float			gradval_;
    MultiID			startvalkey_;
    MultiID			gradvalkey_;
    int				startvalauxidx_;
    int				gradvalauxidx_;
};

} // namespace VolProc

#endif

