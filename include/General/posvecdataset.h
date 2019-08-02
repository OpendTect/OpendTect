#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2005
 Contents:	Set with data vectors on positions
________________________________________________________________________

-*/

#include "generalmod.h"
#include "binnedvalueset.h"
#include "bufstringset.h"
#include "uistrings.h"
class DataColDef;



/*!\brief Data set consisting of data vectors

  Every data vector has an inline, crossline and a Z (which may be undefined).
  The "Z" column is automatically added.

*/

mExpClass(General) PosVecDataSet
{ mODTextTranslationClass(PosVecDataSet)
public:
			PosVecDataSet(const char* nm=0);
			PosVecDataSet(const PosVecDataSet&);
    virtual		~PosVecDataSet();
    PosVecDataSet&	operator =(const PosVecDataSet&);
    void		copyStructureFrom(const PosVecDataSet&);

    bool		isEmpty() const		{ return data_.isEmpty(); }
    void		setEmpty();
    int			add(DataColDef*);
    bool		insert(int idx,DataColDef*);
			//!<\returns index
    void		removeColumn(int); //!< "Z" col (idx=0) can't be removed
    enum OvwPolicy	{ Keep, OvwIfUdf, Ovw };
			//!< During merge, which set is dominant?
    enum ColMatchPol	{ NameExact, RefExact, NameStart, RefStart };
    void		merge(const PosVecDataSet&,OvwPolicy pol=OvwIfUdf,
			      ColMatchPol cmp=NameExact);
			//!< This is a rather intelligent method.

    BinnedValueSet&	data()			{ return data_; }
    const BinnedValueSet& data() const		{ return data_; }
    OD::GeomSystem	geomSystem() const;
    bool		is2D() const		{ return ::is2D(geomSystem()); }

    int			nrCols() const		{ return coldefs_.size(); }
    DataColDef&		colDef( int idx )	{ return *coldefs_[idx]; }
    const DataColDef&	colDef( int idx ) const	{ return *coldefs_[idx]; }
    int			findColDef(const DataColDef&,
				   ColMatchPol p=RefExact) const;
			//!< returns -1 if no match

    const char*		name() const		{ return name_.buf(); }
    void		setName( const char* nm ) { name_ = nm; }

    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }

    bool		getFrom(const char*,BufferString& errmsg);
    bool		getFrom(const char*,uiString& errmsg);
    bool		putTo(const char*,BufferString& errmsg,
				bool tabstyle) const;
    bool		putTo(const char*,uiString& errmsg,bool tabstyle) const;
			//!< tabstyle -> for spreadsheet import (looses info)
			//!< !tabstyle: dTect style (preserves all)
    static bool		getColNames(const char*,BufferStringSet& bss,
				    BufferString& errmsg,bool refs=false);
    static bool		getIOPar(const char*,IOPar& iop,BufferString& errmsg);

protected:

    BinnedValueSet	data_;
    ObjectSet<DataColDef> coldefs_;
    BufferString	name_;
    IOPar&		pars_;

    void		mergeColDefs(const PosVecDataSet&,ColMatchPol,int*);
    friend class	DataPointSet;

};

#define mPosVecDataSetFileType "Positioned Vector Data"
