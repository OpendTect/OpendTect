#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "dbkey.h"
#include "geomid.h"
#include "seistype.h"
#include "threadlock.h"
#include "zsubsel.h"

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
namespace Survey { class GeomSubSel; class HorSubSel; }
namespace PosInfo { class CubeData; class CubeDataPos;
		    class Line2DData; class Line2DDataSet; }


namespace Seis
{

class Fetcher;
class Fetcher2D;
class Fetcher3D;
class ObjectSummary;
class Provider2D;
class Provider3D;
class RawTrcsSequence;
class SelData;


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
    mUseType( Survey,		HorSubSel );
    mUseType( Survey,		GeomSubSel );
    typedef int			idx_type;
    typedef int			size_type;
    typedef float		z_type;
    typedef od_int64		totsz_type;
    typedef idx_type		comp_idx_type;
    typedef TypeSet<comp_idx_type> comp_idx_set_type;

    static Provider*	create(GeomType);
    static Provider*	create(const DBKey&,uiRetVal* uirv=0);
    static Provider*	create(const IOObj&,uiRetVal* uirv=0);
    static Provider*	create(const IOPar&,uiRetVal* uirv=0);
    static DBKey	dbKey(const IOPar&);
    virtual		~Provider();
    Provider2D*		as2D();
    const Provider2D*	as2D() const;
    Provider3D*		as3D();
    const Provider3D*	as3D() const;

    uiRetVal		setInput(const DBKey&);
    uiRetVal		reset()			{ return setInput(dbky_); }

    virtual GeomType	geomType() const	= 0;
    bool		is2D() const	{ return Seis::is2D(geomType()); }
    bool		isPS() const	{ return Seis::isPS(geomType()); }
    const DBKey&	dbKey() const		{ return dbky_; }
    const IOObj*	ioObj() const		{ return ioobj_; }
    BufferString	name() const		{ return dbky_.name(); }

    virtual size_type	nrGeomIDs() const	{ return 1; }
    virtual GeomID	geomID( idx_type idx=0 ) const
						{ return GeomID::get3D(); }
    virtual GeomID	curGeomID() const	{ return geomID(0); }
    virtual idx_type	indexOf( GeomID ) const	{ return 0; }
    void		getCurPosition(TrcKey&) const;

    bool		isPresent(const TrcKey&) const;
    void		getComponentInfo(BufferStringSet&,DataType* dt=0) const;
    size_type		nrOffsets() const { return gtNrOffsets(); }
				//!< can vary; returned from a central location

    const HorSubSel&	horSubSel(idx_type idx=0) const;
    const ZSubSel&	zSubSel(idx_type idx=0) const;
    ZSampling		zRange(idx_type idx=0) const
			{ return zSubSel(idx).outputZRange(); }
    const GeomSubSel&	geomSubSel(idx_type idx=0) const;

    void		setSelData(SelData*); //!< becomes mine
    void		selectComponent(comp_idx_type);
    void		selectComponents(const comp_idx_set_type&);
    void		forceFPData(bool yn=true);
    void		setReadMode(ReadMode);
    void		fillPar(IOPar&) const;
    uiRetVal		usePar(const IOPar&);

    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		getNextGather(SeisTrcBuf&) const;
    uiRetVal		getNextSequence(RawTrcsSequence&) const;

    bool		atValidPos() const;
    bool		goTo(const TrcKey&,uiRetVal* uirv=0) const;
    uiRetVal		getCurrent(SeisTrc&) const;
    uiRetVal		getCurrentGather(SeisTrcBuf&) const;

    uiRetVal		getAt(const TrcKey&,SeisTrc&) const;
    uiRetVal		getGatherAt(const TrcKey&,SeisTrcBuf&) const;

    const comp_idx_set_type& selectedComponents() const	{ return selcomps_; }
    bool		haveSelComps() const;

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;

    static const char*	sKeyForceFPData()
			{ return "Force FPs"; }

    static void		putTraceInGather(const SeisTrc&,SeisTrcBuf&);
			//!< components become offsets 0, 100, 200, ...
    static void		putGatherInTrace(const SeisTrcBuf&,SeisTrc&);
			//!< offsets become components
    static void		getFallbackComponentInfo(BufferStringSet&,DataType&);

    const SelData*	selData() const		{ return seldata_; }

protected:

    enum WorkState	{ NeedInput, NeedPrep, Active };

			Provider()		{}

    mutable Threads::Lock lock_;
    mutable WorkState	state_			= NeedInput;
    DBKey		dbky_;
    IOObj*		ioobj_			= nullptr;
    SelData*		seldata_		= nullptr;
    comp_idx_set_type	selcomps_;
    ReadMode		readmode_		= Prod;
    bool		forcefpdata_		= false;
    mutable Threads::Atomic<totsz_type> nrdone_	= 0;

    size_type		inpnrcomps_		= 1;
    size_type		inpnroffsets_		= 1;
    totsz_type		totalnr_		= -1;

    virtual void	establishGeometry(uiRetVal&) const	= 0;
    virtual void	scanPositions() const			= 0;
    virtual bool	gtAtValidPos() const			= 0;
    virtual bool	moveToNextPosition(uiRetVal&) const	= 0;
    virtual void	prepWork(uiRetVal&) const		= 0;
    virtual size_type	gtNrOffsets() const			{ return 1; }
    virtual void	gtComponentInfo( BufferStringSet& s, DataType& d ) const
			{ return getFallbackComponentInfo( s, d ); }

    virtual void	gtCur(SeisTrc&,uiRetVal&) const		{}
    virtual void	gtCurGather(SeisTrcBuf&,uiRetVal&) const {}

    void		ensurePositionsScanned() const;
    bool		prepGoTo(uiRetVal*) const;

private:

    friend class	Fetcher;

    void		reportSetupChg();
    bool		prepareAccess(uiRetVal&) const;
    void		wrapUpGet(TraceData&,uiRetVal&) const;
    void		wrapUpGet(SeisTrc&,uiRetVal&) const;
    void		wrapUpGet(SeisTrcBuf&,uiRetVal&) const;
    void		fillSequence(RawTrcsSequence&,uiRetVal&) const;
    uiRetVal		getTrc(SeisTrc&,bool) const;
    uiRetVal		getGath(SeisTrcBuf&,bool) const;
    void		getSingleAt(const TrcKey&,TraceData&,SeisTrcInfo&,
				    uiRetVal&) const;
    Fetcher&		gtFetcher() const;

public:

    virtual const SeisTrcTranslator*	curTransl() const	{ return 0; }

};


/*!\brief base class for Providers for 3D data. Extends Provider with some
  3D specific services. */


mExpClass(Seis) Provider3D : public Provider
{ mODTextTranslationClass(Seis::Provider3D);
public:

    mUseType( PosInfo,	CubeData );
    mUseType( PosInfo,	CubeDataPos );

    bool	isPresent(const BinID&) const;
    void	getGeometryInfo(CubeData&) const;
    bool	goTo(const BinID&,uiRetVal* uirv=0) const;
    BinID	curBinID() const;

    const CubeHorSubSel& cubeHorSubSel() const;
    const CubeSubSel& cubeSubSel() const;

    uiRetVal	getAt(const BinID&,SeisTrc&) const;
    uiRetVal	getGatherAt(const BinID&,SeisTrcBuf&) const;

protected:

			Provider3D();
			~Provider3D();

    CubeData&		cubedata_;
    CubeSubSel&		css_;
    CubeDataPos&	cdp_;

    void		establishGeometry(uiRetVal&) const override;
    void		scanPositions() const override;
    bool		moveToNextPosition(uiRetVal&) const override;
    bool		gtAtValidPos() const override;

    virtual Fetcher3D&	fetcher() const				= 0;
    virtual void	getLocationData(uiRetVal&) const	= 0;
    virtual bool	doGoTo(const BinID&,uiRetVal*) const	= 0;
    virtual void	gtAt(const BinID&,TraceData&,
			     SeisTrcInfo&,uiRetVal&) const	{}
    virtual void	gtGatherAt(const BinID&,SeisTrcBuf&,
			      uiRetVal&) const			{}
	// implement gtCur and gtAt OR gtCurGather and gtGatherAt

    int			gtSelRes(const CubeDataPos&) const;

    friend class	Provider;
    friend class	Fetcher3D;

};


/*!\brief base class for Providers for 2D data. Extends Provider with some
  2D specific services. */

mExpClass(Seis) Provider2D : public Provider
{ mODTextTranslationClass(Seis::Provider2D);
public:

    mUseType( PosInfo,	Line2DData );
    mUseType( PosInfo,	Line2DDataSet );
    typedef int		trcnr_type;

    bool	isPresent(GeomID) const;
    bool	isPresent(GeomID,trcnr_type) const;

    size_type	nrGeomIDs() const override;
    GeomID	geomID(idx_type) const override;
    idx_type	indexOf(GeomID) const override;
    GeomID	curGeomID() const override	{ return geomID(lineidx_); }
    trcnr_type	curTrcNr() const;

    size_type	nrLines() const			{ return nrGeomIDs(); }
    idx_type	curLineIdx() const		{ return lineidx_; }
    idx_type	lineNr( GeomID gid ) const	{ return indexOf(gid); }
    void	getGeometryInfo(idx_type,Line2DData&) const;
    BufferString lineName( idx_type iln ) const	{ return geomID(iln).name(); }

    uiRetVal	getAt(GeomID,trcnr_type,SeisTrc&) const;
    uiRetVal	getGatherAt(GeomID,trcnr_type,SeisTrcBuf&) const;
    bool	goTo(GeomID,trcnr_type,uiRetVal* uirv=0) const;

    const LineHorSubSel& lineHorSubSel(idx_type) const;
    const LineSubSel& lineSubSel(idx_type) const;
    const LineSubSelSet& lineSubSelSet() const;

protected:

			Provider2D();
			~Provider2D();

    Line2DDataSet&	l2dds_;
    LineSubSelSet&	lsss_;
    mutable idx_type	lineidx_		= 0;
    mutable idx_type	trcidx_			= 0;

    void		establishGeometry(uiRetVal&) const override;
    void		scanPositions() const override;
    bool		gtAtValidPos() const override;
    bool		moveToNextPosition(uiRetVal&) const override;

    virtual Fetcher2D&	fetcher() const				= 0;
    virtual bool	doGoTo(GeomID,trcnr_type,uiRetVal*) const = 0;
    virtual void	gtAt(GeomID,trcnr_type,TraceData&,
			     SeisTrcInfo&,uiRetVal&) const	{}
    virtual void	gtGatherAt(GeomID,trcnr_type,SeisTrcBuf&,
				    uiRetVal&) const		{}
	// implement gtCur and gtAt OR gtCurGather and gtGatherAt

    void		fillLineData(uiRetVal&) const;
    trcnr_type		trcNrAt(idx_type,idx_type) const;
    int			gtSelRes(idx_type,idx_type) const;

    friend class	Provider;
    friend class	Fetcher2D;

};


} // namespace Seis
