#ifndef waveletmanager_h
#define waveletmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "wavelet.h"
#include "objectset.h"
#include "multiid.h"
#include "uistring.h"
class IOObj;
class IOObjContext;
class WaveletSaver;
class WaveletManager;


/*!\brief access to the singleton Wavelet Manager */

inline WaveletManager& WaveletMGR();


/*!\brief Manages all stored Wavelets.

 If a wavelet is not yet loaded, then it will be loaded by fetch().

 Typical code for read:

 uiRetVal retval;
 ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( id, retval );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

 Typical code for write:

 RefMan<Wavelet> wvlt = new Wavelet;
 fillWvlt( *wvlt );
 uiRetVal retval = WaveletMGR().store( *wvlt, id );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

*/

mExpClass(Seis) WaveletManager : public Monitorable
{ mODTextTranslationClass(WaveletManager)
public:

    typedef MultiID		WvltID;

    ConstRefMan<Wavelet>	fetch(const WvltID&,uiRetVal&) const;
    RefMan<Wavelet>		fetchForEdit(const WvltID&,uiRetVal&);
    ConstRefMan<Wavelet>	fetch(const WvltID&) const;
    RefMan<Wavelet>		fetchForEdit(const WvltID&);

    bool		nameExists(const char*) const;
    bool		canSave(const WvltID&) const;
    uiRetVal		store(const Wavelet&,const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Wavelet&,const WvltID&,
				const IOPar* ioobjpars=0) const;
    uiRetVal		save(const Wavelet&) const;
    uiRetVal		save(const WvltID&) const;
    uiRetVal		saveAs(const WvltID& curid,const WvltID& newid) const;

    bool		isLoaded(const char*) const;
    bool		isLoaded(const WvltID&) const;
    WvltID		getID(const char*) const;
    WvltID		getID(const Wavelet&) const;
    IOPar		getIOObjPars(const WvltID&) const;

			// Use MonitorLock when iterating
    int			size() const;
    ConstRefMan<Wavelet> get(int) const;
    RefMan<Wavelet>	getForEdit(int);
    WvltID		getID(int) const;
    IOPar		getIOObjPars(int) const;

    bool		isScaled(const WvltID&) const;
    bool		getScalingInfo(const WvltID&,WvltID& orgid,
			    MultiID& horid,MultiID& seisid,
			    BufferString& lvlnm) const;
			//!< returns false if unscaled
			//!< orgid == udf when external scaling
    void		setScalingInfo(const WvltID&,
			    const WvltID* orgid=0,const MultiID* horid=0,
			    const MultiID* seisid=0,const char* lvlnm=0);
			//!< orgid == null => unscaled
			//!< *orgid == udf => external scaling

    CNotifier<WaveletManager,WvltID>	WaveletAdded;

protected:

				    WaveletManager();
				    ~WaveletManager();

    ObjectSet<WaveletSaver> savers_;

			// Tools for locked state
    void		setEmpty();
    int			gtIdx(const char*) const;
    int			gtIdx(const WvltID&) const;
    int			gtIdx(const Wavelet&) const;
    Wavelet*		gtWavelet(const WvltID&) const;
    template<class RT,class ST> RT doFetch(const WvltID&,uiRetVal&) const;
    uiRetVal		doSave(const WvltID&) const;

    void		add(const Wavelet&,const WvltID&,
			    const IOPar*,bool) const;

    void		addCBsToWvlt(const Wavelet&);
    void		iomEntryRemovedCB(CallBacker*);
    void		survChgCB(CallBacker*);
    void		wvltDelCB(CallBacker*);

    const IOObjContext&	ctxt_;
    IOObj*		getIOObj(const char*) const;

public:

    static WaveletManager& getInstance();
    mDeclInstanceCreatedNotifierAccess(WaveletManager);
    WaveletManager*	clone() const		{ return 0; }
    friend class	WaveletLoader;

};


inline WaveletManager& WaveletMGR()
{
    return WaveletManager::getInstance();
}


#endif
