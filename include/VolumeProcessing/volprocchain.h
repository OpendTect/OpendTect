#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "dbkey.h"
#include "refcount.h"
#include "threadlock.h"

class Scaler;

namespace VolProc
{

/*!\brief A chain of Steps that can be applied to a volume of scalars.  */

mExpClass(VolumeProcessing) Chain : public RefCount::Referenced
{ mODTextTranslationClass(Chain);
public:
				Chain();

    mExpClass(VolumeProcessing) Connection
    {
    public:
			Connection( Step::ID outpstep=Step::cUndefID(),
			    Step::OutputSlotID outpslot=Step::cUndefSlotID(),
			    Step::ID inpstep=Step::cUndefID(),
			    Step::InputSlotID inpslot=Step::cUndefSlotID());

	bool		isUdf() const;
	bool		operator==(const Connection&) const;
	bool		operator!=(const Connection&) const;

	void		fillPar(IOPar&,const char* key) const;
	bool		usePar(const IOPar&,const char* key);

				//!Step that is receiving data
	Step::ID		inputstepid_;
	Step::InputSlotID	inputslotid_;

				//!Step that is sending data
	Step::ID		outputstepid_;
	Step::OutputSlotID	outputslotid_;
    };

    mExpClass(VolumeProcessing) Web
    {
    public:
	bool			addConnection(const Connection&);
	void			removeConnection(const Connection&);
	void			getConnections(Step::ID,bool input,
					       TypeSet<Connection>&) const;
				/*!Gets all connection that has step as either
				   input or output. */

	TypeSet<Connection>&		getConnections()
					{ return connections_; }
	const TypeSet<Connection>&	getConnections() const
					{ return connections_; }
    private:
	TypeSet<Connection>	connections_;
    };

    bool			addConnection(const Connection&);
    void			removeConnection(const Connection&);
    void			updateConnections();
    const Web&			getWeb() const	{ return web_; }

    int				nrSteps() const;
    Step*			getStep(int);
    Step*			getStepFromName(const char*);
    const Step*			getStepFromName(const char*) const;
    Step*			getStepFromID(Step::ID);
    const Step*			getStepFromID(Step::ID) const;
    int				indexOf(const Step*) const;
    void			addStep(Step*);
    void			insertStep(int,Step*);
    void			swapSteps(int,int);
    void			removeStep(int);
    const ObjectSet<Step>&	getSteps() const	{ return steps_; }
    int				getNrUsers(Step::ID,Step::InputSlotID) const;

    bool			setOutputSlot(Step::ID,Step::OutputSlotID);
    void			setOutputScalers(const ObjectSet<Scaler>&);
    const ObjectSet<Scaler>&	getOutputScalers() const
				{ return outcompscalers_; }

    const VelocityDesc*		getVelDesc() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			setStorageID(const DBKey& mid);
    const DBKey&		storageID() const { return storageid_; }
    uiString			name() const;

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;
    uiString			errMsg() const;

    Step::ID			getNewStepID() { return freeid_++; }

private:
				~Chain();

    friend			class ChainExecutor;

    bool			validConnection(const Connection&) const;

    static const char*		sKeyNrSteps()	    { return "Nr Steps"; }
    static const char*		sKeyStepType()	    { return "Type"; }
    static const char*		sKeyNrConnections() { return "Nr Connections"; }
    static const char*		sKeyConnection(int idx,BufferString&);

    Step::ID			outputstepid_;
    Step::OutputSlotID		outputslotid_;

    DBKey			storageid_;
    ObjectSet<Step>		steps_;
    Web				web_;
    ObjectSet<Scaler>		outcompscalers_;

    uiString			errmsg_;
    Threads::Atomic<int>	freeid_;

};


} // namespace VolProc
