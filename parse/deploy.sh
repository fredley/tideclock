#!/bin/bash

set -e

trap 'printf "\033[0;31mFAIL\033[0m\n"' ERR

printf "Pulling from git...\n"
git pull origin
printf "......................"
printf "\033[0;32mOK\033[0m\n"

printf "Copying files........."
cp -r ./* /opt/tideparse
cp uwsgi.ini /opt/tideparse
cp tideparse.service /etc/systemd/system/
printf "\033[0;32mOK\033[0m\n"

printf "Setting ownership....."
chown -R www-data:www-data /opt/tideparse
printf "\033[0;32mOK\033[0m\n"


printf "Restarting services..."
systemctl daemon-reload
systemctl restart tideparse
systemctl restart nginx
printf "\033[0;32mOK\033[0m\n"

printf "\033[0;32mAll Done\033[0m\n"
