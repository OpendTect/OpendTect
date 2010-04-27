#ifndef emioobjinfo_h
#define emioobjinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
 RCS:		$Id: emioobjinfo.h,v 1.1 2010-04-27 05:32:14 cvssatyaki Exp $
________________________________________________________________________

-*/
 
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

mClass IOObjInfo
{
public:

			IOObjInfo(const IOObj*);
			IOObjInfo(const IOObj&);
			IOObjInfo(const MultiID&);
			IOObjInfo(const char* ioobjnm);
			IOObjInfo(const IOObjInfo&);
			~IOObjInfo();

    enum ObjectType	{ Horizon3D, Horizon2D, FaultStickSet, Fault, Body };
    IOObjInfo&		operator =(const IOObjInfo&);

    bool		isOK() const;

    ObjectType		type() const		{ return type_; }
    const IOObj*	ioObj() const		{ return ioobj_; }
    const char*		name() const;

    bool		getSectionIDs(TypeSet<SectionID>&) const;
    bool		getSectionNames(BufferStringSet&) const;
    bool		getAttribNames(BufferStringSet&) const;
    Interval<float>	getZRange() const;
    StepInterval<int>	getInlRange() const;
    StepInterval<int>	getCrlRange() const;

    // Body

    bool		getBodyRange(CubeSampling&) const;

    // FaultStickSet

    int 		nrSticks() const;

    // 2D Horizons
    bool		getLineSets(BufferStringSet&) const;
    bool		getLineNames(BufferStringSet&) const;
    bool		getTrcRanges(TypeSet< StepInterval<int> >&) const;

protected:

    ObjectType		type_;
    IOObj*		ioobj_;
    mutable dgbSurfaceReader* reader_;

    void		setType();

};

};


#endif
