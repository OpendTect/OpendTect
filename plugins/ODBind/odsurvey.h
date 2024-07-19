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
    along with this program. If not, see <https://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/

#include "odbindmod.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "gendefs.h"
#include "ptrman.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

class IOObj;
class SurveyInfo;
namespace OD{
    namespace JSON{
	class Object;
    };
};


class odSurvey {
public:
		    odSurvey(const char* surveynm, const char* basedir=nullptr);
		    ~odSurvey();

    bool		isOK() const	{ return errmsg_.isEmpty(); }
    BufferString	errMsg() const	{ return errmsg_; }
    void		setErrMsg(const char* msg)	{ errmsg_ = msg; }

    BufferString	type() const;
    bool		has2D() const;
    bool		has3D() const;
    BufferString	get_crsCode() const;
    BufferString	surveyPath() const;
    bool		activate() const;
    const SurveyInfo&	si() const { activate(); return SI(); }

    void		getInfo(OD::JSON::Object&) const;
    void		getFeature(OD::JSON::Object&, bool towgs=true) const;
    void		getPoints(OD::JSON::Array&, bool towgs=true) const;
    void		makeCoordsList(OD::JSON::Array&, const TypeSet<Coord>&,
				       bool towgs) const;

    BufferStringSet*	getObjNames(const char* trlgrpnm) const;
    void		getObjInfoByID(const MultiID&, OD::JSON::Object&) const;
    void		getObjInfo(const char* objname,const char* trlgrpnm,
				   OD::JSON::Object&) const;
    void		getObjInfos(const char* trlgrpnm, bool all,
				    OD::JSON::Array&) const;
    bool		isObjPresent(const char* objname,
				     const char* trgrpnm=nullptr) const;
    IOObj*		createObj(const char* objname, const char* trgrpnm,
				  const char* translkey, bool overwrite,
				  BufferString& errmsg) const;
    void		createObj(const char* objname, const char* trgrpnm,
				  const char* translkey, bool overwrite) const;
    void		removeObj(const char* objname,
				  const char* trgrpnm=nullptr) const;
    void		removeObjByID(const MultiID&) const;

    static bool		initModule(const char*);
    static BufferStringSet*	getNames(const char* data_root=nullptr);
    static void		getInfos(OD::JSON::Array&, const BufferStringSet&,
				 const char* data_root=nullptr);
    static void		getFeatures(OD::JSON::Object&, const BufferStringSet&,
				    const char* data_root=nullptr);
    static BufferStringSet	getCommonItems(const BufferStringSet&,
					       const BufferStringSet&);
    static TrcKeyZSampling	tkzFromRanges(const int32_t inlrg[3],
					      const int32_t crlrg[3],
					      const float zrg[3],
					      bool zistime);
    static TrcKeySampling	tkFromRanges(const int32_t inlrg[3],
					     const int32_t crlrg[3]);

protected:
    BufferString		basedir_, survey_;
    mutable BufferString	errmsg_;
    static BufferString	curbasedir_, cursurvey_;

};

typedef void* hStringSet;
typedef void* hSurvey;

mExternC(ODBind) bool		initModule(const char*);
mExternC(ODBind) void		exitModule();
mExternC(ODBind) hSurvey	survey_new(const char* surveynm,
					   const char* basedir);
mExternC(ODBind) void		survey_del(hSurvey);
mExternC(ODBind) void		survey_bin(hSurvey, double x, double y,
					   int* iline, int* xline);
mExternC(ODBind) void		survey_bincoords(hSurvey, double x, double y,
						 double* iline, double* xline);
mExternC(ODBind) void		survey_coords(hSurvey, int iline, int xline,
					      double* x, double* y);
mExternC(ODBind) const char*	survey_feature(hSurvey);
mExternC(ODBind) const char*	survey_features(const hStringSet,
						const char* basedir);
mExternC(ODBind) bool		survey_has2d(hSurvey);
mExternC(ODBind) bool		survey_has3d(hSurvey);

mExternC(ODBind) bool		survey_hasobject(hSurvey, const char* objname,
						 const char* trgrpnm);
mExternC(ODBind) const char*	survey_getobjinfo(hSurvey, const char* objname,
						  const char* trgrpnm);
mExternC(ODBind) const char*	survey_getobjinfobyid(hSurvey,
						      const char* idstr);
mExternC(ODBind) const char*	survey_getobjinfos(hSurvey,
					       const char* trgrpnm, bool all);
mExternC(ODBind) void		survey_removeobj(hSurvey, const char* objname,
						 const char* trgrpnm);
mExternC(ODBind) void		survey_removeobjbyid(hSurvey, const char* id);
mExternC(ODBind) void		survey_createobj(hSurvey, const char* name,
						 const char* trgrpnm,
						 const char* translkey,
						 bool overwrite);
mExternC(ODBind) hStringSet	survey_getobjnames(hSurvey,
						   const char* trgrpnm);

mExternC(ODBind) const char*	survey_info(hSurvey);
mExternC(ODBind) const char*	survey_errmsg(hSurvey);
mExternC(ODBind) bool		survey_isok(hSurvey);

mExternC(ODBind) const char*	survey_infos(const hStringSet,
					     const char* basedir);
mExternC(ODBind) hStringSet	survey_names(const char* basedir);
mExternC(ODBind) const char*	survey_path(hSurvey);
mExternC(ODBind) const char*	survey_survtype(hSurvey);
mExternC(ODBind) void		survey_zrange(hSurvey, float* zrg);
mExternC(ODBind) void		survey_inlrange(hSurvey, int32_t* rg);
mExternC(ODBind) void		survey_crlrange(hSurvey, int32_t* rg);

mExternC(ODBind) const char*	isValidSurveyDir(const char*);
			    //!< Full path to an OpendTect project directory
mExternC(ODBind) const char*	isValidDataRoot(const char*);
			    //!< Full path to a writable directory
mExternC(ODBind) const char*	survey_createtemp(const char* surveynm,
							const char* basedir);
