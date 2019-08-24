#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "factory.h"
#include "refcount.h"
#include "sets.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class Executor;

namespace Geometry { class Element; }
namespace EM { class Object; }
namespace Attrib { class SelSpec; class SelSpecList; }

namespace MPE
{

class EMSeedPicker;
class SectionTracker;

/*!
\brief Tracks EM objects.
*/

mExpClass(MPEEngine) EMTracker : public RefCount::Referenced
{ mODTextTranslationClass(EMTracker);
public:

    mDefineFactory1ParamInClass( EMTracker, EM::Object*, factory );

				EMTracker(EM::Object*);

    BufferString		objectName() const;
    EM::Object*		emObject()		{ return emobject_; }
    DBKey			objectID() const;

    virtual bool		is2D() const		{ return false; }

    void			setTypeStr(const char* type)
				{ type_ = type; }
    const char*			getTypeStr() const	{ return type_; }

    virtual bool		isEnabled() const	{ return isenabled_; }
    virtual void		enable(bool yn)		{ isenabled_=yn; }

    virtual bool		snapPositions(const TypeSet<TrcKey>&);

    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;
    void			getNeededAttribs(Attrib::SelSpecList&) const;

    virtual SectionTracker*	createSectionTracker() = 0;
    SectionTracker*		cloneSectionTracker();
    SectionTracker*		getSectionTracker(bool create=false);
    virtual EMSeedPicker*	getSeedPicker(bool create=true)
				{ return 0; }

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
				~EMTracker();

    bool			isenabled_;
    SectionTracker*		sectiontracker_;
    BufferString		errmsg_;
    const char*			type_;

    void			setEMObject(EM::Object*);

    static const char*		setupidStr()	{ return "SetupID"; }
    static const char*		sectionidStr()	{ return "SectionID"; }

private:

    EM::Object*			emobject_;

};


} // namespace MPE
