#touch calibration
tc_x=900 #naming: touch calibration variable should start with tc_
tc_found=0
tc_done=0
tc_times=x
while true; do 
    if test -e /mnt/D/touch_calibration; then
        if test $tc_found = 0; then
            tc_done=1
			/system/bin/calibrate
            tc_found=1 #don't do it a second time for 1 udisk insertion
        fi
    else
        tc_found=0
    fi
    sleep 1
    ((tc_x--))

    test "${tc_x:2:1}" == 0

#loop 900 times, 15 minutes; 
    if test $tc_x = 0; then
        break
    fi
done &
