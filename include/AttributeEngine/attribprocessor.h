#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "executor.h"
#include "binid.h"
#include "geomid.h"

class SeisTrcInfo;
namespace Seis { class SelData; }
namespace Survey { class FullSubSel; }

namespace Attrib
{
class DataHolder;
class Output;
class Provider;

/*!\brief Attribute Processor */

mExpClass(AttributeEngine) Processor : public Executor
{ mODTextTranslationClass(Processor)
public:

    mUseType( Pos,	GeomID );
    mUseType( Survey,	FullSubSel );

			Processor(Desc&,uiRetVal&,GeomID gid=GeomID());
			~Processor();

    bool		isOK() const;
    bool		is2D() const;
    void		addOutput(Output*);
    void		setGeomID(GeomID);

    int			nextStep();
    void		init();
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    uiString		message() const;
    uiString		nrDoneText() const { return tr("Positions processed"); }

    void		setDesiredSubSel(const FullSubSel&);
    void		addOutputInterest(int sel);
    bool		setZIntervals(TypeSet< Interval<int> >&,
				      const TrcKey&,const Coord&);
    void		computeAndSetRefZStepAndZ0();

    Notifier<Processor>	moveonly;
				/*!< triggered after a position is reached that
				     requires no processing, e.g. during initial
				     buffer fills. */

    const char*		getAttribName() const;
    const char*		getAttribUserRef() const;
    Provider*		getProvider()		{ return provider_; }
    ObjectSet<Output>	outputs_;

    void		setRdmPaths(const TypeSet<BinID>& truepath,
				    const TypeSet<BinID>& snappedpath);
				//for directional attributes

    void		showDataAvailabilityErrors(bool yn);

protected:

    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);

    void		defineGlobalOutputSpecs(TypeSet<int>&,FullSubSel&);
    void		prepareForTableOutput();
    void		computeAndSetPosAndDesSubSel(FullSubSel&);

    bool		isHidingDataAvailabilityError() const;

    Desc&		desc_;
    Provider*		provider_;
    int			nriter_;
    int			nrdone_;
    TypeSet<int>	outpinterest_;
    uiString		errmsg_;
    bool		isinited_;
    bool		useshortcuts_;

    BinID		prevbid_;
    Seis::SelData*	sd_;

    bool		showdataavailabilityerrors_;

};


} // namespace Attrib
