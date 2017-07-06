# esp-pc-monitor

Monitor del estado del pc via web, con funcion de encendido y reset

## Caracteristicas

- Web de monitorización de estado con funciones de encendio, apagado y reset
- Monitor activo del PC para detectar cuelgues y reiniciar

## Serial commands

- Connect to wlan: ```{wlan|WLAN_SSID|password}```
- Power on PC: ```{poweron}```
- Power off PC: ```{poweroff}```
- Reboot PC: ```{reboot}```
- Enable/disable monitor: ```{power_monitor|1} {power_monitor|0}```
- Ping the monitor: ```{pm_tick}```