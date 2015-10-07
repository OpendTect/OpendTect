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


/*!\brief Set of possibe headers for a certain key. */

mExpClass(uiSEGY) HdrEntryDataSet : public TypeSet<HdrEntryRecord>
{
public:

    void		addRecord();

    void		add(int heidx,int val); // last added record
    void		reject(int heidx); // removes from all records

    void		rejectConstants();
    void		rejectNoProgress();

    void		merge(const HdrEntryDataSet&);

    TypeSet<int>	idxs_; // in TrcHeader::hdrDef()
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
    HdrEntryDataSet	x_;
    HdrEntryDataSet	y_;

    void		setEmpty();
    void		add(const TrcHeader&,bool isswapped);
    void		finish();

    void		merge(const HdrEntryKeyData&);
			// will not finish() - do this after last merge

    void		setBest(TrcHeaderDef&) const;

protected:

    void		init();
    void		setCurOrFirst(HdrEntry&,const HdrEntryDataSet&) const;

};

} // namespace SEGY


#endif
