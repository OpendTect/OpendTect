#ifndef uinlapartserv_h
#define uinlapartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uinlapartserv.h,v 1.6 2004-02-17 16:22:34 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "nlamodel.h"
#include "bufstringset.h"
class IOPar;
class UserIDSet;
class FeatureSet;
class NLACreationDesc;


/*! \brief Service provider for application level - Non-Linear Analysis

Will pop up the an NLA manage window on go(). If go() returns true, the user
will expect that go() to be called again.

 */

class uiNLAPartServer : public uiApplPartServer
{
public:
			uiNLAPartServer(uiApplService&);
    virtual		~uiNLAPartServer();

    const char*		name() const			= 0;
    virtual MultiID	modelId() const			= 0;
    virtual void	reset()				= 0;
    virtual bool	isClassification() const	= 0;
    virtual void	getNeededStoredInputs(
			      const BufferStringSet& ioobjnms,
			      TypeSet<int>&) const	= 0;
    virtual bool	go()				= 0;
    			//!< returns whether manageNN should be called again
    virtual const NLAModel& getModel() const		= 0;
    virtual const NLACreationDesc& creationDesc() const	= 0;

    virtual const char*	modelName() const
			{ return getModel().name(); }
    virtual IOPar&	modelPars() const
			{ return const_cast<NLAModel&>(getModel()).pars(); }

    static const int	evPrepareWrite;
    			//!< need to fill modelPars()
    static const int	evPrepareRead;
    			//!< is FYI
    static const int	evReadFinished;
    			//!< is FYI
    static const int	evGetInputNames;
    			//!< need to fill inputNames()
    static const int	evGetStoredInput;
			//!< need to put stored data into attrset
    static const int	evGetData;
    			//!< need to fill fsTrain() and fsTest()
    static const int	evSaveMisclass;
    			//!< use misclass analysis FS; user wants it.
    static const int	evCreateAttrSet;
    			//!< create attributeset from GDI NN

    			// Following should be filled on events
    BufferStringSet&	inputNames()			{ return inpnms; }
    FeatureSet&		fsTrain()			{ return fstrain; }
    FeatureSet&		fsTest()			{ return fstest; }
    FeatureSet&		fsMCA()				{ return fsmc; }

    virtual bool	fillPar(IOPar&) const			= 0;
    virtual void	usePar(const IOPar&)			= 0;

protected:

    FeatureSet&		fstrain;
    FeatureSet&		fstest;
    FeatureSet&		fsmc;
    BufferStringSet	inpnms;

};


/*!\mainpage Non-Linear Analysis User Interface

  This class was designed in such a way that the existing dGB Neural network
  module could be put in a plugin in an easy way. In that way we kept the
  possibility to make some money by selling our neural network stuff and
  still make OpendTect free software.

  Without this small sacrifice I don't think OpendTect would have ever started.
  We tried to keep the interface as general as possible, though. So it's very
  well possible and feasible (frighteningly!) to make your own special
  analysis module.
  */


#endif
