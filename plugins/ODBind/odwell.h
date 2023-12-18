#pragma once
/*+
________________________________________________________________________________

 Copyright:  (C) 2022 dGB Beheer B.V.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/
#include "odbindmod.h"

#include "refcount.h"
#include "welldata.h"
#include "wellman.h"

#include "odbind.h"
#include "odsurvey_object.h"

namespace Well{
    class Track;
}

class ODSurvey;

class odWell : public odSurveyObject
{
public:
    enum SampleMode { Upscale, Sample };
    enum ZMode { MD, TVD, TVDSS, TWT };
    odWell( const odSurvey& thesurvey, const char* name );

    const Well::Data*	wd() const;

    BufferStringSet*	getLogNames() const;
    void		getLogInfo(OD::JSON::Array&,
				   const BufferStringSet&) const;
    BufferStringSet*	getMarkerNames() const;
    void		getMarkerInfo(OD::JSON::Array&,
				      const BufferStringSet&) const;

    void		getTrack(hAllocator);

    void		getLogs(hAllocator, const BufferStringSet&,
				OD::JSON::Object&,
				float zstep=0.5, SampleMode samplemode=Upscale);

    void		putLog(const char* lognm, const float* dah,
			       const float* logdata, uint32_t size,
			       const char* uom=nullptr,
			       const char* mnem=nullptr, bool overwrite=false);

    void		getInfo(OD::JSON::Object&) const override;
    void		getFeature(OD::JSON::Object&,
				   bool towgs=true) const override;
    void		getPoints(OD::JSON::Array&, bool) const override;

    static const char*	translatorGrp()		{ return "Well"; }
    static BufferStringSet*	getCommonMarkerNames(const odSurvey& suvey,
						     const BufferStringSet&);
    static BufferStringSet*	getCommonLogNames(const odSurvey& suvey,
						  const BufferStringSet&);

protected:
    RefMan<Well::Data>	wd_;

};

mDeclareBaseBindings(Well, well)
mDeclareRemoveBindings(Well, well)

mExternC(ODBind) hStringSet	well_lognames(hWell);
mExternC(ODBind) const char*	well_loginfo(hWell, const hStringSet);
mExternC(ODBind) hStringSet	well_markernames(hWell);
mExternC(ODBind) const char*	well_markerinfo(hWell, const hStringSet);
mExternC(ODBind) void		well_gettrack(hWell, hAllocator);
mExternC(ODBind) const char*	well_getlogs(hWell, hAllocator,
					     const hStringSet, float zstep,
					     bool upscale);
mExternC(ODBind) void		well_putlog(hWell, const char* lognm,
					    const float* dah,
					    const float* logdata,
					    uint32_t sz, const char* uom,
					    const char* mnem, bool overwrite);
mExternC(ODBind) bool		well_deletelogs(hWell, const hStringSet);
mExternC(ODBind) void		well_tvd(hWell, const float dah, float* tvd);
mExternC(ODBind) void		well_tvdss(hWell, const float dah,
					   float* tvdss);
mExternC(ODBind) hStringSet	well_commonlognames(hSurvey, const hStringSet);
mExternC(ODBind) hStringSet	well_commonmarkernames(hSurvey,
						       const hStringSet);


