#!/usr/bin/env bash
function rmshm()
{
get_shmid=`ipcs -m|grep -w 0|awk '{print $2}'`
for i in $get_shmid
do
    get_pid=`ipcs -p | grep $i | awk '{print $4}'`
    get_pids=`ps -ef | grep -v "grep" | grep $get_pid | wc -l`
    if [ $get_pids -eq 0 ];then
        echo "delete shmid $i"
        ipcrm -m $i
    else
         echo "shmid $i is being used"
         return 0
    fi
done
echo "OK"
return 0
}
 
rmshm