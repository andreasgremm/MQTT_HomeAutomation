#!/bin/bash
#
####
#
#     Start Image_Diff for IPC100A on the Homeautomation Server
#
mount /mnt/synology

if [ ! -d "/mnt/synology/FTP" ]; then
  echo "Input Directory not found"
  umount /mnt/synology
  exit
fi

if [ ! -d "/mnt/synology/IPC100A_Diffs" ]; then
  echo "Output Directory not found"
  umount /mnt/synology
  exit
fi

Image_diff.bash /mnt/synology/FTP /mnt/synology/IPC100A_Diffs

umount /mnt/synology
