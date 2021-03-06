#!/bin/bash

mount /mnt/synology
if [ ! -d "/mnt/synology/IPC100A_Diffs" ]; then
  echo "Input Directory not found"
  umount /mnt/synology
  exit
fi

# /usr/local/bin/pictures2video.bash /mnt/synology/IPC100A_Diffs /mnt/synology/video IPC100A
docker run \
    -a stdout -a stderr --rm \
    --user `id -u`:`id -g` \
    -v "/mnt/synology/IPC100A_Diffs":"/mnt/pictures_input" \
    -v "/mnt/synology/video":"/mnt/video_output" \
    pictures2video:prod \
    /mnt/pictures_input /mnt/video_output IPC100A

umount /mnt/synology
