#ifndef datapointset_h
#define datapointset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2008
 RCS:		$Id: datapointset.h,v 1.42 2012/05/08 10:20:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "binidvalset.h"
#include "color.h"
#include "datapackbase.h"
class DataColDef;
class BinIDValueSet;
class PosVecDataSet;
class UnitOfMeasure;
class BufferStringSet;
namespace Pos { class Filter; class Provider; }


/*!\brief Set of data points with group selection.

  The idea is to generate a set of data points, which are then put in different
  groups. The data is transferred into a BinIDValueSet for performance,
  but a fixed int addressing becomes available.

  The design is based on the model that you extract data in some way which
  should then be quickly accessible. The data set is not meant to be
  manipulated other than via setSelected() and setGroup().

  For data associated with 2D seismics, you can specify that the DataPointSet
  needs to be 2D; then also 'trcNr() can be used.

  For large sets, you may not be interested in precise X and Y, and
  grouping/selection. Then specify the 'minimal' flag on construction.

*/

mClass DataPointSet : public PointDataPack
{
public:

    typedef int		RowID;
    typedef int		ColID;
    class		DataRow;

    /*!\brief Real Coord3D-position storable in BinIDValueSet + trc nr */

    mClass Pos
    {
    public:
			Pos() : offsx_(0), offsy_(0), z_(0), nr_(0)	{}
			Pos( const BinID& bid, float _z )
			    : binid_(bid), nr_(0), z_(_z)
			    , offsx_(0), offsy_(0)		{}
			Pos(const Coord&,float z);
			Pos(const Coord3&);

	bool		operator ==(const Pos& pos) const 
			{ return binid_==pos.binid_ && offsx_ ==pos.offsx_
			    	&& offsy_==pos.offsy_ && z_==pos.z_; }
	const BinID&	binID() const	{ return binid_; }
	Coord		coord() const;
	float		z() const	{ return z_; }

	void		set( const BinID& bid )
	    		{ binid_ = bid; offsx_ = offsy_ = 0; }
	void		set(const Coord&);
	void		set(const Coord3&);

	BinID		binid_;
	float		z_;
	int		nr_;			//!< unused if not 2D

	float		binIDOffSet( bool inx ) const
			{ return inx ? offsx_ : offsy_; }
	void		setBinIDOffset( bool inx, float o )
	    		{ (inx ? offsx_ : offsy_) = o; }
	void		setBinIDOffsets( float ox, float oy )
	    		{ offsx_ = ox; offsy_ = oy; }

    protected:

	float		offsx_, offsy_;
	void		setOffs(const Coord&);
	friend class	DataRow;

    };

    /*!\brief Data point with group. Group 0 means 'inactive',
      	      it can never be selected. */

    mClass DataRow
    {
    public:
			DataRow()
			    : grp_(1)			{ setSel(false); }
			DataRow( const Pos& p, unsigned short grp=1,
				   bool issel=false )
			    : pos_(p), grp_((short)grp)
							{ setSel( issel ); }

	bool			operator ==(const DataRow& dr) const 
				{ return pos_==dr.pos_ && grp_==dr.grp_
				    	&& data_==dr.data_; }
	const BinID&		binID() const		{ return pos_.binID(); }
	Coord			coord() const		{ return pos_.coord(); }
	const TypeSet<float>&	data() const		{ return data_; }
	unsigned short		group() const;
	bool			isSel() const		{ return grp_ > 0; }
	bool			isInactive() const	{ return grp_ == 0; }
	void			setSel( bool yn )
	    			{ if ( (grp_ >= 0) != yn ) grp_ = -grp_; }
	void			setGroup(unsigned short grp);
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
    			DataPointSet(::Pos::Provider&,
				     const ObjectSet<DataColDef>&,
				     const ::Pos::Filter* f=0,
				     bool minimal=false);
    			DataPointSet(const PosVecDataSet&,bool is2d,
					bool minimal=false);
    			DataPointSet(const DataPointSet&,const ::Pos::Filter&);
    			DataPointSet(const DataPointSet&);
    virtual		~DataPointSet();
    DataPointSet&	operator =(const DataPointSet&);
    bool		is2D() const		{ return is2d_; }
    bool		isMinimal() const	{ return minimal_; }
    bool		isEmpty() const		{ return bvsidxs_.isEmpty(); }
    void		setEmpty();
    void		clearData(); //!< Keeps structure

    int			nrCols() const;
    int			nrFixedCols() const	{ return nrfixedcols_; }
    const char*		colName(ColID) const;
    const UnitOfMeasure* unit(ColID) const;
    const DataColDef&	colDef( ColID i ) const	{ return gtColDef(i); }
    ColID		indexOf(const char*) const;

    			// size, binID, coord, z and trcNr impl PointDataPack
    int			size() const		{ return bvsidxs_.size(); }
    BinID		binID(RowID) const;
    Coord		coord(RowID) const;
    float		z(RowID) const;
    int			trcNr(RowID) const;

    Pos			pos(RowID) const;
    DataRow		dataRow(RowID) const;
    float		value(ColID,RowID) const;
    bool		setValue(ColID,RowID,float);
    float*		getValues(RowID);
    const float*	getValues(RowID) const;
    unsigned short	group(RowID) const;
    bool		isSelected(RowID) const;
    int			selGroup(RowID) const;
    bool		isInactive( RowID rid ) const { return group(rid) == 0;}

    void		setGroup(RowID,unsigned short);
    void		setSelected(RowID,int selgrp);
    void		setInactive(RowID,bool);

    int			nrActive() const;
    void		purgeInactive();
    void		purgeSelected(bool selected_rows=true);

    RowID		find(const Pos&) const;
    RowID		find(const Pos&,float horradius,float deltaz) const;
    RowID		findFirst(const Coord&) const;
    RowID		findFirst(const BinID&) const;

    const PosVecDataSet& dataSet() const		{ return data_; }
    const BinIDValueSet& bivSet() const { return const_cast<DataPointSet*>
					  (this)->bivSet(); }
    BinIDValueSet&	bivSet();
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
    RowID		getRowID(BinIDValueSet::Pos) const;
    DataColDef&		colDef( ColID i )		{ return gtColDef(i); }
    BinIDValueSet::Pos	bvsPos( RowID rid ) const	{ return bvsidxs_[rid];}
    			DataPointSet(bool is2d,bool minimal=false);
			//!< use dataSet() to add columns

    // DataPack interface impl
    bool		simpleCoords() const		{ return minimal_; }
    float		nrKBytes() const;
    void		dumpInfo(IOPar&) const;


protected:

    PosVecDataSet&		data_;
    TypeSet<BinIDValueSet::Pos>	bvsidxs_;
    bool			is2d_;
    bool			minimal_;

    void		initPVDS();
    void		init(const TypeSet<DataRow>&,
	    		     const ObjectSet<DataColDef>&);
    void		calcIdxs();

    static const int	groupcol_;
    const int		nrfixedcols_;

    DataColDef&		gtColDef(ColID) const;
};

#endif
