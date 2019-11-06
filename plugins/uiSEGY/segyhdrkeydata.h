#ifndef segyhdrkeydata_h
#define segyhdrkeydata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "typeset.h"
#include "sortedlist.h"


namespace SEGY
{

typedef TypeSet<int>	HdrEntryRecord;


/*!\brief Set of constraints on various needed header values. */

mExpClass(uiSEGY) HdrEntryConstraints
{
public:

			HdrEntryConstraints(); //!< gets from settigns
    void		save2Settings() const;

    Interval<int>	inlrg_;
    Interval<int>	crlrg_;
    Interval<int>	trcnrrg_;
    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<float>	offsrg_;
    Interval<float>	azimuthrg_;

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    static const HdrEntryConstraints&	get();
    static HdrEntryConstraints&		get4Edit();

};



/*!\brief Set of possibe headers for a certain key. */

mExpClass(uiSEGY) HdrEntryDataSet : public TypeSet<HdrEntryRecord>
{
public:

    void		addRecord();

    void		add(int heidx,int val);
    void		reject(int heidx);	// removes from all records

    void		rejectConstants(int,int);
    void		rejectNoProgress(int,int);

    void		merge(const HdrEntryDataSet&);

    TypeSet<int>	idxs_;		// in TrcHeader::hdrDef()
    SortedList<int>	rejectedidxs_;

    virtual void	erase();

};


/*!\brief header key info collected by scanning SEG-Y file */

mExpClass(uiSEGY) HdrEntryKeyData
{
public:

			HdrEntryKeyData();

    HdrEntryDataSet	inl_;
    HdrEntryDataSet	crl_;
    HdrEntryDataSet	trcnr_;
    HdrEntryDataSet	refnr_;
    HdrEntryDataSet	offs_;
    HdrEntryDataSet	azimuth_;
    HdrEntryDataSet	x_;
    HdrEntryDataSet	y_;
    TypeSet<int>	newfileat_;

    int			size() const		{ return inl_.size(); }
    void		setEmpty();
    void		add(const TrcHeader&,bool isswapped,
			    bool isnewline);
    void		finish(bool isps);

    void		merge(const HdrEntryKeyData&);
			// will not finish() - do this after last merge

    void		setBest(TrcHeaderDef&) const;

protected:

    void		init();
    void		setCurOrPref(HdrEntry&,const HdrEntryDataSet&,
				     int prefhdidx,int defidx) const;
    void		setCurOrPref(HdrEntry&,const HdrEntryDataSet&,
				     const TypeSet<int>&,int) const;

};

} // namespace SEGY


#endif
