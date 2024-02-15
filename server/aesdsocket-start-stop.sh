#!/bin/sh
# aesdsocket-start-stop


case "$1" in
   start)
      echo "Starting aesdsocket application"
      start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket
      ;;
   stop)
      echo "Stopping aesdsocket application"
      start-stop-daemon -K -n aesdsocket
      ;;
   *)
      echo "Usage: $0 {start|stop}"
      exit 1
esac

exit 0
