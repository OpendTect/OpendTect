#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "dbkey.h"
#include "seistype.h"
#include "bin2d.h"
#include "threadlock.h"
#include "zsubsel.h"

class BinnedValueSet;
class CubeHorSubSel;
class CubeSubSel;
class LineHorSubSel;
class LineSubSel;
class LineSubSelSet;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcInfo;
class SeisTrcTranslator;
class TraceData;
class TrcKey;
namespace PosInfo { class CubeData; class LineData; class LineCollData; }


namespace Seis
{

class ObjectSummary;
class Provider2D; class Provider3D;
class RawTrcsSequence;
class SelData;
class Fetcher; class Fetcher2D; class Fetcher3D;
class VolFetcher; class LineFetcher; class PS2DFetcher; class PS3DFetcher;


/*!\brief is the access point for seismic traces. Instantiate a subclass and ask
  for what you need. The class can fetch multi-threaded, it is (should be)
  MT-safe.

 After instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components of the seismic object.

 By default, you will get all stored components. If you want just one,
 use selectComponent(). You can have the data resampled; just use
 setSampleInterval().

 You can get all components in a gather, or the entire gather in one trace as
 components. Just call the corresponding get or getNext function.

 Note: the getNext() function may return !isOK(). At end of input, this will
 return the special 'error' 'Finished' which can be checked by
 isFinished( uirv ).

  */


mExpClass(Seis) Provider
{ mODTextTranslationClass(Seis::Provider);
public:

    mUseType( Pos,		GeomID );
    mUseType( Pos,		ZSubSel );
    mUseType( PosInfo,		LineCollData );
    typedef int			idx_type;
    typedef int			size_type;
    typedef float		z_type;
    typedef od_int64		totsz_type;
    typedef idx_type		comp_idx_type;
    typedef TypeSet<comp_idx_type> comp_idx_set_type;
    typedef SeisTrcTranslator	STTrl;

    static Provider*	create(GeomType);
    static Provider*	create(const DBKey&,uiRetVal* uirv=0);
    static Provider*	create(const IOObj&,uiRetVal* uirv=0);
    static Provider*	create(const IOPar&,uiRetVal* uirv=0);
    static DBKey	dbKey(const IOPar&);

    virtual		~Provider();

    virtual GeomType	geomType() const		= 0;
    Provider2D*		as2D();
    const Provider2D*	as2D() const;
    Provider3D*		as3D();
    const Provider3D*	as3D() const;

    uiRetVal		setInput(const DBKey&);
    uiRetVal		reset()		{ return setInput(dbKey()); }

    bool		is2D() const	{ return Seis::is2D(geomType()); }
    bool		isPS() const	{ return Seis::isPS(geomType()); }
    const DBKey&	dbKey() const;
    const IOObj*	ioObj() const	{ return ioobj_; }
    BufferString	name() const;

    const LineCollData&	possiblePositions() const { return possiblepositions_; }
    void		getComponentInfo(BufferStringSet&,DataType* dt=0) const;
    size_type		nrOffsets() const { return gtNrOffsets(); }
				//!< can vary; returned from a central location
    virtual size_type	nrGeomIDs() const
				{ return 1; }
    virtual GeomID	geomID( idx_type iln=0 ) const
				{ return GeomID::get3D(); }
    const ZSubSel&	zSubSel( idx_type iln=0 ) const
				{ return zsubsels_[iln]; }
    ZSampling		zRange( idx_type iln=0 ) const
				{ return zSubSel(iln).outputZRange(); }
    void		setZRange(const ZSampling&,idx_type iln=0);

    void		setSelData(const SelData&);
    void		selectComponent(comp_idx_type);
    void		selectComponents(const comp_idx_set_type&);
    void		forceFPData(bool yn=true);
    void		setReadMode(ReadMode);
    const comp_idx_set_type& selectedComponents() const
						{ return selectedcomponents_; }
    bool		haveSelComps() const;
    void		fillPar(IOPar&) const;
    uiRetVal		usePar(const IOPar&);

    bool		isPresent(const TrcKey&) const;
    void		getCurPosition(TrcKey&) const;

    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		getNextGather(SeisTrcBuf&) const;
    uiRetVal		getNextSequence(RawTrcsSequence&) const;

    bool		goTo(const TrcKey&) const;
    uiRetVal		getCurrent(SeisTrc&) const;
    uiRetVal		getCurrentGather(SeisTrcBuf&) const;
    uiRetVal		getAt(const TrcKey&,SeisTrc&) const;
    uiRetVal		getGatherAt(const TrcKey&,SeisTrcBuf&) const;

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }

    static const char*	sKeyForceFPData()
			{ return "Force FPs"; }

protected:

    typedef TypeSet<ZSubSel> ZSubSelSet;
    typedef IJPos	SPos;
    enum WorkState	{ NeedInput, NeedPrep, Active };

			Provider(bool is2d);

    IOObj*		ioobj_			= nullptr;
    LineCollData&	possiblepositions_;
    BinnedValueSet*	selectedpositions_	= nullptr;
    ZSubSelSet		zsubsels_;
    comp_idx_set_type	selectedcomponents_;
    ReadMode		readmode_		= Prod;
    bool		forcefpdata_		= false;
    virtual Fetcher&	gtFetcher()		= 0;
    Fetcher&		fetcher()		{ return gtFetcher(); }
    const Fetcher&	fetcher() const		{ return mSelf().gtFetcher(); }

    mutable Threads::Lock lock_;
    mutable WorkState	state_			= NeedInput;
    SPos		spos_;
    mutable Threads::Atomic<totsz_type> nrdone_	= 0;
    totsz_type		totalnr_;

    void		prepWork(uiRetVal&) const;
    virtual size_type	gtNrOffsets() const			{ return 1; }

    virtual void	gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const	{}
    virtual void	gtGather(SeisTrcBuf&,uiRetVal&) const	{}

    bool		prepGoTo(uiRetVal*) const;
    void		handleNewPositions();

private:

    void		reportSetupChg();
    void		ensureSelectedPositions();
    bool		prepareAccess(uiRetVal&) const;
    bool		doGoTo(const IdxPair&) const;
    void		wrapUpGet(TraceData&,uiRetVal&) const;
    void		wrapUpGet(SeisTrc&,uiRetVal&) const;
    void		wrapUpGet(SeisTrcBuf&,uiRetVal&) const;
    void		fillSequence(RawTrcsSequence&,uiRetVal&) const;
    uiRetVal		doGetTrc(SeisTrc&,bool next) const;
    uiRetVal		doGetGath(SeisTrcBuf&,bool next) const;

    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	Fetcher3D;
    friend class	VolFetcher;
    friend class	LineFetcher;
    friend class	PS2DFetcher;
    friend class	PS3DFetcher;

public:

    static void		putTraceInGather(const SeisTrc&,SeisTrcBuf&);
				//!< components become offsets 0, 100, 200, ...
    static void		putGatherInTrace(const SeisTrcBuf&,TraceData&,
					 SeisTrcInfo&);
				//!< offsets become components
    static void		getFallbackComponentInfo(BufferStringSet&,DataType&);

    const STTrl*	curTransl() const;

};


/*!\brief base class for Providers for 3D data. Extends Provider with some
  3D specific services. */


mExpClass(Seis) Provider3D : public Provider
{ mODTextTranslationClass(Seis::Provider3D);
public:

    mUseType( PosInfo,	CubeData );

    void	getCubeData(CubeData&) const;

    bool	isPresent(const BinID&) const;
    BinID	curBinID() const	    { return trcpos_; }

    bool	goTo(const BinID&,uiRetVal* uirv=0) const;

    uiRetVal	getAt(const BinID&,SeisTrc&) const;
    uiRetVal	getGatherAt(const BinID&,SeisTrcBuf&) const;

protected:

			Provider3D() : Provider(false)		{}

    BinID		trcpos_;

private:

    friend class	Provider;
    friend class	Fetcher;
    friend class	Fetcher3D;
    friend class	VolFetcher;
    friend class	PS3DFetcher;

};


/*!\brief base class for Providers for 2D data. Extends Provider with some
  2D specific services. */

mExpClass(Seis) Provider2D : public Provider
{ mODTextTranslationClass(Seis::Provider2D);
public:

    mUseType( PosInfo, LineData );

    bool	isPresent(GeomID) const;
    bool	isPresent(const Bin2D&) const;
    size_type	nrLines() const;
    size_type	nrGeomIDs() const override	{ return nrLines(); }
    GeomID	geomID(idx_type) const override;
    idx_type	lineIdx(GeomID) const;
    void	getLineData(idx_type,LineData&) const;
    BufferString lineName( idx_type iln ) const	{ return geomID(iln).name(); }
    Bin2D	curBin2D() const		{ return trcpos_; }

    bool	goTo(const Bin2D&,uiRetVal* uirv=0) const;
    uiRetVal	getAt(const Bin2D&,SeisTrc&) const;
    uiRetVal	getGatherAt(const Bin2D&,SeisTrcBuf&) const;

    const LineHorSubSel& lineHorSubSel(idx_type) const;
    const LineSubSel& lineSubSel(idx_type) const;
    const LineSubSelSet& lineSubSelSet() const;

protected:

			Provider2D() : Provider(true)		{}

    Bin2D		trcpos_;

private:

    friend class	Provider;
    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	LineFetcher;
    friend class	PS2DFetcher;

};


} // namespace Seis
