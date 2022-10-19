#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "crsmod.h"
#include "latlong.h"
#include "manobjectset.h"
#include "uistring.h"

class BufferStringSet;
class UnitOfMeasure;

namespace Coords
{


mExpClass(CRS) AuthorityCode
{
public:
			AuthorityCode(const char* auth,const char* code)
			    : authority_(auth),code_(code) {}
			AuthorityCode(const char* auth,int code)
			    : authority_(auth),code_(::toString(code)) {}
			AuthorityCode(const AuthorityCode& oth)
			    : authority_(oth.authority_),code_(oth.code_) {}

    const char*		authority() const { return authority_.buf(); }
    const char*		code() const	  { return code_.buf(); }

    bool		operator ==(const AuthorityCode&) const;

    static AuthorityCode	fromString(const char*);
    static AuthorityCode	sWGS84AuthCode();

    BufferString	toString() const;
    BufferString	toURNString();

protected:

    BufferString	authority_;
    BufferString	code_;
};


mExpClass(CRS) Projection
{ mODTextTranslationClass(Projection);
public:

				Projection(AuthorityCode);
    virtual			~Projection();

    AuthorityCode		authCode() const	{ return authcode_; }
    virtual const char*		userName() const	= 0;

    virtual bool		isOK() const;

    virtual bool		isOrthogonal() const;
    virtual bool		isLatLong() const	{ return false; }
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    virtual AuthorityCode	getGeodeticAuthCode() const;

    virtual BufferString	getProjDispString() const;
    virtual BufferString	getGeodeticProjDispString() const;
    static BufferString		sWGS84ProjDispString();

    static Projection*		getByAuthCode(AuthorityCode);
    static Coord		convert(const Coord&,const Projection& from,
					const Projection& to);
    double			getConvFactorToM() const { return convfac_; }
    const UnitOfMeasure*	getUOM() const { return uom_; }
protected:

    virtual LatLong		toGeographic(const Coord&,
					     bool wgs84=false) const;
    virtual Coord		fromGeographic(const LatLong&,
					       bool wgs84=false) const;
    const UnitOfMeasure*	uom_ = nullptr;
    double			convfac_ = mUdf(double);

private:

    virtual Coord		transformTo(const Projection& target,
					    LatLong) const;
    virtual LatLong		transformTo(const Projection& target,
					    Coord) const;

    AuthorityCode		authcode_;

    friend class ProjectionBasedSystem;
};


mExpClass(CRS) CRSInfoList
{
public:
    virtual			~CRSInfoList()	{}

    virtual int			size() const				= 0;
    virtual const char*		authCode(int) const			= 0;
    virtual const char*		authName(int) const			= 0;
    virtual const char*		name(int) const				= 0;
    virtual const char*		areaName(int) const			= 0;
    virtual const char*		projMethod(int) const			= 0;
    virtual int			indexOf(const AuthorityCode&) const	= 0;

    uiString			getDispString(int) const;
    uiString			getDescString(int) const;

protected:
				CRSInfoList()	{}

};


mGlobal(CRS) void		initCRSDatabase();
mGlobal(CRS) CRSInfoList*	getCRSInfoList(bool orthogonal = true);

} // namespace Coords


extern "C" {
    mGlobal(CRS) bool		initCRSPlugin(bool withdatabase);
}