VENV_DIR=/opt/ecofloc/gui/venv/
FLOC_DIR=/opt/ecofloc/
GUI_DIR=/opt/ecofloc/gui/
REQUIREMENTS=python_libs.conf

all: clean uninstall cpu sd ram nic floc install

create_ecofloc_folder: 
	mkdir -p $(FLOC_DIR)
create_gui_folder: 
	mkdir -p $(GUI_DIR)

######## CPU ##########

cpu:
	$(MAKE) -C ecofloc-cpu

install_cpu: 
	cp ecofloc-cpu/ecofloc-cpu.out $(FLOC_DIR)
	cp ecofloc-cpu/cpu_settings.conf $(FLOC_DIR)
	ln -sf /opt/ecofloc/ecofloc-cpu.out /usr/local/bin/ecofloc-cpu
	ln -sf /opt/ecofloc/cpu_settings.conf /usr/local/bin/cpu_settings.conf
clean_cpu:
	$(MAKE) -C ecofloc-cpu clean

uninstall_cpu:
	rm -f /usr/local/bin/ecofloc-cpu
	rm -f /usr/local/bin/cpu_settings.conf

######## SD ##########

sd:
	$(MAKE) -C ecofloc-sd

install_sd: 
	cp ecofloc-sd/ecofloc-sd.out $(FLOC_DIR)
	cp ecofloc-sd/sdFeatures.conf $(FLOC_DIR)
	ln -sf /opt/ecofloc/ecofloc-sd.out /usr/local/bin/ecofloc-sd
	ln -sf /opt/ecofloc/sdFeatures.conf /usr/local/bin/sdFeatures.conf

clean_sd:
	$(MAKE) -C ecofloc-sd clean

uninstall_sd:
	rm -f /usr/local/bin/ecofloc-sd
	rm -f /usr/local/bin/sdFeatures.conf
	
######## RAM ##########

ram:
	$(MAKE) -C ecofloc-ram

install_ram: 
	cp ecofloc-ram/ecofloc-ram.out $(FLOC_DIR)
	ln -sf /opt/ecofloc/ecofloc-ram.out /usr/local/bin/ecofloc-ram

clean_ram:
	$(MAKE) -C ecofloc-ram clean

uninstall_ram:
	rm -f /usr/local/bin/ecofloc-ram

######## NIC ##########

nic:
	$(MAKE) -C ecofloc-nic

install_nic: 
	cp ecofloc-nic/ecofloc-nic.out $(FLOC_DIR)
	ln -sf /opt/ecofloc/ecofloc-nic.out /usr/local/bin/ecofloc-nic

clean_nic:
	$(MAKE) -C ecofloc-nic clean

uninstall_nic:
	rm -f /usr/local/bin/ecofloc-nic

######## FLOC ##########

floc:
	gcc find_by_name.c ecofloc.c -o ecofloc

######## GUI ##########

# Main target to install the GUI
install_gui: create_gui_folder setup_venv copy_files
	chmod +x /opt/ecofloc/gui/execute_gui.sh
	ln -sf /opt/ecofloc/gui/execute_gui.sh /usr/local/bin/ecoflocUI

# Create and activate the virtual environment, then install the required packages

setup_venv: $(REQUIREMENTS)
	python3 -m venv $(VENV_DIR)
	. $(VENV_DIR)/bin/activate && pip install -r $(REQUIREMENTS)

# Copy GUI files -> TODO ->  update names
copy_files:
	cp gui/flocUI.py $(GUI_DIR) 
	cp gui/energy_plotter.py $(GUI_DIR)
	cp gui/floc_daemon.py $(GUI_DIR)
	cp gui/power_plotter.py $(GUI_DIR)
	cp gui/system_monitor.py $(GUI_DIR)
	cp -r gui/assets $(GUI_DIR)
	cp gui/execute_gui.sh $(GUI_DIR)
	cp activities.yaml $(FLOC_DIR)


######## ALL ##########

clean:
	$(MAKE) -C ecofloc-cpu clean
	$(MAKE) -C ecofloc-ram clean
	$(MAKE) -C ecofloc-sd clean
	$(MAKE) -C ecofloc-nic clean
	rm -f ecofloc-cpu.out ecofloc-sd.out sdFeatures.conf cpu_settings.conf ecofloc-ram.out ecofloc-nic.out ecofloc


install: create_ecofloc_folder install_cpu install_sd install_ram install_nic
	cp ecofloc /opt/ecofloc/
	chmod +x /opt/ecofloc/ecofloc-cpu.out /opt/ecofloc/ecofloc-sd.out /opt/ecofloc/ecofloc-ram.out /opt/ecofloc/ecofloc-nic.out /opt/ecofloc/ecofloc
	ln -sf /opt/ecofloc/ecofloc /usr/local/bin/ecofloc


uninstall:
	rm -rf /opt/ecofloc
	rm -f /usr/local/bin/ecofloc-cpu /usr/local/bin/ecofloc-sd /usr/local/bin/ecofloc-ram /usr/local/bin/ecofloc-nic /usr/local/bin/floc /usr/local/bin/flocUI
	rm -f /usr/local/bin/sdFeatures.conf /usr/local/bin/cpu_settings.conf
