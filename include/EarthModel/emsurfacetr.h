#ifndef emsurfacetr_h
#define emsurfacetr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfacetr.h,v 1.20 2012-08-03 13:00:20 cvskris Exp $
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM
{
    class dgbSurfaceReader;
    class FaultStickSet;
    class Fault3D;
    class Horizon;
    class Horizon2D;
    class Horizon3D;
    class Surface;
}

typedef EM::Horizon3D 		EMHorizon3D;
typedef EM::Horizon2D		EMHorizon2D;
typedef EM::Horizon		EMAnyHorizon;
typedef EM::Fault3D		EMFault3D;
typedef EM::FaultStickSet	EMFaultStickSet;


/*!\brief Read/write EM::Horizon to storage */

mClass(EarthModel) EMHorizon3DTranslatorGroup : public TranslatorGroup
{				   isTranslatorGroup(EMHorizon3D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon3D)

    const char*			defExtension() const	{ return "hor"; }

    static FixedString		keyword();
};


mClass(EarthModel) EMHorizon2DTranslatorGroup : public TranslatorGroup
{				   isTranslatorGroup(EMHorizon2D)
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon2D)

    const char*			defExtension() const	{ return "2dh"; }

    static FixedString		keyword();
};


mClass(EarthModel) EMAnyHorizonTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(EMAnyHorizon)
public:
			    mDefEmptyTranslatorGroupConstructor(EMAnyHorizon)

    static FixedString	    keyword();
};


mClass(EarthModel) EMFault3DTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(EMFault3D)
public:
    			mDefEmptyTranslatorGroupConstructor(EMFault3D)

    const char*		defExtension() const { return "flt"; }

    static FixedString	keyword();
};


mClass(EarthModel) EMFaultStickSetTranslatorGroup : public TranslatorGroup
{				       isTranslatorGroup(EMFaultStickSet)
public:
    			mDefEmptyTranslatorGroupConstructor(EMFaultStickSet)

    const char*		defExtension() const { return "fss"; }

    static FixedString	keyword();
};


mClass(EarthModel) EMSurfaceTranslator : public Translator
{
public:
    				EMSurfaceTranslator(const char* nm,
						    const char* unm)
				    : Translator(nm,unm)
				    , ioobj_(0)
				    , surface(0)
				    , sels_(sd_)	{}

    virtual			~EMSurfaceTranslator();

    bool			startRead(const IOObj&);
    bool			startWrite(const EM::Surface&);

    EM::SurfaceIODataSelection& selections()		{ return sels_; }

    virtual Executor*		reader(EM::Surface&)	{ return 0; }
				/*!< Executor is managed by client. */
    Executor*			writer(const IOObj&,bool fullimplremove=true);
				/*!< Executor is managed by client. */ 

    const char*			errMsg() const		{ return errmsg_.str(); }

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    				   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    static bool			getBinarySetting();

protected:

    IOObj*			ioobj_;
    EM::Surface*		surface;
    BufferString		errmsg_;
    EM::SurfaceIOData		sd_;
    EM::SurfaceIODataSelection	sels_;

    virtual bool		prepRead()		{ return true; }
    virtual bool		prepWrite()		{ return true; }
    virtual Executor*		getWriter()		{ return 0; }

    void			init(const EM::Surface*,const IOObj*);
    void			setIOObj(const IOObj*);
};


mClass(EarthModel) dgbEMSurfaceTranslator : public EMSurfaceTranslator
{
public:
    				dgbEMSurfaceTranslator(const char*,const char*);
    virtual			~dgbEMSurfaceTranslator();

    virtual Executor*		reader(EM::Surface&);

protected:

    static BufferString		createHOVName(const char* base,int idx);
    bool			setSurfaceTransform(const IOPar&);
    bool			binary;
    void			getSels(StepInterval<int>&,StepInterval<int>&);

    bool			prepRead();
    Executor*			getWriter();

    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }

    EM::dgbSurfaceReader*	reader_;
};


mClass(EarthModel) dgbEMHorizon3DTranslator : public dgbEMSurfaceTranslator
{				 isTranslator(dgb,EMHorizon3D)
public:
    				dgbEMHorizon3DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon3DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }
};


mClass(EarthModel) dgbEMHorizon2DTranslator : public dgbEMSurfaceTranslator
{				 isTranslator(dgb,EMHorizon2D)
public:
    				dgbEMHorizon2DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon2DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{return false;}
    virtual bool		writeOnlyZ() const		{return true;}
    virtual bool		hasRangeSelection() const	{return false;}
};


mClass(EarthModel) dgbEMFault3DTranslator : public dgbEMSurfaceTranslator
{			       isTranslator(dgb,EMFault3D)
public:
    				dgbEMFault3DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMFault3DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return false;}
    virtual bool		writeOnlyZ() const		{ return false;}
    virtual bool		hasRangeSelection() const	{ return false;}
};


mClass(EarthModel) dgbEMFaultStickSetTranslator : public dgbEMSurfaceTranslator
{				     isTranslator(dgb,EMFaultStickSet)
public:
    				dgbEMFaultStickSetTranslator(const char* unm,
							     const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMFaultStickSetTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return false;}
    virtual bool		writeOnlyZ() const		{ return false;}
    virtual bool		hasRangeSelection() const	{ return false;}
};


#endif

