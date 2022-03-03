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
					Array2D<int>& presence,
					BufferStringSet& lognms,
					Well::Info::DepthType depthtype) const;
    void			getLogsForMnems(const MnemonicSelection& mns,
					BufferStringSet& lognms) const;

private:
    const ObjectSet<Well::Data>&	allwds_;
};

}; // namespace Well
