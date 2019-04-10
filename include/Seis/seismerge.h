#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "samplingdata.h"
#include "seismultiprovider.h"

class Scaler;
class SeisTrc;
namespace Seis { class MultiProvider; class Storer; }


/*!\brief Merges 2D and 3D post-stack data */

mExpClass(Seis) SeisMerger : public Executor
{ mODTextTranslationClass(SeisMerger);
public:

    mUseType( Seis,	MultiProvider );
    mUseType( Seis,	Storer );

			SeisMerger(const ObjectSet<IOPar>& in,
				   const IOPar& out,bool stacktrcs,
				   Seis::MultiProvider::ZPolicy zpolicy);
			//!<\param stacktrcs If false,first trc will be used
			//!<\param zpolicy Specifies whether union or
			//!< intersection of z-ranges has to be considered.
    virtual		~SeisMerger();

    uiString		message() const;
    od_int64		nrDone() const		{ return nrpos_; }
    od_int64		totalNr() const		{ return totnrpos_; }
    uiString		nrDoneText() const	{
					    return tr("Positions handled");
						}
    int			nextStep();
    void		setScaler(Scaler*);

protected:

    bool		stacktrcs_;
    MultiProvider*	multiprov_		= nullptr;
    Storer*		storer_			= nullptr;
    int			nrpos_			= 0;
    od_int64		totnrpos_;
    uiString		errmsg_;

    int			nrsamps_;
    SamplingData<float>	sd_;
    Scaler*		scaler_			= nullptr;

    int			writeTrc(SeisTrc*);

};
