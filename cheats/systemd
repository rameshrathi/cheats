1. Manage Services
Task	                                      Command
Start a service                             -	sudo systemctl start <service-name>
Stop a service                              -	sudo systemctl stop <service-name>
Restart a service                           -	sudo systemctl restart <service-name>
Reload service configuration                -	sudo systemctl reload <service-name>
Enable a service at startup                 -	sudo systemctl enable <service-name>
Disable a service at startup                -	sudo systemctl disable <service-name>
Check the status of a service               -	systemctl status <service-name>
List all services	                          - systemctl list-units --type=service
Check if a service is enabled               -	systemctl is-enabled <service-name>
View logs for a service	                    - journalctl -u <service-name>

2. Manage System State
Task	                                      Command
Reboot the system	                          -  sudo systemctl reboot
Shut down the system	                      -  sudo systemctl poweroff
Suspend the system	                        -  sudo systemctl suspend
Hibernate the system	                      -  sudo systemctl hibernate
View current system state	                  -  systemctl is-system-running

3. Manage Systemd Units
Task	                                      Command
List all systemd units	                    -  systemctl list-units
List all failed units	                      -  systemctl --failed
View unit dependencies	                    -  systemctl list-dependencies <unit-name>
Show detailed unit info	                    -  systemctl show <unit-name>
Mask a unit (prevent starting)	            -  sudo systemctl mask <unit-name>
Unmask a unit	                              -  sudo systemctl unmask <unit-name>

4. Create and Manage Unit Files
Task	                                      Command
Create a new service file	                  -  sudo nano /etc/systemd/system/<service-name>.service
Reload unit files after changes	sudo        -  systemctl daemon-reload
Start/enable the new service	              -  sudo systemctl start <service-name>
                                            -  sudo systemctl enable <service-name>

5. View Logs
Task	                                      Command
View all logs	                              -  journalctl
View kernel logs	                          -  journalctl -k
View logs since boot	                      -  journalctl -b
View logs for a specific time	              -  journalctl --since "YYYY-MM-DD HH:MM:SS"
Follow logs in real time	                  -  journalctl -f

6. Analyze System Performance
Task	                                      Command
Check boot time	                            -  systemd-analyze
View service startup times	                -  systemd-analyze blame
View critical chain (boot process)	        -  systemd-analyze critical-chain

7. Manage Targets (Runlevels)
Task	                                      Command
List all targets	                          -  systemctl list-units --type=target
View current target	                        -  systemctl get-default
Set default target	                        -  sudo systemctl set-default <target-name>
Switch to a target	                        -  sudo systemctl isolate <target-name>

8. Timers (Cron Replacement)
Task	                                      Command
List all active timers	                    -  systemctl list-timers
Start a timer manually	                    -  sudo systemctl start <timer-name>
Stop a timer	                              -  sudo systemctl stop <timer-name>


#system file example

[Unit]
Description=My Python Script Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /usr/local/bin/myscript.py
Restart=always
User=myuser
Group=mygroup
WorkingDirectory=/usr/local/bin
Environment="MY_ENV_VAR=value"

[Install]
WantedBy=multi-user.target

