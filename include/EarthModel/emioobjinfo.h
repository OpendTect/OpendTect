#ifndef emioobjinfo_h
#define emioobjinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "earthmodelmod.h"
#include "emobject.h"
#include "ranges.h"
#include "typeset.h"

class IOObj;
class MultiID;
class CubeSampling;
class BufferStringSet;

/*!\brief Info on IOObj for earthmodel */

namespace EM
{

class dgbSurfaceReader;
class SurfaceIOData;

mClass(EarthModel) IOObjInfo
{
public:

			IOObjInfo(const IOObj*);
			IOObjInfo(const IOObj&);
			IOObjInfo(const MultiID&);
			IOObjInfo(const char* ioobjnm);
			IOObjInfo(const IOObjInfo&);
			~IOObjInfo();
    IOObjInfo&		operator =(const IOObjInfo&);

    enum ObjectType	{ Horizon3D, Horizon2D, FaultStickSet, Fault, Body };
    static void		getIDs(ObjectType,TypeSet<MultiID>&);
				//!< Does not erase the IDs at start

    bool		isOK() const;
    inline const IOObj*	ioObj() const		{ return ioobj_; }
    const char*		name() const;
    inline ObjectType	type() const		{ return type_; }

    bool		getSectionIDs(TypeSet<SectionID>&) const;
    bool		getSectionNames(BufferStringSet&) const;
    bool		getAttribNames(BufferStringSet&) const;
    Interval<float>	getZRange() const;
    StepInterval<int>	getInlRange() const;
    StepInterval<int>	getCrlRange() const;
    IOPar*		getPars() const;

    // Surface
    inline bool		isSurface() const	{ return type_ != Body; }
    const char*		getSurfaceData(SurfaceIOData&) const;
    			//!<\returns err msg or null if OK

    // Horizon
    inline bool		isHorizon() const	{ return type_ < FaultStickSet;}
    inline bool		is2DHorizon() const	{ return type_ == Horizon2D; }
    int			levelID() const;
    static void		getTiedToLevelID(int lvlid,TypeSet<MultiID>&,bool is2d);
    static bool		sortHorizonsOnZValues(const TypeSet<MultiID>&,
					      TypeSet<MultiID>&);

    // 2D Horizons
    bool		getLineSets(BufferStringSet&) const;
    bool		getLineNames(BufferStringSet&) const;
    bool		getTrcRanges(TypeSet< StepInterval<int> >&) const;

    // Body

    bool		getBodyRange(CubeSampling&) const;

    // FaultStickSet

    int 		nrSticks() const;


    // Helpful stuff
    static ObjectType	objectTypeOfIOObjGroup(const char*);

protected:

    ObjectType		type_;
    IOObj*		ioobj_;
    mutable dgbSurfaceReader* reader_;

    void		setType();

};

};


#endif

