#
# Default IntrepidMUD starter Makefile
# (This will start the script to set up the correct Makefile)
# Copyright 2012-2021 by Mikael Segercrantz, Dan Griffiths and Dave Etheridge
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the license, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
#

all:	newos

reall:	newos

install:	newos

reinstall:	newos

depend:	newos

newos:
	@bash configure/configure

clean:	full

purge:	full

scrub:	full

move:	full

dist:	full

full:
	@echo "No need to perform a cleaning operation now."
	@echo ""

help:
	@echo "IntrepidMUD build system help"
	@echo ""
	@echo "The IntrepidMUD Makefile contains the following make targets to be used:"
	@echo ""
	@echo "      all - Builds every file if needed."
	@echo "    reall - Builds every file, even if not needed."
	@echo "  install - Builds every file if needed and installs the resulting"
	@echo "            executables into the bin/ directory."
	@echo "reinstall - Builds every file, even if not needed, and installs the"
	@echo "            resulting executables into the bin/ directory."
	@echo "   depend - Works out the dependencies needed by the source files."
	@echo "    newos - Works out everything related to compiling on a new target/version"
	@echo "            of the OS, including the dependencies."
	@echo "    clean - Deletes the object files."
	@echo "    purge - As make clean, adding old logs, scripts, reports and emergency"
	@echo "            files."
	@echo "    scrub - As make purge, adding notes and defrags."
	@echo "     move - As make scrub, adding any binaries and other build intermediaries."
	@echo "     dist - As make move, adding any user and character files."
	@echo "     full - As make dist, adding any areas."
	@echo "     help - Shows this help file."
	@echo ""
