@echo off
python D:\PyInstaller-3.3.1\pyinstaller.py -F --distpath .\ --workpath .\modify\build --specpath .\modify modify.py
rd /s/q modify