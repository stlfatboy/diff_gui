import win32api
import win32con
import os

print "code version: 2018.10.12"

# default_config_filepath = 'C:\\Users\\Administrator\\AppData\\Roaming\\Subversion'
path2append = '\\AppData\\Roaming\\Subversion'

seglen = 50
slash = os.sep


def pauseandexit(exitkey='input anykey to exit...>'):
    _ = raw_input(exitkey)
    exit()


def readselfconfigpath(file='self_configpath.txt'):
    print 'trying open {}'.format(file)
    try:
        fr_selfpath = open(file, 'r')
    except IOError:  # can not open self_configpath.txt, create it
        print '"{}" not found.'.format(file)
        fr_selfpath = open(file, 'w')
        fr_selfpath.close()
        print 'created "{}".'.format(file)
        print 'pls find SVN config file path manually and ' \
              'copy it in the "{}"\nthen run modify.exe again.'.format(file)
        pauseandexit()
    else:  # opened self_configpath.txt, return
        print '{} opened.'.format(file)
        return fr_selfpath


def readconfigbyself(selfpath, slash, filename):
    print 'trying open config from self_configpath.txt'
    try:
        fr = open(selfpath[0] + slash + 'config', 'r')
    except IOError:  # can not open SVN config at self path
        print 'IOError: "{}" not found'.format(filename.split(slash)[-1])
        print 'path in self_configpath.txt is wrong.\npls check.'
        pauseandexit()
    else:
        print 'open config by self_configpath.txt succeed.'
        return fr


# get path to config file
print 'getting path to SVN config file'.center(seglen, '=')
# get path to Documents directory
key = win32api.RegOpenKey(win32con.HKEY_CURRENT_USER,
                          r'Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders',
                          0, win32con.KEY_READ)
doc_path = win32api.RegQueryValueEx(key, 'Personal')[0]
print 'doc_path is: "{}"'.format(doc_path)
# use doc_path get config path
configpath = slash.join(doc_path.split(slash)[:-1]) + path2append
filename = configpath + slash + 'config'
print 'filename is: "{}"'.format(filename)

# read config file
print 'reading SVN config file'.center(seglen, '=')
try:
    fr = open(filename, 'r')
except IOError:  # can not open SVN config
    print 'IOError: "{}" not found'.format(filename.split(slash)[-1])

    fr_selfpath = readselfconfigpath()
    selfpath = fr_selfpath.readlines()  #read manually found config file path
    fr_selfpath.close()
    fr = readconfigbyself(selfpath, slash, filename)  # read config file by manually found path

else:
    print 'open config from default path succeed.'

lines = fr.readlines()
fr.close()
print 'read SVN config end'

# get current path
print 'getting current path'.center(seglen, '=')
pwd = os.getcwd()
print 'path = "{}"'.format(pwd)
str2write = 'diff-cmd = ' + pwd + slash + 'diff.bat\n'
print 'string to write: "{}"'.format(str2write[:-1])

# write config
print 'writing config file'.center(seglen, '=')
str2cmpr = '# diff-cmd = diff_program (diff, gdiff, etc.)'
fw_config = open(filename, 'w')
for line in lines:  # write config by lines
    if line.endswith('diff.bat\n') and line.startswith('diff-cmd'):  # check if it's a existing path
        print 'find existing path at line: {}.'.format(lines.index(line)+1)
        print 'removing path: "{}".'.format(line[:-1])
        continue  # remove existing path
    if str2cmpr in line:  # locate where to write
        print 'located writing line!'
        print 'writing "{}"!'.format(str2write[:-1])
        fw_config.write(str2write)
    fw_config.write(line)
fw_config.close()
print 'write config end.'

# write diff.bat
print 'writing diff.bat'.center(seglen, '=')
fw_diff = open(pwd + slash + 'diff.bat', 'w')
fw_diff.write('@echo off' + '\n')
fw_diff.write('"{}" -U9999999 -L %3 -L %5 %6 %7'.format(pwd + slash + 'diff') + '\n')
fw_diff.close()
print 'write diff.bat end.'

# write run.bat
print 'writing run.bat'.center(seglen, '=')
fw_run = open(pwd + slash + 'run.bat', 'w')
fw_run.write('"{}" %cd%'.format(pwd + slash + 'Diff_GUI.exe') + '\n')
fw_run.close()
print 'write run.bat end.'

# read 'Demo_menu.txt' and modify contents to write
Demo_name = 'Demo_menu.txt'
print 'reading {}'.format(Demo_name).center(seglen, '=')
try:
    fr_demo_rc = open(Demo_name, 'r')
except IOError:
    print 'IOError: "{}" not found'.format(Demo_name)
    print 'Check if "{}" is at current path'.format(Demo_name)
    pauseandexit()
else:
    lines = fr_demo_rc.readlines()
    fr_demo_rc.close()
    print 'read {} over'.format(Demo_name)
    print 'modifying contents'.center(seglen, '=')
    for i, line in enumerate(lines):
        if 'CurrentPath' in line:
            # replace 'CurrentPath' with actual path
            # replace single slash with double slash
            lines[i] = line.replace('CurrentPath', pwd.replace(slash, '\\'*2))
    print 'modify contents end'

# write menu.reg   
print 'writing menu.reg'.center(seglen, '=')
fw_run = open(pwd + slash + 'menu.reg', 'w')
fw_run.writelines(lines)
fw_run.close()
print 'write menu.reg end.'

os.system("menu.reg")
os.system("exit /0")
#pauseandexit(exitkey='Done.\ninput anykey to exit...>')
