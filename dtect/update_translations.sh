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
# - run lupdate for plural (*en.ts) and non-plural translations (all others).
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
outputdir=${binarydir}/data/translations

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
removetmpoddirsed+=`basename ${tmpoddir}`
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

projectdir="${tmpoddir}/data/translations"
mkdir -p ${projectdir}

#Converting from *.qm file to *.ts file for update, these files will be deleted later.

cd ${olddir}

shopt -s nullglob

for qmfile in ${tsbasedir}/data/translations/${application}*.qm ; do
    qmfilename=`basename ${qmfile} .qm`
    binpath=`dirname ${lupdate}`
    convertpath=${binpath}/lconvert
    ${binpath}/lconvert -i ${qmfile} -o ${tsbasedir}/data/translations/${qmfilename}.ts
done


#Copy existing ts-files ot project dir
cp -a ${tsbasedir}/data/translations/${application}*.ts ${projectdir}
if [ -d ${tsbasedir}/data/translations ]; then
    cp -a ${tsbasedir}/data/translations/${application}*.ts ${projectdir}
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
    if [[ "${fnm}" =~ .*en.ts ]]; then
	continue
    fi
    #Is explicitly added above, so remove eventual entry from the list to avoid duplicates
    if [[ "${fnm}" =~ .*_template.ts ]]; then
	continue
    fi

    echo " \\" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
done
echo "I am Line 238"
echo ${profnm} 
#vi ${profnm}
cat ${filelist} >> ${profnm}

echo "" >> ${profnm}
echo -n "INCLUDEPATH += " >> ${profnm}
#vi ${profnm}
for dir in ${dirs} ; do
    echo " \\" >> ${profnm}
    echo -n "	${dir}" >>${profnm}
done

#Create a list of .ts files for plural operations
pluralpro=$projectdir/plural.pro
#vi $projectdir/plural.pro
pluralfile=${application}_en.ts
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
echo "I am Line 273"

echo "Outside  ${pluralfile}"
#Remove the filelist
rm -rf ${filelist}

#Filter the sources for patterns
filter_files ${sources}


#Filter the headers for patterns

filter_files ${headers}

result=0
#Run lupdate in the background and save jobid
errfile=${tmpoddir}/stderr
${lupdate}  -locations absolute -pro ${profnm} 2> ${errfile} &
update_pid=$!
#vi ${tmpoddir}/stderr
if [ -e ${pluralpro} ]; then
    #Run this in the forground, and the other job is going in the background
    plural_errfile=${tmpoddir}/stderr_plural
    ${lupdate} -locations relative -pluralonly -pro ${pluralpro} 2> ${plural_errfile}
    errors=`cat ${plural_errfile}`
    if [ "${errors}" != "" ]; then
	echo "Errors during plural processing:"
	cat ${plural_errfile} | sed -e $removetmpoddirsed -e 's/Q_OBJECT/*TextTranslationClass/g'
	result=1
    fi

    #Test if unfinished entries are found
    grep 'type="unfinished"' --before-context=1 ${application}_en.ts > /dev/null
    if [ $? -eq 0 ]; then
	echo "The following plural phrase(s) are not translated:"
	grep 'type="unfinished"' --before-context=1 ${application}_en.ts \
			| head -1 \
			| sed 's/<source>//g' \
			| sed 's/<\/source>//g' \
			| sed 's/ //g'
	echo
	echo "Please run:"
	echo
	echo  1. cp ${outputdir}/${application}_en.ts ${tsbasedir}/data/translations/${application}_en.ts
	echo  2. ${scriptdir}/linguist.csh ${tsbasedir}/data/translations/${application}_en.ts
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
