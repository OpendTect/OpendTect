#!/bin/bash
#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id$
#
#_______________________________________________________________________________

# The purpose of this script is to make a .pro file to tell lupdate which
# files to scan and for which languages a ts file has to be created.
# It will:
# - make  a copy of an od directory
# - replace mExpClass and mClass by class
# - replace *_static_tr( "myfunc", ... ) with static_func_myfunc( ... )
# - create lupdate.pro
# - run lupdate for plural (*en-us.ts) and non-plural translations (all others).
# - copy the new translations into the work-directory.
# - remove the temporary copy

if [ $# -lt 5 ]; then
    echo "Usage : $0 <sourcedir> <tsbasedir> <binarydir> <application> <lupdate> [--quiet]"
    exit 1
fi

quiet=off

if [ $# -gt 5 ] && [ "$6" == "--quiet" ]; then
    quiet=on
fi

sourcedir=$1
tsbasedir=$2
binarydir=$3
application=$4
lupdate=$5
tmpoddir=/tmp/lupdate_tmp_$$
outputdir=${binarydir}/data/localizations/generated

control_c()
 # Control-C Press
{
   cleanup
   exit $?
}

cleanup()
{
   if [ -e "${tmpoddir}" ]; then
       rm -rf ${tmpoddir}
   fi
   return $?
}


filter_files() {
    #1. Insert Q_OBJECT on all translated classed by replace TextTranslationClass
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/m.*TextTranslationClass(.*)/Q_OBJECT/g' -iTMP

    #2. Remove mExpClass, mExpStruct, mStruct and mClass and replace by class/struct
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/mExpClass(.*)/class /g' -iTMP
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/mClass(.*)/class /g' -iTMP
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/mStruct(.*)/struct /g' -iTMP
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/mExpStruct(.*)/struct /g' -iTMP

    #3 Cleanup enum translation so lupdate will understand it
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/mEnumTr/EnumDefImpl::tr /g' -iTMP

    #4 Remove templates as they screw up lupdate Template classes are treated as
    #  non template classes when it comes to translation
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/template[ \t]*<[^>]*>//g' -iTMP

    #5 Remove bit field declarations i.e. "unsigned a:4" as they screw up lupdate.
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/:[1-9]//g' -iTMP

    #8 Replace *_static_tr("Function", "text") with QT_TRANSLATE_NOOP("static_func_Function", "text");
    echo $@ | xargs -n 100 -P ${nrcpu} sed -e 's/[^( \t]*_static_tr[^(]*([^"]*"/QT_TRANSLATE_NOOP("static_func_/g' -iTMP

    #9. Filter away macros in sources
    echo $@ | xargs -n 100 -P ${nrcpu} ${php} ${scriptdir}/remove_macros.php
}


#Get the directory where the scripts are
pushd `dirname $0` > /dev/null
scriptdir=`pwd`
popd > /dev/null

nrcpu=`${scriptdir}/GetNrProc`
if [ "${nrcpu}" == "" ]; then
    nrcpu=1
fi

php=`which php`
if [ "${php}" == "" ]; then
    echo "PHP is not installed"
    exit 1
fi

removetmpoddirsed="s/\/tmp\/"
removetmpoddirsed+=`basename $tmpoddir`
removetmpoddirsed+="\///g"

kernel=`uname -a | awk '{print $1}'`
if [ "${kernel}" == "Darwin" ]; then
    lupdate_dir=`dirname ${lupdate}`
    export DYLD_LIBRARY_PATH=${lupdate_dir}/../lib
fi

if [ -e $tmpoddir ]; then
    cleanup
fi

mkdir $tmpoddir

#Catch premature exit so we can remove temporary directory
trap control_c 0 SIGHUP SIGINT SIGQUIT SIGABRT SIGKILL SIGALRM SIGSEGV SIGTERM


#Copy all src, include and pluginfiles (excluding CMake-stuff) to temp folder
olddir=`pwd`
cd ${sourcedir}
if [ -e ${sourcedir}/src ]; then
    find src -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/include ]; then
    find include -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/plugins ]; then
    find plugins -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi

projectdir="${tmpoddir}/data/localizations/source"
mkdir -p ${projectdir}

cd ${olddir}

shopt -s nullglob
#Copy existing ts-files ot project dir
cp -a ${tsbasedir}/data/localizations/source/${application}*.ts ${projectdir}
if [ -d ${tsbasedir}/data/localizations/inprogress ]; then
    cp -a ${tsbasedir}/data/localizations/inprogress/${application}*.ts ${projectdir}
fi

#Check if all projects exist in destination if so, check dates
missing=0
for fnm in ${projectdir}/${application}*.ts ; do
    basefnm=`basename ${fnm}`
    if [ ! -e "${outputdir}/${basefnm}" ]; then
	missing=1
    fi
done

if [ ${missing} -eq 0 ]; then

    #Set arguments to stat as they are different on MacOS
    statarg='-c %Y'
    kernel=`uname -a | awk '{print $1}'`
    if [ "${kernel}" == "Darwin" ]; then
	statarg='-f %m'
    fi

    #Get the timestamp of all ts-files, and get the oldest one
    oldest_output_timestamp=`find ${outputdir} -name "${application}*.ts" -type f \
		    -exec stat ${statarg} '{}' \; \
		    | sort -rn \
		    | tail -1`

    #Get the timestamp of all input files, and get the newest one
    newest_input_timestamp=`find ${tmpoddir} \
		\( -path "*.h" -o -path "*.cc" -o -name "${application}*.ts" \) \
		-type f -exec stat ${statarg} '{}' \; | sort -n | tail -1`

    if [ ${oldest_output_timestamp} -gt ${newest_input_timestamp} ]; then
	if [ "${quiet}" == "off" ]; then
	    echo "Nothing to do: Dependencies are older than target."
	fi
	#Remove temporary dir
	cleanup
	exit 0;
    fi
fi

profnm=${projectdir}/normaltrans.pro

cd ${projectdir}

headers=`find $tmpoddir -path "*.h"`
dirfile=${tmpoddir}/dirs
for header in ${headers}; do
    dirname ${header} >> ${dirfile}
done

dirs=`sort -u ${dirfile}`

#Create file with all files
filelist="filelist.txt"

echo "" >> ${filelist}

echo -n "HEADERS = " >> ${filelist}
for fnm in ${headers} ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

echo "" >> ${filelist}

sources=`find ${tmpoddir} -path "*.cc"`

echo -n "SOURCES = " >> ${filelist}
for fnm in $sources ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

# Create a list of target .ts files for normal ts files
echo -n "TRANSLATIONS = " > ${profnm}

echo " \\" >> ${profnm}
echo -n "    ${application}_template.ts" >> ${profnm}

nonomatch=1
for fnm in ${application}*.ts ; do
    #Don't run any plural here, as that is handled in plural project
    if [[ "${fnm}" =~ .*en-us.ts ]]; then
	continue
    fi

    #Is explicitly added above, so remove eventual entry from the list to avoid duplicates
    if [[ "${fnm}" =~ .*_template.ts ]]; then
	continue
    fi

    echo " \\" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
done

cat ${filelist} >> ${profnm}

echo "" >> ${profnm}
echo -n "INCLUDEPATH += " >> ${profnm}

for dir in ${dirs} ; do
    echo " \\" >> ${profnm}
    echo -n "	${dir}" >>${profnm}
done

#Create a list of .ts files for plural operations
pluralpro=$projectdir/plural.pro
pluralfile=${application}_en-us.ts
if [ -e ${pluralfile} ]; then
    echo -n "TRANSLATIONS = " > ${pluralpro}
    echo " \\" >> ${pluralpro}
    echo -n "    ${pluralfile}" >> ${pluralpro}
    cat ${filelist} >> ${pluralpro}

    echo "" >> ${pluralpro}
    echo -n "INCLUDEPATH += " >> ${pluralpro}

    for dir in ${dirs} ; do
	    echo " \\" >> ${pluralpro}
	    echo -n "	${dir}" >>${pluralpro}
    done
fi

#Remove the filelist
rm -rf ${filelist}

#Filter the sources for patterns
filter_files ${sources}


#Filter the headers for patterns

filter_files ${headers}

result=0
#Run lupdate in the background and save jobid
errfile=${tmpoddir}/stderr
${lupdate} -silent -locations relative ${profnm} 2> ${errfile} &
update_pid=$!

if [ -e ${pluralpro} ]; then
    #Run this in the forground, and the other job is going in the background
    plural_errfile=${tmpoddir}/stderr_plural
    ${lupdate} -silent -locations relative -pluralonly ${pluralpro} 2> ${plural_errfile}
    errors=`cat ${plural_errfile}`
    if [ "${errors}" != "" ]; then
	echo "Errors during plural processing:"
	cat ${plural_errfile} | sed -e $removetmpoddirsed -e 's/Q_OBJECT/*TextTranslationClass/g'
	result=1
    fi

    #Test if unfinished entries are found
    grep 'type="unfinished"' --before-context=1 ${application}_en-us.ts > /dev/null
    if [ $? -eq 0 ]; then
	echo "The following plural phrase(s) are not translated:"
	grep 'type="unfinished"' --before-context=1 ${application}_en-us.ts \
			| head -1 \
			| sed 's/<source>//g' \
			| sed 's/<\/source>//g' \
			| sed 's/ //g'
	echo
	echo "Please run:"
	echo
	echo  1. cp ${outputdir}/${application}_en-us.ts ${tsbasedir}/data/localizations/source/${application}_en-us.ts
	echo  2. ${scriptdir}/linguist.csh ${tsbasedir}/data/localizations/source/${application}_en-us.ts
	echo  3. Fix the problem
	echo  4. Commit
	echo
	result=1
    fi
fi

#Wait for background job to complete
wait ${update_pid}

errors=`cat ${errfile}`
if [ "${errors}" != "" ]; then
    echo "Errors during non-plural processing:"
    cat ${errfile} | sed -e $removetmpoddirsed -e 's/Q_OBJECT/*TextTranslationClass/g'
    result=1
fi

#Copy results back
rm -f ${outputdir}/${application}*.ts
rsync *.ts ${outputdir}/

#Remove temporary dir
cleanup

#Go back to starting dir
cd ${olddir}

exit ${result}
