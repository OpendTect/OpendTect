#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "uinlamod.h"
#include "uiapplserv.h"
#include "uistring.h"
#include "dbkey.h"
#include "nlamodel.h"
#include "bufstringset.h"
#include "uistring.h"
#include "ptrman.h"

class DataPointSet;
class DataPointSetDisplayMgr;
class uiDataPointSet;
class PosVecDataSet;
class NLACreationDesc;

/*!
\brief Service provider for application level - Non-Linear Analysis.

  Will pop up the an NLA manage window on go(). If go() returns true, the user
  will expect that go() to be called again.
*/

mExpClass(uiNLA) uiNLAPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiNLAPartServer)
public:
			uiNLAPartServer(uiApplService&);
    virtual		~uiNLAPartServer();

    const char*		name() const			= 0;
    virtual DBKey	modelId() const			= 0;
    virtual void	reset()				= 0;
    virtual bool	isClassification() const	= 0;
    virtual void	getNeededStoredInputs(
				BufferStringSet& linekeys) const = 0;
    virtual bool	go()				= 0;
			//!< returns whether manageNN should be called again
    virtual bool	doUVQ()				= 0;
    virtual const NLAModel* getModel() const		= 0;
    virtual const NLACreationDesc& creationDesc() const	= 0;

    virtual const char*	modelName() const;
    bool		willDoExtraction() const;

    static int		evPrepareWrite();
			//!< need to fill modelPars()
    static int		evPrepareRead();
			//!< is FYI
    static int		evReadFinished();
			//!< is FYI
    static int		evGetInputNames();
			//!< need to fill inputNames()
    static int		evGetStoredInput();
			//!< need to put stored data into attrset
    static int		evGetData();
			//!< need to fill vdsTrain() and vdsTest()
    static int		evSaveMisclass();
			//!< use misclass analysis VDS; user wants it.
    static int		evCr2DRandomSet();
			//!< create 2D random pick set
    static uiString	sKeyUsrCancel();
			//!< Returned when operation must stop without error

			// Following should be filled on events
    BufferStringSet&	inputNames()			{ return inpnms_; }
    const BufferStringSet& inputNames() const		{ return inpnms_; }
    RefMan<DataPointSet>	dps()			{ return gtDps(); }
    ConstRefMan<DataPointSet>	dps() const		{ return gtDps(); }
    IOPar&		storePars()			{ return storepars_; }
    const IOPar&	storePars() const		{ return storepars_; }

    void		setDPSDispMgr( DataPointSetDisplayMgr* dispmgr )
			{ dpsdispmgr_ = dispmgr; }

    virtual bool	fillPar(IOPar&) const		= 0;
    virtual void	usePar(const IOPar&)		= 0;

    void		getDataPointSets(ObjectSet<DataPointSet>&) const;
    uiString		prepareInputData(ObjectSet<DataPointSet>&);

    void		set2DEvent( bool is2d )		{ is2d_ = is2d; }
    bool		is2DEvent()			{ return is2d_; }


protected:

    RefMan<DataPointSet> dps_;
    BufferStringSet	inpnms_;
    bool		is2d_;
    IOPar&		storepars_;

    DataPointSetDisplayMgr* dpsdispmgr_;
    void		writeSets(CallBacker*);

    bool		extractDirectData(ObjectSet<DataPointSet>&);
    const uiString	convertToClasses(const ObjectSet<DataPointSet>&,int);
    bool		doDPSDlg();

    struct LithCodeData
    {
	TypeSet<int>	codes;
	TypeSet<int>	ptrtbl;
	TypeSet<int>	usedcodes;
	BufferStringSet	usednames;

	void		useUserSels(const BufferStringSet&);
	void		addCols(PosVecDataSet&,const char*);
	void		fillCols(PosVecDataSet&,const int);
    };

    RefMan<DataPointSet>	gtDps() const;

};
