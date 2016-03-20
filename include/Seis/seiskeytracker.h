#ifndef seistrackrecord_h
#define seistrackrecord_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2016
________________________________________________________________________


-*/

#include "seiscommon.h"
#include "binid.h"
#include "manobjectset.h"
#include "od_iosfwd.h"
class ascbinistream;
class ascbinostream;


namespace Seis
{

/*!\brief a record of visited positions. */

mExpClass(Seis) TrackRecord
{
public:

    typedef Index_Type			IdxType;
    typedef od_int64			SeqNrType;

    struct Entry
    {
	enum Type	{ TStart, LStart, Stop, OffsChg };
	static const char* fileKey(Type);
	static Entry*	getFrom(ascbinistream&,bool is2d);

	Type		type_;
	IdxType		linenr_;
	IdxType		trcnr_;
	SeqNrType	seqnr_;

	inline bool	isOffs() const		{ return type_ == OffsChg; }
	inline bool	isStart() const		{ return type_ < Stop; }
	inline const char* fileKey() const	{ return fileKey( type_ ); }
	inline SeqNrType seqNr() const		{ return seqnr_; }
	inline IdxType	trcNr() const		{ return trcnr_; }
	inline IdxType	lineNr() const		{ return linenr_; }
	inline IdxType	crl() const		{ return trcnr_; }
	inline IdxType	inl() const		{ return linenr_; }
	inline BinID	binID() const		{ return BinID(linenr_,trcnr_);}
	inline bool	isTrcNrDir() const	{ return type_ != LStart; }

	bool		dump(ascbinostream&,bool is2d) const;

    protected:
			Entry( Type typ, SeqNrType seqnr, IdxType trcnr )
			    : type_(typ)
			    , seqnr_(seqnr)
			    , trcnr_(trcnr)
			    , linenr_(mUdf(int))    {}

    };

    typedef ManagedObjectSet<Entry>	EntrySet;
    typedef EntrySet::size_type		ArrIdxType;

			TrackRecord(Seis::GeomType);

    inline bool		is2D() const		{ return is2d_; }
    inline bool		isPS() const		{ return isps_; }

    inline void		setEmpty()		{ entries_.erase(); }
    inline EntrySet&	entries()		{ return entries_; }
    inline const EntrySet& entries() const	{ return entries_; }

    TrackRecord&	addStartEntry(SeqNrType,const BinID&,IdxType step,
				      bool diriscrl);
    inline TrackRecord&	addStartEntry( SeqNrType s, int trcnr, IdxType st,
					bool d )
		{ return addStartEntry(s,BinID(mUdf(IdxType),trcnr),st,d);}
    TrackRecord&	addEndEntry(SeqNrType,const BinID&);
    inline TrackRecord&	addEndEntry( SeqNrType s, int trcnr )
		{ return addEndEntry(s,BinID(mUdf(IdxType),trcnr));}
    TrackRecord&	addOffsetEntry(const BinID&,const TypeSet<float>&);
    inline TrackRecord&	addOffsetEntry( int t, const TypeSet<float>& o )
		{ return addOffsetEntry( BinID(mUdf(IdxType),t), o ); }

    bool		getFrom(od_istream&,bool binary);
    bool		dump(od_ostream&,bool binary) const;

protected:

    struct StartEntry : public Entry
    {
	IdxType		step_;
    protected:
			StartEntry( Type typ, SeqNrType seqnr,
				    IdxType trcnr, IdxType step )
			    : Entry(typ,seqnr,trcnr)
			    , step_(step)			{}
    };
    struct StartEntry2D : public StartEntry
    {
			StartEntry2D( SeqNrType seqnr, IdxType tnr,
				      IdxType step )
			    : StartEntry(TStart,seqnr,tnr,step)	{}
    };
    struct StartEntry3D : public StartEntry
    {
		    StartEntry3D( SeqNrType seqnr, IdxType inl, IdxType xln,
				  IdxType step, bool crldir )
			: StartEntry(crldir?TStart:LStart,seqnr,xln,step)
							    { linenr_ = inl; }
    };
    struct StopEntry : public Entry
    {
    protected:
			StopEntry( SeqNrType seqnr, IdxType trcnr )
			    : Entry(Stop,seqnr,trcnr)		{}
    };
    struct StopEntry2D : public StopEntry
    {
			StopEntry2D( SeqNrType seqnr, IdxType trcnr )
			    : StopEntry(seqnr,trcnr)		{}
    };
    struct StopEntry3D : public StopEntry
    {
			StopEntry3D( SeqNrType seqnr, IdxType inl, IdxType xln )
			    : StopEntry(seqnr,xln)	    { linenr_ = inl; }
    };
    struct OffsEntry : public Entry
    {
	TypeSet<float>	offsets_;
    protected:
			OffsEntry( IdxType trcnr )
			    : Entry(OffsChg,0,trcnr)		{}
    };
    struct OffsEntry2D : public OffsEntry
    {
			OffsEntry2D( IdxType trcnr )
			    : OffsEntry(trcnr)			{}
    };
    struct OffsEntry3D : public OffsEntry
    {
			OffsEntry3D( IdxType inl, IdxType xln )
			    : OffsEntry(xln)		{ linenr_ = inl; }
    };

    const bool		is2d_;
    const bool		isps_;
    EntrySet		entries_;

};


mExpClass(Seis) KeyTracker
{
public:

    typedef Index_Type	IdxType;
    typedef od_int64	SeqNrType;

			KeyTracker(TrackRecord&);
    virtual		~KeyTracker()		{ finish(); }
    const TrackRecord&	trackRecord() const	{ return trackrec_; }
    void		reset();

    inline bool		is2D() const		{ return trackrec_.is2D(); }
    inline bool		isPS() const		{ return trackrec_.isPS(); }

    void		add(int trcnr,float offs=0.f);
    void		add(const BinID&,float offs=0.f);
    SeqNrType		lastSeqNr() const	{ return seqnr_; }

    od_int64		nrDone() const		{ return seqnr_; };
    void		finish();
			//!< after call, trackrecord will not be used anymore
			//!< unless you call reset()

protected:

    TrackRecord&	trackrec_;

    od_int64		seqnr_;
    BinID		prevbid_;
    bool		diriscrl_;
    IdxType		step_;
    bool		finished_;
    int			offsidx_;
    TypeSet<float>	offsets_;
    bool		offsetschanged_;

    void		addFirst(const BinID&,float);
    void		addFirstFollowUp(const BinID&,float);
    void		addNext(const BinID&,float);
    void		addNextPS(const BinID&,float);
    void		checkCurOffset(float);
    void		getNewIncs(const BinID&);
    void		getNextPredBinID(BinID&) const;
    bool		isSamePos(const BinID&,const BinID&) const;
    void		addStartEntry(SeqNrType,const BinID&);
    void		addEndEntry(SeqNrType,const BinID&);
    void		addOffsetEntry();

};


} // namespace Seis

#endif
