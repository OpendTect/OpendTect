#ifndef uinlapartserv_h
#define uinlapartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uinlapartserv.h,v 1.1 2003-10-01 12:51:42 bert Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
#include <multiid.h>
class IOPar;
class FeatureSet;
class UserIDSet;


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
    virtual const char*	modelName() const		= 0;
    virtual IOPar&	modelPars() const		= 0;
    virtual void	reset()				= 0;
    virtual bool	isClassification()		= 0;
    virtual void	getNeededStoredInputs(
			      const UserIDSet& ioobjnms,
			      TypeSet<int>&) const	= 0;

    virtual bool	go()				= 0;
    			//!< returns whether manageNN should be called again

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
    ObjectSet<BufferString>& inputNames()		{ return inpnms; }
    FeatureSet&		fsTrain()			{ return fstrain; }
    FeatureSet&		fsTest()			{ return fstest; }
    FeatureSet&		fsMCA()				{ return fsmc; }

    virtual bool	fillPar(IOPar&) const			= 0;
    virtual void	usePar(const IOPar&)			= 0;

protected:

    FeatureSet&		fstrain;
    FeatureSet&		fstest;
    FeatureSet&		fsmc;
    ObjectSet<BufferString> inpnms;

};


#endif
