"""Touchscreen UI for the Pi 5.

Framework not yet decided (PROJECT_SEED §11): leaning toward FastAPI + HTMX in
a kiosk Chromium so the lab LAN can monitor remotely with no extra work. The
PyQt6/PySide6 option stays open. Keep this package free of broker/control logic
so the UI is a thin view over ModuleManager snapshots and MqttBridge commands.
"""
