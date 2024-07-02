#!/bin/bash
echo url="https://www.duckdns.org/update?domains=intellian.duckdns.org&token=b809f202-7f4c-4b66-8bbe-9f43ee9173c6&ip=" | curl -k -o /home/jjm/my_web_server/duck.log -K -
