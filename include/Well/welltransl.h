#ifndef welltransl_h
#define welltransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: welltransl.h,v 1.8 2008-05-22 14:09:42 cvsbert Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "position.h"
class Executor;
class DataPointSet;
class BufferStringSet;


namespace Well { class Data; };


class WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
    			mDefEmptyTranslatorGroupConstructor(Well)
    const char*		defExtension() const { return "well"; }
};


class WellTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(Well)

    virtual bool	read(Well::Data&,const IOObj&)		= 0;
    virtual bool	write(const Well::Data&,const IOObj&)	= 0;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;

    			//!< Implemented using TrackSampler
    static Executor*	createDataPointSets(const BufferStringSet&,
					    const IOPar&,bool for2d,
					    ObjectSet<DataPointSet>&,
					    bool zvalsintime);
};


class dgbWellTranslator : public WellTranslator
{			  isTranslator(dgb,Well)
public:
    			mDefEmptyTranslatorConstructor(dgb,Well)

    virtual bool	read(Well::Data&,const IOObj&);
    virtual bool	write(const Well::Data&,const IOObj&);

};


#endif
