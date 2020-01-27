#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2008
________________________________________________________________________

-*/

#include "generalmod.h"
#include "binnedvalueset.h"
#include "color.h"
#include "datapackbase.h"
#include "valseries.h"
#include "bin2d.h"
class DataColDef;
class BinnedValueSet;
class PosVecDataSet;
class UnitOfMeasure;
class BufferStringSet;
class TaskRunnerProvider;
class TrcKeyZSampling;

namespace Pos { class Filter; class Provider; }


/*!\brief Set of data points with group selection.

  The idea is to generate a set of data points, which are then put in different
  groups. The data is transferred into a BinnedValueSet for performance,
  but a fixed int addressing becomes available.

  The design is based on the model that you extract data in some way which
  should then be quickly accessible. The data set is not meant to be
  manipulated other than via setSelected() and setGroup().

  For data associated with 2D seismics, you can specify that the DataPointSet
  needs to be 2D; then also 'trcNr() can be used.

  For large sets, you may not be interested in precise X and Y, and
  grouping/selection. Then specify the 'minimal' flag on construction.

*/

mExpClass(General) DataPointSet : public PointDataPack
{
public:

    typedef int			RowID;
    typedef int			ColID;
    mUseType( BinnedValueSet,	SPos );
    mUseType( Pos,		GeomID );

    class DataRow;

    /*!\brief Real Coord3D-position storable in BinnedValueSet + Bin2D */

    mExpClass(General) Pos
    {
    public:
			Pos()
			    : offsx_(0), offsy_(0), z_(0)	{}
			Pos( const BinID& bid, float _z )
			    : binid_(bid), z_(_z)
			    , offsx_(0), offsy_(0)		{}
			Pos(const Bin2D&,float);
			Pos(const Coord&,float _z);
			Pos(const Coord3&);

	bool		operator ==(const Pos& pos) const
			{ return binid_==pos.binid_ && offsx_ ==pos.offsx_
				&& offsy_==pos.offsy_ && z_==pos.z_; }
	const BinID&	binID() const	{ return binid_; }
	const Bin2D&	bin2D() const	{ return bin2d_; }
	Coord		coord() const;
	float		z() const	{ return z_; }

	void		set( const BinID& bid )
					{ binid_ = bid; offsx_ = offsy_ = 0; }
	void		set(const Coord&); //!< updates binid_
	void		set(const Coord3&); //!< updates binid_
	void		set( const Bin2D& b2d )
					{ set( b2d, b2d.coord() ); }
	void		set(const BinID& bid,const Coord&);
	void		set(const Bin2D&,const Coord&);
	void		setZ( float _z ) { z_ = _z; }

	float		binIDOffSet( bool inx ) const
			{ return inx ? offsx_ : offsy_; }
	void		setBinIDOffset( bool inx, float o )
			{ (inx ? offsx_ : offsy_) = o; }
	void		setBinIDOffsets( float ox, float oy )
			{ offsx_ = ox; offsy_ = oy; }

    protected:

	BinID		binid_;
	float		z_;
	float		offsx_, offsy_;
	Bin2D		bin2d_;		    //!< unused if not 2D

	void		setOffs(const Coord&);
	friend class	DataRow;

    };

    /*!\brief Data point with group. Group 0 means 'inactive',
	      it can never be selected. */

    mExpClass(General) DataRow
    {
    public:
			DataRow()
			    : grp_(1)			{ setSel(false); }
			DataRow( const Pos& p, od_uint16 grp=1,
				   bool issel=false )
			    : pos_(p), grp_((short)grp)
							{ setSel( issel ); }

	bool			operator ==(const DataRow& dr) const
				{ return pos_==dr.pos_ && grp_==dr.grp_
					&& data_==dr.data_; }
	const BinID&		binID() const		{ return pos_.binID(); }
	Coord			coord() const		{ return pos_.coord(); }
	const TypeSet<float>&	data() const		{ return data_; }
	od_uint16		group() const;
	bool			isSel() const		{ return grp_ > 0; }
	bool			isInactive() const	{ return grp_ == 0; }
	const Bin2D&		bin2D() const		{ return pos_.bin2D(); }
	void			setSel( bool yn )
				{ if ( (grp_ >= 0) != yn ) grp_ = -grp_; }
	void			setGroup(od_uint16);
	void			getBVSValues(TypeSet<float>&,bool is2d,
					     bool ismini) const;

	Pos		pos_;
	TypeSet<float>	data_;
	short		grp_;
    };

			DataPointSet(const TypeSet<DataRow>&,
				     const ObjectSet<DataColDef>&,
				     bool is2d,bool minimal=false);
			DataPointSet(const TypeSet<DataRow>&,
				     const BufferStringSet& valnms,
				     bool is2d,bool minimal=false);
			DataPointSet(const PosVecDataSet&,bool is2d,
					bool minimal=false);
			DataPointSet(const DataPointSet&,const ::Pos::Filter&);
			mDeclMonitorableAssignment(DataPointSet);

    OD::GeomSystem	geomSystem() const;
    bool		is2D() const override	{ return is2d_; }
    bool		isMinimal() const	{ return minimal_; }
    bool		isEmpty() const		{ return sposs_.isEmpty(); }
    void		setEmpty();
    void		clearData(); //!< Keeps structure

    int			nrCols() const;
    int			nrFixedCols() const	{ return nrfixedcols_; }
    const char*		colName(ColID) const;
    const UnitOfMeasure* unit(ColID) const;
    const DataColDef&	colDef( ColID i ) const	{ return gtColDef(i); }
    ColID		indexOf(const char*) const;
    bool		validColID(ColID) const;

			// size, binID, coord, z and trcNr impl PointDataPack
    int			size() const override	{ return sposs_.size(); }
    BinID		binID(RowID) const override;
    Bin2D		bin2D(RowID) const override;
    Coord		coord(RowID) const override;
    float		z(RowID) const override;
    linenr_type		lineNr(RowID) const override;
    trcnr_type		trcNr(RowID) const override;

    Pos			pos(RowID) const;
    DataRow		dataRow(RowID) const;
    float		value(ColID,RowID) const;
    bool		setValue(ColID,RowID,float);
    float*		getValues(RowID);
    const float*	getValues(RowID) const;
    od_uint16		group(RowID) const;
    bool		isSelected(RowID) const;
    int			selGroup(RowID) const;
    bool		isInactive( RowID rid ) const { return group(rid) == 0;}

    void		setGroup(RowID,od_uint16);
    void		setSelected(RowID,int selgrp);
    void		setInactive(RowID,bool);

    int			nrActive() const;
    void		purgeInactive();
    void		purgeSelected(bool selected_rows=true);

    RowID		find(const Pos&) const;
    RowID		find(const Pos&,float horradius,float deltaz) const;
    RowID		findFirst(const Coord&) const;
    RowID		findFirst(const BinID&) const;
    RowID		findFirst(const Bin2D&) const;

    const PosVecDataSet& dataSet() const		{ return data_; }
    const BinnedValueSet& bivSet() const { return const_cast<DataPointSet*>
					  (this)->bivSet(); }
    BinnedValueSet&	bivSet();
			//!< The idea is to manage vectors with the selection
			//!< mechanism. But if you really must remove
			//!< vectors, this may be your access point
    PosVecDataSet&	dataSet()		{ return data_; }
			//!< To add/remove columns. Never remove the position
			//!< columns!

    DataPointSet*	getSubselected(int maxsz,
			    const TypeSet<int>* selected_cols=0,
			    bool allow_udf_values=true,
			    const ObjectSet<Interval<float> >* value_ranges=0)
							const;
    void		randomSubselect(int maxsz);

    int			bivSetIdx( ColID idx ) const
						{ return idx+nrfixedcols_; }

    void		dataChanged()			{ calcIdxs(); }
			//!< When data modified, you want to call this.
			//!< all RowIDs may change
			//!< In case you want to change the definition of a col
    void		addRow(const DataRow&);
			//!< When finished, you have to call dataChanged()
    bool		setRow(const DataRow&);
			//!< Can be add or change
			//!< Returns whether it's an add (see addRow)
    RowID		getRowID(SPos) const;
    DataColDef&		colDef( ColID i )		{ return gtColDef(i); }
    SPos		bvsPos( RowID rid ) const	{ return sposs_[rid];}

			// Building from scratch
			DataPointSet(bool is2d,bool minimal=false);
    bool		extractPositions(::Pos::Provider&,
				     const ObjectSet<DataColDef>&,
				     const TaskRunnerProvider&,
				     const ::Pos::Filter* f=0,
				     bool filterAccept=true);
    void		addCol(const char* nm,const char* ref=0,
				const UnitOfMeasure* un=0);
			//!< or use dataSet() to add columns

    bool		simpleCoords() const		{ return minimal_; }

    void		dumpLocations(od_ostream* strm=0) const;

protected:

    virtual		~DataPointSet();

    PosVecDataSet&	data_;
    TypeSet<SPos>	sposs_;
    bool		is2d_;
    bool		minimal_;

    void		initPVDS();
    void		init(const TypeSet<DataRow>&,
			     const ObjectSet<DataColDef>&);
    void		calcIdxs();

    static const int	groupcol_;
    const int		nrfixedcols_;

    DataColDef&		gtColDef(ColID) const;

    virtual float	gtNrKBytes() const override;
    virtual void	doDumpInfo(IOPar&) const override;

};


/*!\brief ValueSeries based on DataPointSet */

mExpClass(General) DPSValueSeries : public ValueSeries<float>
{
public:

    typedef DataPointSet::ColID	ColID;
    typedef DataPointSet::RowID	RowID;

				DPSValueSeries( const DataPointSet& dps,
						ColID colid )
				    : dps_(dps), colid_(colid)
				{ dps_.ref(); }
				DPSValueSeries( const DPSValueSeries& oth )
				    : dps_(oth.dps_), colid_(oth.colid_)
				{ dps_.ref(); }
				~DPSValueSeries()
				{ dps_.unRef();}

    virtual ValueSeries<float>*	clone() const
				{ return new DPSValueSeries(*this); }
    virtual float		value( od_int64 idx ) const
				{ return dps_.value( colid_, (RowID)idx ); }
    od_int64			size() const override
				{ return dps_.size(); }

protected:

    const DataPointSet&		dps_;
    ColID			colid_;

};


/*!\brief Fills DataPointSet with data from a VolumeDataPack */

mExpClass(General) DPSFromVolumeFiller : public ParallelTask
{ mODTextTranslationClass(DPSFromVolumeFiller)
public:
				DPSFromVolumeFiller(DataPointSet&,int firstcol,
						    const VolumeDataPack&,
						    int component);
				~DPSFromVolumeFiller();

    virtual uiString		message() const;
    virtual uiString		nrDoneText() const;

    void			setSampling(const TrcKeyZSampling*);

protected:
    virtual od_int64		nrIterations() const;
    virtual bool		doWork(od_int64 start,od_int64 stop,int thridx);

    DataPointSet&		dps_;
    const VolumeDataPack&	vdp_;
    int				component_;
    int				firstcol_;

    bool			hastrcdata_;
    bool			hasstorage_;
    const TrcKeyZSampling*	sampling_;
};
