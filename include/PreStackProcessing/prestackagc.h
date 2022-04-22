#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "ranges.h"

namespace PreStack
{

/*!
\brief Processor for PreStack %AGC
*/

mExpClass(PreStackProcessing) AGC : public Processor
{ mODTextTranslationClass(AGC)
public:
				mDefaultFactoryInstantiation(
					Processor, AGC, "AGC", 
					toUiString(sFactoryKeyword()))

				AGC();
    bool			prepareWork() override;

    void			setWindow(const Interval<float>&);
    const Interval<float>&	getWindow() const;
    void			getWindowUnit(BufferString&,
					      bool withparens) const;

    void			setLowEnergyMute(float fraction);
    float			getLowEnergyMute() const;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    static const char*		sKeyWindow();
    static const char*		sKeyMuteFraction();

protected:
    bool			doWork(od_int64,od_int64,int) override;
    od_int64			totalNr() const override { return totalnr_; }

    Interval<float>		window_;
    Interval<int>		samplewindow_;
    float			mutefraction_;
    int				totalnr_;
};

} // namespace PreStack

