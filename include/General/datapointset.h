#ifndef datapointset_h
#define datapointset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jan 2008
 RCS:		$Id: datapointset.h,v 1.1 2008-01-30 16:38:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "binidvalset.h"
#include "datapackbase.h"
class DataColDef;
class BinIDValueSet;
class PosVecDataSet;
class UnitOfMeasure;
class BufferStringSet;


/*!\brief Set of data points with group selection.

  The idea is to generate a set of data points, which are then put in different
  groups. The data is transferred into a BinIDValueSet for performance,
  but a fixed int addressing becomes available.

  The design is based on the model that you extract data in some way which
  should then be quickly accessible. The data set is not meant to be
  manipulated other than via setSelected() and setGroup().

*/

class DataPointSet : public PointDataPack
{
public:

    typedef int		RowID;
    typedef int		ColID;

    /*!\brief Fast position based on BinID but more precise */

    struct Pos
    {
			Pos() : offsx_(0), offsy_(0), z_(0)	{}
			Pos( const BinID& bid, float _z,
			     float offsx=0, float offsy=0 )
			    : binid_(bid), z_(_z)
			    , offsx_(offsx), offsy_(offsy)	{}
			Pos(const Coord&,float z);
			Pos(const Coord3&);

	const BinID&	binID() const	{ return binid_; }
	Coord		coord() const;
	float		z() const	{ return z_; }

	BinID		binid_;
	float		offsx_, offsy_;
	float		z_;

    protected:

	void		setOffs(const Coord&);
    };

    /*!\brief Data point with group. Group 0 means 'inactive',
      	      it can never be selected. */

    struct DataRow
    {
			DataRow()
			    : grp_(1)			{ setSel(false); }
			DataRow( const Pos& p, unsigned short grp=1,
				   bool issel=false )
			    : pos_(p), grp_((short)grp)
							{ setSel( issel ); }

	const BinID&		binID() const		{ return pos_.binID(); }
	Coord			coord() const		{ return pos_.coord(); }
	const TypeSet<float>&	data() const		{ return data_; }
	unsigned short		group() const
				{ return grp_ < 0 ? -grp_ : grp_; }
	bool			isSel() const		{ return grp_ > 0; }
	bool			isInactive() const	{ return grp_ == 0; }
	void			setSel( bool yn )
	    			{ if ( grp_ >= 0 != yn ) grp_ = -grp_; }

	Pos		pos_;
	TypeSet<float>	data_;
	short		grp_;
    };

    			DataPointSet(const TypeSet<DataRow>&,
				     const ObjectSet<DataColDef>&);
    			DataPointSet(const TypeSet<DataRow>&,
				     const BufferStringSet& valnms);
    			DataPointSet(const PosVecDataSet&,bool haveoffs=false,
				     bool havegroup=false);
			//!< Must have Z column, opt xoffs&yoffs and group cols
    			DataPointSet(const DataPointSet&);
    virtual		~DataPointSet();
    DataPointSet&	operator =(const DataPointSet&);

    int			nrCols() const;
    const char*		colName(ColID) const;
    const UnitOfMeasure* unit(ColID) const;
    const DataColDef&	colDef(ColID) const;
    ColID		indexOf(const char*) const;

    const PosVecDataSet& dataSet() const		{ return data_; }
    const BinIDValueSet& bivSet() const;
    int			bivSetIdx( ColID idx ) const
						{ return idx-nrfixedcols_; }

    int			size() const;		//!< PointDataPack impl
    Pos			pos(RowID) const;
    DataRow		dataRow(RowID) const;
    BinID		binID(RowID) const;	//!< PointDataPack impl
    Coord		coord(RowID) const;	//!< PointDataPack impl
    float		z(RowID) const;		//!< PointDataPack impl
    float		value(ColID,RowID) const;
    unsigned short	group(RowID) const;
    bool		selected(RowID) const;
    bool		inactive( RowID rid ) const { return group(rid) == 0; }

    void		setGroup(RowID,unsigned short);
    void		setSelected(RowID,bool);
    void		setInactive(RowID,bool);

    BinIDValueSet&	bivSet() { return const_cast<DataPointSet*>
					  (this)->bivSet(); }
    			//!< The idea is to manage vectors with the selection
    			//!< mechanism. But if you really must remove
    			//!< vectors, this is your access point
    void		dataChanged()			{ calcIdxs(); }
    			//!< When data modified, you want to call this.
    			//!< all RowIDs may change
    DataColDef&		colDef( ColID cid )
			{ return const_cast<DataPointSet*>(this)->colDef(cid); }
    			//!< In case you want to change the definition of a col
    void		addRow(const DataRow&);
    			//!< When finished, you have to call dataChanged()

    // DataPack interface impl
    bool		simpleCoords() const		{ return false; }
    void		getAuxInfo(RowID,IOPar&) const;
    float		nrKBytes() const;
    void		dumpInfo(IOPar&) const;


protected:

    PosVecDataSet&		data_;
    TypeSet<BinIDValueSet::Pos>	bvsidxs_;

    void		initPVDS();
    void		init(const TypeSet<DataRow>&,
	    		     const ObjectSet<DataColDef>&);
    void		calcIdxs();

    static const int	nrfixedcols_;
    static const int	groupcol_;
};


#endif
