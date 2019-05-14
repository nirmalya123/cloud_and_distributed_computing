#!/bin/bash

#################################################
# FILE NAME: KVStoreGrader.sh
#
# DESCRIPTION: Grader for MP2
#
# RUN PROCEDURE:
# $ chmod +x KVStoreGrader.sh
# $ ./KVStoreGrader.sh
#################################################
#!/bin/sh
set -xe

function contains () {
  	local e
  	for e in "${@:2}"
	do
		if [ "$e" == "$1" ]; then
			echo 1
			return 1;
		fi
	done
  	echo 0
}

####
# Main function
####

verbose=$(contains "-v" "$@")
verbose=1

###
# Global variables
###
SUCCESS=0
FAILURE=-1
RF=3
RFPLUSONE=4
CREATE_OPERATION="CREATE OPERATION"
CREATE_SUCCESS="create success"
GRADE=0
DELETE_OPERATION="DELETE OPERATION"
DELETE_SUCCESS="delete success"
DELETE_FAILURE="delete fail"
INVALID_KEY="invalidKey"
READ_OPERATION="READ OPERATION"
READ_SUCCESS="read success"
READ_FAILURE="read fail"
QUORUM=2
QUORUMPLUSONE=3
UPDATE_OPERATION="UPDATE OPERATION"
UPDATE_SUCCESS="update success"
UPDATE_FAILURE="update fail"

echo "============================================" > grade.log
echo "Grading Started" >> grade.log
echo "============================================" >> grade.log

echo ""  >> grade.log
echo "############################" >> grade.log
echo " CREATE TEST" >> grade.log
echo "############################" >> grade.log
echo ""

CREATE_TEST_STATUS="${SUCCESS}"
CREATE_TEST_SCORE=0

make clean > /dev/null 2>&1

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
    ./Application ./testcases/create.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
	./Application ./testcases/create.conf
fi

echo "TEST 1: Create 3 replicas of every key" >> grade.log

create_count=`grep -i "${CREATE_OPERATION}" dbg.log | wc -l`
create_success_count=`grep -i "${CREATE_SUCCESS}" dbg.log | wc -l`
expected_count=$(( ${create_count} * ${RFPLUSONE} ))

if [ ${create_success_count} -ne ${expected_count} ]
then
	CREATE_TEST_STATUS="${FAILURE}"
else
	keys=`grep -i "${CREATE_OPERATION}" dbg.log | cut -d" " -f7`
	for key in ${keys}
	do
		key_create_success_count=`grep -i "${CREATE_SUCCESS}" dbg.log | grep "${key}" | wc -l`
		if [ "${key_create_success_count}" -ne "${RFPLUSONE}" ]
		then
			CREATE_TEST_STATUS="${FAILURE}"
			break
		fi
	done
fi

if [ "${CREATE_TEST_STATUS}" -eq "${SUCCESS}" ]
then
	CREATE_TEST_SCORE=3
fi

# Display score
echo "TEST 1 SCORE..................: ${CREATE_TEST_SCORE} / 3" >> grade.log
# Add to grade
GRADE=$(( ${GRADE} + ${CREATE_TEST_SCORE} ))

#echo ""
#echo "############################"
#echo " CREATE TEST ENDS"
#echo "############################"
#echo ""

echo "" >> grade.log
echo "############################" >> grade.log
echo " DELETE TEST" >> grade.log
echo "############################" >> grade.log
echo "" >> grade.log

DELETE_TEST1_STATUS="${SUCCESS}"
DELETE_TEST2_STATUS="${SUCCESS}"
DELETE_TEST1_SCORE=0
DELETE_TEST2_SCORE=0

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
    ./Application ./testcases/delete.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
	./Application ./testcases/delete.conf
fi

echo "TEST 1: Delete 3 replicas of every key" >> grade.log

delete_count=`grep -i "${DELETE_OPERATION}" dbg.log | wc -l`
valid_delete_count=$(( ${delete_count} - 1 ))
expected_count=$(( ${valid_delete_count} * ${RFPLUSONE} ))
delete_success_count=`grep -i "${DELETE_SUCCESS}" dbg.log | wc -l`

if [ "${delete_success_count}" -ne "${expected_count}" ]
then
	DELETE_TEST1_STATUS="${FAILURE}"
else
	keys=""
	keys=`grep -i "${DELETE_OPERATION}" dbg.log | cut -d" " -f7`
	for key in ${keys}
	do
		if [ $key != "${INVALID_KEY}" ]
		then
			key_delete_success_count=`grep -i "${DELETE_SUCCESS}" dbg.log | grep "${key}" | wc -l`
			if [ "${key_delete_success_count}" -ne "${RFPLUSONE}" ]
			then
				DELETE_TEST1_STATUS="${FAILURE}"
				break
			fi
		fi
	done
fi

echo "TEST 2: Attempt delete of an invalid key" >> grade.log

delete_fail_count=`grep -i "${DELETE_FAILURE}" dbg.log | grep "${INVALID_KEY}" | wc -l`
echo "delete_fail_count ${delete_fail_count} --- expected 4" >> grade.log
if [ "${delete_fail_count}" -ne 4 ]
then
	DELETE_TEST2_STATUS="${FAILURE}"
fi

if [ "${DELETE_TEST1_STATUS}" -eq "${SUCCESS}" ]
then
	DELETE_TEST1_SCORE=3
fi

if [ "${DELETE_TEST2_STATUS}" -eq "${SUCCESS}" ]
then
	DELETE_TEST2_SCORE=4
fi

# Display score
echo "TEST 1 SCORE..................: ${DELETE_TEST1_SCORE} / 3" >> grade.log
echo "TEST 2 SCORE..................: ${DELETE_TEST2_SCORE} / 4" >> grade.log
# Add to grade
GRADE=$(( ${GRADE} + ${DELETE_TEST1_SCORE} ))
GRADE=$(( ${GRADE} + ${DELETE_TEST2_SCORE} ))

#echo ""
#echo "############################"
#echo " DELETE TEST ENDS"
#echo "############################"
#echo ""

echo "" >> grade.log
echo "############################" >> grade.log
echo " READ TEST" >> grade.log
echo "############################" >> grade.log
echo "" >> grade.log

READ_TEST1_STATUS="${FAILURE}"
READ_TEST1_SCORE=0
READ_TEST2_STATUS="${FAILURE}"
READ_TEST2_SCORE=0
READ_TEST3_PART1_STATUS="${FAILURE}"
READ_TEST3_PART1_SCORE=0
READ_TEST3_PART2_STATUS="${FAILURE}"
READ_TEST3_PART2_SCORE=0
READ_TEST4_STATUS="${FAILURE}"
READ_TEST4_SCORE=0
READ_TEST5_STATUS="${FAILURE}"
READ_TEST5_SCORE=0

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
    ./Application ./testcases/read.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
	./Application ./testcases/read.conf
fi

read_operations=`grep -i "${READ_OPERATION}" dbg.log  | cut -d" " -f3 | tr -s ']' ' '  | tr -s '[' ' ' | sort`

cnt=1
for time in ${read_operations}
do
	if [ ${cnt} -eq 1 ]
	then
		echo "TEST 1: Read a key. Check for correct value being read at least in quorum of replicas" >> grade.log
		read_op_test1_time="${time}"
		read_op_test1_key=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test1_time}" | cut -d" " -f7`
		read_op_test1_value=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test1_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 2 ]
	then
		echo "TEST 2: Read a key after failing a replica. Check for correct value being read at least in quorum of replicas" >> grade.log
		read_op_test2_time="${time}"
		read_op_test2_key=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test2_time}" | cut -d" " -f7`
		read_op_test2_value=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test2_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 3 ]
	then
		echo "TEST 3 PART 1: Read a key after failing two replicas. Read should fail" >> grade.log
		read_op_test3_part1_time="${time}"
		read_op_test3_part1_key=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test3_part1_time}" | cut -d" " -f7`
		read_op_test3_part1_value=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test3_part1_time}" | cut -d" " -f9`
        echo "read_op_test3_part1_time $read_op_test3_part1_time --- read_op_test3_part1_key $read_op_test3_part1_key --- read_op_test3_part1_value $read_op_test3_part1_value" >> grade.log
	elif [ ${cnt} -eq 4 ]
	then
		echo "TEST 3 PART 2: Read the key after allowing stabilization protocol to kick in. Check for correct value being read at least in quorum of replicas" >> grade.log
		read_op_test3_part2_time="${time}"
		read_op_test3_part2_key=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test3_part2_time}" | cut -d" " -f7`
		read_op_test3_part2_value=`grep -i "${READ_OPERATION}" dbg.log | grep "${read_op_test3_part2_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 5 ]
	then
		echo "TEST 4: Read a key after failing a non-replica. Check for correct value being read at least in quorum of replicas" >> grade.log
		read_op_test4_time="${time}"
		read_op_test4_key="${read_op_test1_key}"
		read_op_test4_value="${read_op_test1_value}"
	elif [ ${cnt} -eq 6 ]
	then
		echo "TEST 5: Attempt read of an invalid key" >> grade.log
		read_op_test5_time="${time}"
	fi
	cnt=$(( ${cnt} + 1 ))
done

echo "read_op_test1_time $read_op_test1_time" >> grade.log
echo "read_op_test2_time $read_op_test2_time" >> grade.log
echo "read_op_test3_part1_time $read_op_test3_part1_time" >> grade.log
echo "read_op_test3_part2_time $read_op_test3_part2_time" >> grade.log
echo "read_op_test4_time $read_op_test4_time" >> grade.log
echo "read_op_test5_time $read_op_test5_time" >> grade.log

read_test1_success_count=0
read_test2_success_count=0
read_test3_part2_success_count=0
read_test4_success_count=0

read_successes=`grep -i "${READ_SUCCESS}" dbg.log | grep ${read_op_test1_key} | grep ${read_op_test1_value} 2>/dev/null`
if [ "${read_successes}" ]
then
	while read success
	do
		time_of_this_success=`echo "${success}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
		if [ "${time_of_this_success}" -ge "${read_op_test1_time}" -a "${time_of_this_success}" -lt "${read_op_test2_time}" ]
		then
			read_test1_success_count=`expr ${read_test1_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${read_op_test2_time}" -a "${time_of_this_success}" -lt "${read_op_test3_part1_time}" ]
		then
			read_test2_success_count=`expr ${read_test2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${read_op_test3_part2_time}" -a "${time_of_this_success}" -lt "${read_op_test4_time}" ]
		then
			read_test3_part2_success_count=`expr ${read_test3_part2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${read_op_test4_time}" ]
		then
			read_test4_success_count=`expr ${read_test4_success_count} + 1`
		fi
	done <<<"${read_successes}"
fi

read_test3_part1_fail_count=0
read_test5_fail_count=0

read_fails=`grep -i "${READ_FAILURE}" dbg.log 2>/dev/null`
echo ">> read_fails [$read_fails]" >> grade.log
cp dbg.log dbg_read.log
if [ "${read_fails}" ]
then
	while read fail
	do
		time_of_this_fail=`echo "${fail}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
        echo "time_of_this_fail $time_of_this_fail --- read_op_test3_part1_time $read_op_test3_part1_time --- read_op_test3_part2_time $read_op_test3_part2_time"  >> grade.log
		if [ "${time_of_this_fail}" -ge "${read_op_test3_part1_time}" -a "${time_of_this_fail}" -lt "${read_op_test3_part2_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${read_op_test3_part1_key}" | wc -l`
			if [ "${actual_key}"  -eq 1 ]
			then
				read_test3_part1_fail_count=`expr ${read_test3_part1_fail_count} + 1`
                echo "read_test3_part1_fail_count $read_test3_part1_fail_count"  >> grade.log
			fi
		elif [ "${time_of_this_fail}" -ge "${read_op_test5_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${INVALID_KEY}" | wc -l`
			if [ "${actual_key}" -eq 1 ]
			then
				read_test5_fail_count=`expr ${read_test5_fail_count} + 1`
			fi
		fi
	done <<<"${read_fails}"
fi

if [ "${read_test1_success_count}" -eq "${QUORUMPLUSONE}" -o "${read_test1_success_count}" -eq "${RFPLUSONE}" ]
then
	READ_TEST1_STATUS="${SUCCESS}"
fi
if [ "${read_test2_success_count}" -eq "${QUORUMPLUSONE}" ]
then
	READ_TEST2_STATUS="${SUCCESS}"
fi

echo "22 read_test3_part1_fail_count $read_test3_part1_fail_count"  >> grade.log

if [ "${read_test3_part1_fail_count}" -eq 1 ]
then
	READ_TEST3_PART1_STATUS="${SUCCESS}"
fi
if [ "${read_test3_part2_success_count}" -eq "${QUORUMPLUSONE}" -o "${read_test3_part2_success_count}" -eq "${RFPLUSONE}" ]
then
	READ_TEST3_PART2_STATUS="${SUCCESS}"
fi
if [ "${read_test4_success_count}" -eq "${QUORUMPLUSONE}" -o "${read_test4_success_count}" -eq "${RFPLUSONE}" ]
then
	READ_TEST4_STATUS="${SUCCESS}"
fi
if [ "${read_test5_fail_count}" -eq "${QUORUMPLUSONE}" -o "${read_test5_fail_count}" -eq "${RFPLUSONE}" ]
then
	READ_TEST5_STATUS="${SUCCESS}"
fi

if [ "${READ_TEST1_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST1_SCORE=3
fi
if [ "${READ_TEST2_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST2_SCORE=9
fi
if [ "${READ_TEST3_PART1_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST3_PART1_SCORE=9
fi
if [ "${READ_TEST3_PART2_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST3_PART2_SCORE=10
fi
if [ "${READ_TEST4_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST4_SCORE=6
fi
if [ "${READ_TEST5_STATUS}" -eq "${SUCCESS}" ]
then
	READ_TEST5_SCORE=3
fi

# Display score
echo "TEST 1 SCORE..................: ${READ_TEST1_SCORE} / 3" >> grade.log
echo "TEST 2 SCORE..................: ${READ_TEST2_SCORE} / 9" >> grade.log
echo "TEST 3 PART 1 SCORE..................: ${READ_TEST3_PART1_SCORE} / 9" >> grade.log
echo "TEST 3 PART 2 SCORE..................: ${READ_TEST3_PART2_SCORE} / 10" >> grade.log
echo "TEST 4 SCORE..................: ${READ_TEST4_SCORE} / 6" >> grade.log
echo "TEST 5 SCORE..................: ${READ_TEST5_SCORE} / 3" >> grade.log
# Add to grade
GRADE=`expr ${GRADE} + ${READ_TEST1_SCORE}`
GRADE=`expr ${GRADE} + ${READ_TEST2_SCORE}`
GRADE=`echo ${GRADE} ${READ_TEST3_PART1_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${READ_TEST3_PART2_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${READ_TEST4_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${READ_TEST5_SCORE} | awk '{print $1 + $2}'`

#echo ""
#echo "############################"
#echo " READ TEST ENDS"
#echo "############################"
#echo ""

echo "" >> grade.log
echo "############################" >> grade.log
echo " UPDATE TEST" >> grade.log
echo "############################" >> grade.log
echo "" >> grade.log

UPDATE_TEST1_STATUS="${FAILURE}"
UPDATE_TEST1_SCORE=0
UPDATE_TEST2_STATUS="${FAILURE}"
UPDATE_TEST2_SCORE=0
UPDATE_TEST3_PART1_STATUS="${FAILURE}"
UPDATE_TEST3_PART1_SCORE=0
UPDATE_TEST3_PART2_STATUS="${FAILURE}"
UPDATE_TEST3_PART2_SCORE=0
UPDATE_TEST4_STATUS="${FAILURE}"
UPDATE_TEST4_SCORE=0
UPDATE_TEST5_STATUS="${FAILURE}"
UPDATE_TEST5_SCORE=0

if [ "${verbose}" -eq 0 ]
then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    if [ $? -ne "${SUCCESS}" ]
    then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
    ./Application ./testcases/update.conf > /dev/null 2>&1
else
	make clean
	make
	if [ $? -ne "${SUCCESS}" ]
	then
    	echo "COMPILATION ERROR !!!" >> grade.log
    	exit
    fi
	./Application ./testcases/update.conf
fi

update_operations=`grep -i "${UPDATE_OPERATION}" dbg.log  | cut -d" " -f3 | tr -s ']' ' '  | tr -s '[' ' ' | sort`

cnt=1
for time in ${update_operations}
do
	if [ ${cnt} -eq 1 ]
	then
		echo "TEST 1: Update a key. Check for correct value being updated at least in quorum of replicas" >> grade.log
		update_op_test1_time="${time}"
		update_op_test1_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test1_time}" | cut -d" " -f7`
		update_op_test1_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test1_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 2 ]
	then
		echo "TEST 2: Update a key after failing a replica. Check for correct value being updated at least in quorum of replicas" >> grade.log
		update_op_test2_time="${time}"
		update_op_test2_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test2_time}" | cut -d" " -f7`
		update_op_test2_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test2_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 3 ]
	then
		echo "TEST 3 PART 1: Update a key after failing two replicas. Update should fail" >> grade.log
		update_op_test3_part1_time="${time}"
		update_op_test3_part1_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part1_time}" | cut -d" " -f7`
		update_op_test3_part1_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part1_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 4 ]
	then
		echo "TEST 3 PART 2: Update the key after allowing stabilization protocol to kick in. Check for correct value being updated at least in quorum of replicas" >> grade.log
		update_op_test3_part2_time="${time}"
		update_op_test3_part2_key=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part2_time}" | cut -d" " -f7`
		update_op_test3_part2_value=`grep -i "${UPDATE_OPERATION}" dbg.log | grep "${update_op_test3_part2_time}" | cut -d" " -f9`
	elif [ ${cnt} -eq 5 ]
	then
		echo "TEST 4: Update a key after failing a non-replica. Check for correct value being updated at least in quorum of replicas" >> grade.log
		update_op_test4_time="${time}"
		update_op_test4_key="${update_op_test1_key}"
		update_op_test4_value="${update_op_test1_value}"
	elif [ ${cnt} -eq 6 ]
	then
		echo "TEST 5: Attempt update of an invalid key" >> grade.log
		update_op_test5_time="${time}"
	fi
	cnt=$(( ${cnt} + 1 ))
done
echo "update_op_test1_time $update_op_test1_time" >> grade.log
echo "update_op_test2_time $update_op_test2_time" >> grade.log
echo "update_op_test3_part1_time $update_op_test3_part1_time" >> grade.log
echo "update_op_test3_part2_time $update_op_test3_part2_time" >> grade.log
echo "update_op_test4_time $update_op_test4_time" >> grade.log
echo "update_op_test5_time $update_op_test5_time" >> grade.log
update_test1_success_count=0
update_test2_success_count=0
update_test3_part2_success_count=0
update_test4_success_count=0

update_successes=`grep -i "${UPDATE_SUCCESS}" dbg.log | grep ${update_op_test1_key} | grep ${update_op_test1_value} 2>/dev/null`
if [ "${update_successes}" ]
then
	while read success
	do
		time_of_this_success=`echo "${success}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
		if [ "${time_of_this_success}" -ge "${update_op_test1_time}" -a "${time_of_this_success}" -lt "${update_op_test2_time}" ]
		then
			update_test1_success_count=`expr ${update_test1_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test2_time}" -a "${time_of_this_success}" -lt "${update_op_test3_part1_time}" ]
		then
			update_test2_success_count=`expr ${update_test2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test3_part2_time}" -a "${time_of_this_success}" -lt "${update_op_test4_time}" ]
		then
			update_test3_part2_success_count=`expr ${update_test3_part2_success_count} + 1`
		elif [ "${time_of_this_success}" -ge "${update_op_test4_time}" ]
		then
			update_test4_success_count=`expr ${update_test4_success_count} + 1`
		fi
	done <<<"${update_successes}"
fi

update_test3_part1_fail_count=0
update_test5_fail_count=0

update_fails=`grep -i "${UPDATE_FAILURE}" dbg.log 2>/dev/null`
echo "$update_success" >> grade.log
echo "time_of_this_fail ${time_of_this_fail} --- update_op_test3_part1_time ${update_op_test3_part1_time} --- update_op_test3_part2_time ${update_op_test3_part2_time} "
if [ "${update_fails}" ]
then
	while read fail
	do
		time_of_this_fail=`echo "${fail}" | cut -d" " -f2 | tr -s '[' ' ' | tr -s ']' ' '`
		if [ "${time_of_this_fail}" -ge "${update_op_test3_part1_time}" -a "${time_of_this_fail}" -lt "${update_op_test3_part2_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${update_op_test3_part1_key}" | wc -l`
			if [ "${actual_key}"  -eq 1 ]
			then
				update_test3_part1_fail_count=`expr ${update_test3_part1_fail_count} + 1`
			fi
		elif [ "${time_of_this_fail}" -ge "${update_op_test5_time}" ]
		then
			actual_key=`echo "${fail}" | grep "${INVALID_KEY}" | wc -l`
			if [ "${actual_key}" -eq 1 ]
			then
				update_test5_fail_count=`expr ${update_test5_fail_count} + 1`
			fi
		fi
	done <<<"${update_fails}"
fi

if [ "${update_test1_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test1_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST1_STATUS="${SUCCESS}"
fi
if [ "${update_test2_success_count}" -eq "${QUORUMPLUSONE}" ]
then
	UPDATE_TEST2_STATUS="${SUCCESS}"
fi
if [ "${update_test3_part1_fail_count}" -eq 1 ]
then
	UPDATE_TEST3_PART1_STATUS="${SUCCESS}"
fi
if [ "${update_test3_part2_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test3_part2_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST3_PART2_STATUS="${SUCCESS}"
fi
if [ "${update_test4_success_count}" -eq "${QUORUMPLUSONE}" -o "${update_test4_success_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST4_STATUS="${SUCCESS}"
fi
if [ "${update_test5_fail_count}" -eq "${QUORUMPLUSONE}" -o "${update_test5_fail_count}" -eq "${RFPLUSONE}" ]
then
	UPDATE_TEST5_STATUS="${SUCCESS}"
fi

if [ "${UPDATE_TEST1_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST1_SCORE=3
fi
if [ "${UPDATE_TEST2_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST2_SCORE=9
fi
if [ "${UPDATE_TEST3_PART1_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST3_PART1_SCORE=9
fi
if [ "${UPDATE_TEST3_PART2_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST3_PART2_SCORE=10
fi
if [ "${UPDATE_TEST4_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST4_SCORE=6
fi
if [ "${UPDATE_TEST5_STATUS}" -eq "${SUCCESS}" ]
then
	UPDATE_TEST5_SCORE=3
fi

# Display score
echo "TEST 1 SCORE..................: ${UPDATE_TEST1_SCORE} / 3" >> grade.log
echo "TEST 2 SCORE..................: ${UPDATE_TEST2_SCORE} / 9" >> grade.log
echo "TEST 3 PART 1 SCORE..................: ${UPDATE_TEST3_PART1_SCORE} / 9" >> grade.log
echo "TEST 3 PART 2 SCORE..................: ${UPDATE_TEST3_PART2_SCORE} / 10" >> grade.log
echo "TEST 4 SCORE..................: ${UPDATE_TEST4_SCORE} / 6" >> grade.log
echo "TEST 5 SCORE..................: ${UPDATE_TEST5_SCORE} / 3" >> grade.log
# Add to grade
GRADE=`echo ${GRADE} ${UPDATE_TEST1_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST2_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST3_PART1_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST3_PART2_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST4_SCORE} | awk '{print $1 + $2}'`
GRADE=`echo ${GRADE} ${UPDATE_TEST5_SCORE} | awk '{print $1 + $2}'`

#echo ""
#echo "############################"
#echo " UPDATE TEST ENDS"
#echo "############################"
#echo ""

echo "" >> grade.log
echo "TOTAL GRADE: ${GRADE} / 90" >> grade.log
echo "" >> grade.log
