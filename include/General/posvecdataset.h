#ifndef posvecdataset_h
#define posvecdataset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 Contents:	Set with data vectors on positions
 RCS:		$Id: posvecdataset.h,v 1.1 2005-01-17 16:35:02 bert Exp $
________________________________________________________________________

-*/

#include "binidvalset.h"
class UnitOfMeasure;



/*!\brief Data set consisting of data vectors

  Every data vector has an inline, crossline and a Z (which may be undefined).
  The "Z" column is automatically added.

*/

class PosVecDataSet
{
public:
    			PosVecDataSet();
    			PosVecDataSet( const PosVecDataSet& vds )
			:data_(1,true)		{ *this = vds; }
    virtual		~PosVecDataSet()	{ deepErase(coldefs_); }
    PosVecDataSet&	operator =( const PosVecDataSet& vds )
			{
			    if ( &vds != this ) {
			    empty();
			    deepCopy(coldefs_,vds.coldefs_);
			    merge( vds );
			    } return *this;
			}

    /*!\brief Column definition in PosVecDataSet

      The ref_ is intended for whatever references that are important in the
      software but which should probably not be displayed to the user.

    */

    class ColumnDef
    {
    public:
				ColumnDef( const char* nm,
					   const UnitOfMeasure* un=0 )
				: name_(nm), unit_(un)		{}

	BufferString		name_;
	BufferString		ref_;
	const UnitOfMeasure*	unit_;

	enum MatchLevel		{ Exact, Start, None };
	MatchLevel		compare(const ColumnDef&,bool use_name) const;
				//!< if !use_name, matches ref_ .

    };

    void		empty();
    void		add(ColumnDef*);
    void		removeColumn(int); //!< "Z" col (idx=0) can't be removed
    enum OvwPolicy	{ Keep, OvwIfUdf, Ovw };
    			//!< During merge, which set is dominant?
    enum ColMatchPol	{ NameExact, RefExact, NameStart, RefStart };
    void		merge(const PosVecDataSet&,OvwPolicy pol=OvwIfUdf,
			      ColMatchPol cmp=NameExact);
    			//!< This is a rather intelligent method.

    BinIDValueSet&	data()			{ return data_; }
    const BinIDValueSet& data() const		{ return data_; }

    int			nrCols() const		{ return coldefs_.size(); }
    ColumnDef&		colDef( int idx )	{ return *coldefs_[idx]; }
    const ColumnDef&	colDef( int idx ) const	{ return *coldefs_[idx]; }

protected:

    BinIDValueSet	data_;
    ObjectSet<ColumnDef> coldefs_;

    void		mergeColDefs(const PosVecDataSet&,ColMatchPol,int*);

};


#endif
