#!/bin/bash
# autostart for mavproxy on Porter

(
date
#export PATH=$PATH:/bin:/sbin:/usr/bin:/usr/local/bin:/root/.local/bin
#export HOME=/root
cd /root/cuav
screen -d -m -S MAVProxy -s /bin/bash /root/Falcon/mav_falcon.sh
) #> /root/autostart.log 2>&1
exit 0
