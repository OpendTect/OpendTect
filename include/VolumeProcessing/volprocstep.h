#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "factory.h"
#include "uistrings.h"
#include "survgeom.h"

class ProgressMeter;
class RegularSeisDataPack;
class ReportingTask;
class TrcKeyZSampling;
class VelocityDesc;
namespace PosInfo { class CubeData; }


namespace VolProc
{

class Chain;


/*!\brief An algorithm/calculation/transformation that takes one scalar volume
  as input, processes it, and puts the output in another volume.

  Every step will be part of a Chain, which will give the step its ID.

 */

mExpClass(VolumeProcessing) Step
{ mODTextTranslationClass(Step);
public:

    typedef ConstRefMan<RegularSeisDataPack> CVolRef;
    typedef RefMan<RegularSeisDataPack> VolRef;
    typedef int			ID;
    typedef int			InputSlotID;
    typedef int			OutputSlotID;
    static ID			cUndefID()		{ return mUdf(int); }
    static int			cUndefSlotID()		{ return mUdf(int); }

				mDefineFactoryInClass( Step, factory );
    virtual			~Step();
    virtual void		releaseData();
    virtual uiString		errMsg() const		{ return errmsg_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    ID				getID() const		{ return id_; }
    Chain&			getChain();
    const Chain&		getChain() const;
    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);

    void			resetInput();
    virtual int			getNrInputs() const;
    bool			validInputSlotID(InputSlotID) const;
    virtual InputSlotID		getInputSlotID(int idx) const;
    virtual void		getInputSlotName(InputSlotID,
						 BufferString&) const;

    virtual int			getNrOutputs() const		{ return 1; }
    bool			validOutputSlotID(OutputSlotID) const;
    virtual OutputSlotID	getOutputSlotID(int idx) const;

    virtual BinID		getHorizontalStepout() const
							{ return BinID(0,0); }
				/*!<How many extra samples are required in the
				    inline/crossline direction */
    virtual int			getVerticalStepout() const	{ return 0; }
				/*!<How many extra vertical samples are required
				     for the computation?*/
    const PosInfo::CubeData*	getPosSamplingOfNonNullTraces(InputSlotID,
							      int comp=0);

    virtual void		setInput(InputSlotID,
					 const RegularSeisDataPack*);
    CVolRef			getInput(InputSlotID) const;
    virtual void		setOutput(OutputSlotID,RegularSeisDataPack*);
    CVolRef			getOutput(OutputSlotID) const;
    VolRef			getOutput(OutputSlotID);

    int				getOutputIdx(OutputSlotID) const;
    void			enableOutput(OutputSlotID);

    CVolRef			getOutput() const	{ return output_; }
    VolRef			getOutput()		{ return output_; }

    TrcKeyZSampling		getInputSampling(const TrcKeyZSampling&) const;
    od_int64			getComponentMemory(const TrcKeyZSampling&,
						   bool input) const;

    od_int64			getExtraMemoryUsage(const TrcKeyZSampling&,
					const TypeSet<OutputSlotID>&
					=TypeSet<OutputSlotID>()) const;
				/*!< returns total amount of bytes needed
				     on top of the base consumption */

    virtual const VelocityDesc*	getVelDesc() const		{ return 0; }

    virtual ReportingTask*	createTask();
				/*!< You probably want to call prepareWork
				   to do sanity check and set your components
				   names
				 */

				// Processing properties
    virtual bool		needsFullVolume() const		= 0;
    virtual bool		canInputAndOutputBeSame() const	= 0;
    virtual bool		areSamplesIndependent() const	= 0;
    virtual bool		canHandle2D() const		{ return false;}
    virtual bool		needsInput() const		{ return true; }
    virtual bool		prefersBinIDWise() const	{ return false;}
    virtual bool		canHandleUndefs() const		{ return true; }
    virtual bool		isInputPrevStep() const
				{ return needsInput(); }

    virtual int			getNrInputComponents(InputSlotID,
						     Pos::GeomID) const;
    virtual int			getNrOutComponents(OutputSlotID,
						   Pos::GeomID) const;
				/*!< Should be filled when the step is setup,
				     so before a task can be created from it*/
    virtual bool		copyComponentsSel(const InputSlotID,
						  OutputSlotID&) const
				{ return false;}

    virtual bool		hasPeakMemoryAllocatedInput() const
				{ return needsInput(); }
				/*!< Are the input datapacks still allocated
				     during peak memory consumption? */

    virtual bool		hasPeakMemoryAllocatedOutput() const
				{ return true;}
				/*!< Are the output datapacks still allocated
				     during peak memory consumption? */

protected:

				Step();

    virtual bool		prepareWork(int nrthreads=1);

    // Only called if prefersBinIDWise() returns true
    virtual bool		computeBinID(const BinID&,int threadid)
								{ return false;}

    // The memory needed on top of the 'base' memory usage. Can be 0.
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						 const TrcKeyZSampling&) const
								= 0;

    ID				id_;
    Chain*			chain_;
    BufferString		username_;
    ObjectSet<const RegularSeisDataPack> inputs_;
    TypeSet<InputSlotID>	inputslotids_;
    TypeSet<int>		inputcompnrs_;
    uiString			errmsg_;
    TypeSet<OutputSlotID>	outputslotids_; // enabled slotids
    TypeSet<int>		outputcompnrs_;

private:

    RefMan<RegularSeisDataPack>	output_;

    void			setChain(Chain&);
    void			setInputNrComps(InputSlotID,int);

    friend class		Chain;
    friend class		ChainExecutor;
    friend class		BinIDWiseTask;

};


} // namespace VolProc
