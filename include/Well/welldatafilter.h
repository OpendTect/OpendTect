#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Feb 2022
________________________________________________________________________

-*/

#include "wellmod.h"

#include "welldata.h"

class BufferStringSet;
class MnemonicSelection;

template <class T> class Array2D;


namespace Well {

mExpClass(Well) WellDataFilter
{
public:
				WellDataFilter(const ObjectSet<Well::Data>&);
				~WellDataFilter();

    void			getWellsFromLogs(
					const BufferStringSet& lognms,
					BufferStringSet& wellnms) const;
    void			getWellsFromMnems(
					const MnemonicSelection& mns,
					BufferStringSet& wellnms) const;
    void			getWellsWithNoLogs(
					BufferStringSet& wellnms) const;
    void			getWellsFromMarkers(
					const BufferStringSet& markernms,
					BufferStringSet& wellnms) const;
    void			getMarkersLogsMnemsFromWells(
					const BufferStringSet& wellnms,
					BufferStringSet& lognms,
					MnemonicSelection& mns,
					BufferStringSet& markernms) const;
    void			getLogPresence(
					const BufferStringSet& wellnms,
					const char* topnm,const char* botnm,
					const BufferStringSet& alllognms,
					Array2D<int>& presence ) const;
    void			getLogPresenceForMnems(
					const BufferStringSet& wellnms,
					const char* topnm,const char* botnm,
					const MnemonicSelection& mns,
					Array2D<int>& presence ) const;
    void			getLogPresenceFromValFilter(
					const BufferStringSet& wellnms,
					const BufferStringSet& lognms,
					const BufferStringSet& alllognms,
					const MnemonicSelection& mns,
					const TypeSet<Interval<float>> valrg,
					Array2D<int>& presence) const;
    void			getLogsInMarkerZone(
					BufferStringSet& wellnms,
					const char* topnm,const char* botnm,
					BufferStringSet& lognms) const;
    void			getMnemsInMarkerZone(
					BufferStringSet& wellnms,
					const char* topnm,const char* botnm,
					MnemonicSelection& mns) const;
    void			getLogsInDepthInterval(
					const Interval<float> depthrg,
					BufferStringSet& wellnms,
					BufferStringSet& lognms) const;
    void			getMnemsInDepthInterval(
					const Interval<float> depthrg,
					BufferStringSet& wellnms,
					MnemonicSelection& mns) const;
    void			getLogsInValRange(
					const MnemonicSelection& mns,
					const TypeSet<Interval<float>> valrg,
					BufferStringSet& wellnms,
					BufferStringSet& lognms) const;
    void			getLogsForMnems(
					const MnemonicSelection& mns,
					BufferStringSet& lognms) const;
    void			getWellsOfType(
					const OD::WellType,
					BufferStringSet& wellnms) const;

private:

    Interval<float>		getDepthRangeFromMarkers(
					const Well::Data* wd,
					const char* topnm,const char* botnm,
					bool vertical) const;

    const ObjectSet<Well::Data>&	allwds_;
};

}; // namespace Well
